/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/server.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVER_H
#define HATNAPISERVER_H

#include <memory>

#include <hatn/common/pmr//allocatorfactory.h>
#include <hatn/common/sharedptr.h>

#include <hatn/api/api.h>
#include <hatn/api/apiliberror.h>
#include <hatn/api/server/serverrequest.h>
#include <hatn/api/server/env.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename ConnectionsStoreT, typename DispatcherT, typename AuthDispatcherT=DispatcherT, typename EnvT=BasicEnv, typename RequestT=Request<EnvT>>
class Server : public std::enable_shared_from_this<Server<ConnectionsStoreT,DispatcherT,AuthDispatcherT,EnvT,RequestT>>
{
    public:

        using Env=EnvT;
        using Request=RequestT;

        Server(
                std::shared_ptr<ConnectionsStoreT> connectionsStore,
                std::shared_ptr<DispatcherT> dispatcher,
                std::shared_ptr<AuthDispatcherT> authDispatcher={}
            ) : m_connectionsStore(std::move(connectionsStore)),
                m_dispatcher(std::move(dispatcher)),
                m_authDispatcher(std::move(authDispatcher)),
                m_closed(false)
        {}

        void close()
        {
            m_closed=true;
            closeAllConnections();
        }

        bool isClosed() const noexcept
        {
            return m_closed;
        }

        std::shared_ptr<DispatcherT> dispatcher() const
        {
            return m_dispatcher;
        }

        std::shared_ptr<AuthDispatcherT> authDispatcher() const
        {
            return m_authDispatcher;
        }

        /**
         * @brief Handle new connection by server.
         * @param ctx Connection context.
         * @param connection Connection.
         * @param waitNextConnection Callback for waiting for the next connection.
         */
        template <typename ConnectionContext, typename Connection, typename CallbackT>
        void handleNewConnection(common::SharedPtr<ConnectionContext> ctx,
                                 Connection& connection,
                                 CallbackT waitNextConnection
                                )
        {
            if (m_closed)
            {
                return;
            }
            m_connectionsStore->registerConnection(ctx);

            waitForRequest(std::move(ctx),connection);
            if (waitNextConnection)
            {
                waitNextConnection(Error{});
            }
        }

        template <typename ConnectionContext, typename Connection>
        void waitForRequest(common::SharedPtr<ConnectionContext> ctx, Connection& connection)
        {
            if (m_closed)
            {
                return;
            }

            // create request
            auto reqCtx=allocateRequestContext<Env,typename Request::RequestUnit>(ctx->template get<WithEnv<Env>>().envShared());
            auto& req=reqCtx->template get<Request>();
            req.connectionCtx=ctx;
            req.requestThread=common::ThreadQWithTaskContext::current();
            auto& envLogger=req.env->template get<Logger>();
            auto& logCtx=reqCtx->template get<HATN_LOGCONTEXT_NAMESPACE::Context>();
            logCtx.setLogger(envLogger.logger());

            ctx->resetParentCtx(reqCtx);
            //! @todo Fill request parameters with connection parameters

            //! @todo Log tenancy / handle tenancy
            //! @todo Log env
            //! @todo Log connection

            auto reqPtr=reqCtx.get();
            reqPtr->beforeThreadProcessing();

            HATN_CTX_SCOPE("apiwaitforrequest")

            // recv header
            auto self=this->shared_from_this();
            connection.recv(
                std::move(reqCtx),
                req.header.data(),
                req.header.size(),
                [ctx{std::move(ctx)},self{std::move(self)},this,&connection,&req](common::SharedPtr<RequestContext<Request>> reqCtx, const Error& ec)
                {
                    HATN_CTX_SCOPE("apireqheader")

                    // handle error
                    if (ec)
                    {
                        HATN_CTX_SCOPE_ERROR("recv header failed")
                        closeRequest(reqCtx,ec);
                        resetConnection(ctx);
                        return;
                    }

                    // if no message then wait for the next request
                    if (req.header.messageSize()==0)
                    {
                        HATN_CTX_WARN("zero request size")
                        closeRequest(reqCtx);
                        waitForRequest(std::move(ctx),connection);
                        return;
                    }

                    if (req.header.messageSize() > req.env->template get<ProtocolConfig>().maxMessageSize())
                    {
                        //! @todo Detect HTTP request and optionally redirect to some URL or send http-response

                        req.response.setStatus(protocol::ResponseStatus::RequestTooBig);
                        sendResponse(std::move(ctx),std::move(reqCtx),connection);
                        return;
                    }

                    // receive message
                    req.rawData()->resize(req.header.messageSize());
                    connection.recv(
                        std::move(reqCtx),
                        req.rawData()->data(),
                        req.rawData()->size(),
                        [ctx{std::move(ctx)},self{std::move(self)},this,&connection,&req](common::SharedPtr<RequestContext<Request>> reqCtx, const Error& ec)
                        {
                            HATN_CTX_SCOPE("apireqbody")

                            // handle error
                            if (ec)
                            {
                                HATN_CTX_SCOPE_ERROR("recv body failed")
                                closeRequest(reqCtx,ec);
                                resetConnection(ctx);
                                return;
                            }

                            // parse request
                            auto ec1=req.parseMessage();
                            if (ec1)
                            {
                                req.response.setStatus(protocol::ResponseStatus::FormatError,ec1);
                                sendResponse(std::move(ctx),std::move(reqCtx),connection);
                                return;
                            }

                            //! @todo Validate request

                            //! @todo Handle proxy field if allowed by server settings

                            auto& req=reqCtx->template get<Request>();
                            HATN_CTX_PUSH_VAR("mthd",req.unit.fieldValue(protocol::request::method))
                            HATN_CTX_PUSH_VAR("req",lib::string_view{req.unit.fieldValue(protocol::request::id).string()})
                            HATN_CTX_PUSH_VAR("srv",req.unit.fieldValue(protocol::request::service))
                            if (req.unit.field(protocol::request::service_version).isSet())
                            {
                                HATN_CTX_PUSH_VAR("s_ver",req.unit.fieldValue(protocol::request::service_version))
                            }
                            if (req.unit.field(protocol::request::topic).isSet())
                            {
                                HATN_CTX_PUSH_VAR("tpc",req.unit.fieldValue(protocol::request::topic))
                            }
                            if (req.unit.field(protocol::request::message_type).isSet())
                            {
                                HATN_CTX_PUSH_VAR("typ",req.unit.fieldValue(protocol::request::message_type))
                            }

                            // auth request if auth dispatcher is set
                            if (m_authDispatcher)
                            {
                                authRequest(std::move(ctx),std::move(reqCtx),connection);
                            }
                            else
                            {
                                dispatchRequest(std::move(ctx),std::move(reqCtx),connection);
                            }
                        }
                    );
                }
            );

            reqPtr->afterThreadProcessing();
        }

        auto closeConnection(const lib::string_view& id)
        {
            return m_connectionsStore->closeConnection(id);
        }

        void closeAllConnections()
        {
            return m_connectionsStore->clear();
        }

        size_t connectionCount() const noexcept
        {
            return m_connectionsStore->count();
        }

    private:

        template <typename ConnectionContext, typename Connection>
        void authRequest(common::SharedPtr<ConnectionContext> ctx, common::SharedPtr<RequestContext<Request>> reqCtx, Connection& connection)
        {
            HATN_CTX_SCOPE("apiauthrequest")

            if (m_closed)
            {
                closeRequest(reqCtx,apiLibError(ApiLibError::SERVER_CLOSED));
                return;
            }

            auto wCtx=common::toWeakPtr(ctx);
            watchConnection(std::move(ctx),connection);

            auto self=this->shared_from_this();
            m_authDispatcher->dispatch(std::move(reqCtx),
                                        [wCtx{std::move(wCtx)},self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Request>> reqCtx)
                                   {
                                        HATN_CTX_SCOPE("apiauthcb")

                                        if (m_closed)
                                        {
                                            closeRequest(reqCtx,apiLibError(ApiLibError::SERVER_CLOSED));
                                            return;
                                        }
                                        auto& req=reqCtx->template get<Request>();
                                        auto ctx=wCtx.lock();

                                        // close connection if needed
                                        if (req.closeConnection && ctx)
                                        {
                                            closeRequest(reqCtx,apiLibError(ApiLibError::FORCE_CONNECTION_CLOSE));
                                            closeConnection(ctx,connection);
                                            return;
                                        }

                                        // cancel connection watching
                                        if (ctx)
                                        {
                                            connection.cancel();
                                        }

                                        // check auth status
                                        if (req.response.status()==protocol::ResponseStatus::Success)
                                        {
                                            // auth is ok, dispatch request
                                            dispatchRequest(std::move(ctx),std::move(reqCtx),connection);
                                        }
                                        else
                                        {
                                            // auth failed, send response
                                            sendResponse(std::move(ctx),std::move(reqCtx),connection);
                                        }
                                   }
                                );
        }

        template <typename ConnectionContext,typename Connection>
        void dispatchRequest(common::SharedPtr<ConnectionContext> ctx, common::SharedPtr<RequestContext<Request>> reqCtx, Connection& connection)
        {
            HATN_CTX_SCOPE("apidispatch")

            if (m_closed)
            {
                closeRequest(reqCtx,apiLibError(ApiLibError::SERVER_CLOSED));
                return;
            }

            auto wCtx=common::toWeakPtr(ctx);
            watchConnection(std::move(ctx),connection);

            auto self=this->shared_from_this();
            m_dispatcher->dispatch(std::move(reqCtx),
                                   [wCtx{std::move(wCtx)},self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Request>> reqCtx)
                {
                    HATN_CTX_SCOPE("apidispatchcb")
                    if (m_closed)
                    {
                       closeRequest(reqCtx,apiLibError(ApiLibError::SERVER_CLOSED));
                       return;
                    }
                    auto& req=reqCtx->template get<Request>();

                    // check if connection was destroyed
                    auto ctx=wCtx.lock();
                    if (!ctx)
                    {
                        closeRequest(reqCtx,apiLibError(ApiLibError::CONNECTION_CLOSED));
                        return;
                    }

                    // cancel connection watching
                    connection.cancel();

                    // close connection if needed
                    if (req.closeConnection)
                    {
                        HATN_CTX_WARN("force closing connection due to request processing result")
                        closeConnection(ctx,connection);                        
                        closeRequest(reqCtx);
                        return;
                    }

                    // send response
                    sendResponse(std::move(ctx),std::move(reqCtx),connection);
                }
            );
        }

        template <typename ConnectionContext, typename Connection>
        void sendResponse(common::SharedPtr<ConnectionContext> ctx, common::SharedPtr<RequestContext<Request>> reqCtx, Connection& connection)
        {
            HATN_CTX_SCOPE("apisendresponse")

            if (m_closed)
            {
                closeRequest(reqCtx,apiLibError(ApiLibError::SERVER_CLOSED));
                return;
            }
            auto& req=reqCtx->template get<Request>();

            // serialize response
            auto ec=req.response.serialize();
            if (ec)
            {
                HATN_CTX_ERROR(ec,"failed to serialize response")
                req.response.setStatus(protocol::ResponseStatus::InternalServerError);
            }

            auto self=this->shared_from_this();

            // send header
            req.header.setMessageSize(static_cast<uint32_t>(req.response.size()));
            connection.send(
                std::move(reqCtx),
                req.header.data(),
                req.header.size(),
                [ctx{std::move(ctx)},self{std::move(self)},this,&connection,&req](common::SharedPtr<RequestContext<Request>> reqCtx, const Error& ec, size_t)
                {
                    HATN_CTX_SCOPE("apirespheadercb")

                    if (m_closed)
                    {
                        closeRequest(reqCtx,apiLibError(ApiLibError::SERVER_CLOSED));
                        return;
                    }

                    // handle error
                    if (ec)
                    {
                        HATN_CTX_SCOPE_ERROR("send header failed")
                        closeRequest(reqCtx,ec);
                        resetConnection(ctx);
                        return;
                    }

                    // send message
                    connection.send(
                        std::move(reqCtx),
                        req.response.buffers(req.env->template get<AllocatorFactory>().factory()),
                        [ctx{std::move(ctx)},self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Request>> reqCtx, const Error& ec, size_t,common::SpanBuffers)
                        {
                            // handle error
                            if (ec)
                            {
                                HATN_CTX_SCOPE("apirespbodycb")
                                HATN_CTX_SCOPE_ERROR("send body failed")
                                closeRequest(reqCtx,ec);
                                resetConnection(ctx);
                                return;
                            }

                            // close request
                            closeRequest(reqCtx);

                            // wait for next request
                            waitForRequest(std::move(ctx),connection);
                        }
                    );
                }
            );
        }

        void closeRequest(common::SharedPtr<RequestContext<Request>>& reqCtx, const Error& ec={})
        {
            auto& req=reqCtx->template get<Request>();
            req.close(ec);
        }

        template <typename ConnectionContext,typename Connection>
        void watchConnection(common::SharedPtr<ConnectionContext> ctx, Connection& connection)
        {
            auto self=this->shared_from_this();
            connection.waitForRead(
                std::move(ctx),
                [&connection,self{std::move(self)},this](common::SharedPtr<ConnectionContext> ctx, const Error&)
                {
                    HATN_CTX_SCOPE("apiwatchconnection")

                    // this callback is invoked only if either connection is broken or unexpected data is received
                    closeConnection(ctx,connection);
                }
            );
        }

        template <typename ConnectionContext,typename Connection>
        void closeConnection(const common::SharedPtr<ConnectionContext>& ctx, Connection& connection)
        {
            ctx->resetParentCtx();
            connection.close();
            resetConnection(ctx);
        }

        template <typename ConnectionContext>
        void resetConnection(const common::SharedPtr<ConnectionContext>& ctx)
        {
            ctx->resetParentCtx();
            m_connectionsStore->removeConnection(ctx->id());
        }

        std::shared_ptr<ConnectionsStoreT> m_connectionsStore;
        std::shared_ptr<DispatcherT> m_dispatcher;
        std::shared_ptr<AuthDispatcherT> m_authDispatcher;
        bool m_closed;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVER_H
