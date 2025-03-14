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

#include <functional>
#include <memory>

#include <hatn/common/pmr//allocatorfactory.h>
#include <hatn/common/sharedptr.h>

#include <hatn/api/api.h>
#include <hatn/api/server/request.h>
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
                std::shared_ptr<AuthDispatcherT> authDispatcher={},
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_connectionsStore(std::move(connectionsStore)),
                m_dispatcher(std::move(dispatcher)),
                m_authDispatcher(std::move(authDispatcher)),
                m_closed(false),
                m_allocatorFactory(factory)
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

        const common::pmr::AllocatorFactory* factory() const noexcept
        {
            return m_allocatorFactory;
        }

        /**
         * @brief Handle new connection by server.
         * @param ctx Connection context.
         * @param connection Connection.
         * @param waitNextConnection Callback for waiting for the next connection.
         */
        template <typename ConnectionContext, typename Connection>
        void handleNewConnection(common::SharedPtr<ConnectionContext> ctx,
                                 Connection& connection,
                                 std::function<void ()> waitNextConnection={}
                                )
        {
            if (m_closed)
            {
                return;
            }
            m_connectionsStore->registerConnection(ctx);

            //! @todo maybe log

            waitForRequest(std::move(ctx),connection);
            if (waitNextConnection)
            {
                waitNextConnection();
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
            //! @todo set current thread as request's thread
            auto reqCtx=allocateRequestContext(m_allocatorFactory);
            auto& req=reqCtx->template get<Request>();
            req.connectionCtx=ctx;

            // copy env from connection ctx to request
            req.env=ctx->template get<WithEnv<Env>>().envShared();

            // recv header
            auto self=this->shared_from_this();
            connection.recv(
                std::move(reqCtx),
                req.header.data(),
                req.header.size(),
                [ctx{std::move(ctx)},self{std::move(self)},this,&connection,&req](common::SharedPtr<RequestContext<Request>> reqCtx, const Error& ec)
                {
                    // handle error
                    if (ec)
                    {
                        //! @todo log
                        closeRequest(reqCtx);
                        resetConnection(ctx);
                        return;
                    }

                    // if no message then wait for the next request
                    if (req.header.messageSize()==0)
                    {
                        //! @todo log
                        closeRequest(reqCtx);
                        waitForRequest(std::move(ctx),connection);
                        return;
                    }

                    if (req.header.messageSize()>req.env->template get<ProtocolConfig>().maxMessageSize())
                    {
                        //! @todo report error to client
                        //! @todo log error?

                        //! @todo log                        
                        closeRequest(reqCtx);
                        closeConnection(ctx,connection);
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
                            // handle error
                            if (ec)
                            {
                                //! @todo Log eror?

                                //! @todo log
                                closeRequest(reqCtx);
                                resetConnection(ctx);
                                return;
                            }

                            // parse request
                            auto ec1=req.parseMessage();
                            if (!ec1)
                            {
                                //! @todo report error to client
                                //! @todo log error?

                                //! @todo log
                                closeRequest(reqCtx);
                                waitForRequest(std::move(ctx),connection);
                                return;
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
            if (m_closed)
            {
                //! @todo log
                closeRequest(reqCtx);
                return;
            }

            auto wCtx=common::toWeakPtr(ctx);
            monitorConnection(std::move(ctx),connection);

            auto self=this->shared_from_this();
            m_authDispatcher->dispatch(std::move(reqCtx),
                                        [wCtx{std::move(wCtx)},self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Request>> reqCtx)
                                   {
                                        if (m_closed)
                                        {
                                           //! @todo log
                                           closeRequest(reqCtx);
                                           return;
                                        }
                                        auto& req=reqCtx->template get<Request>();
                                        auto ctx=wCtx.lock();

                                        // close connection if needed
                                        if (req.closeConnection && ctx)
                                        {
                                            //! @todo log                                            
                                            closeRequest(reqCtx);
                                            closeConnection(ctx,connection);
                                            return;
                                        }

                                        // cancel connection monitoring
                                        if (ctx)
                                        {
                                            connection.cancel();
                                        }

                                        // check auth status
                                        if (req.response.status()==protocol::ResponseStatus::OK)
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
            if (m_closed)
            {
                //! @todo log
                closeRequest(reqCtx);

                return;
            }

            auto wCtx=common::toWeakPtr(ctx);
            monitorConnection(std::move(ctx),connection);

            auto self=this->shared_from_this();
            m_dispatcher->dispatch(std::move(reqCtx),
                                   [wCtx{std::move(wCtx)},self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Request>> reqCtx)
                {
                    if (m_closed)
                    {
                       //! @todo log
                       closeRequest(reqCtx);
                       return;
                    }
                    auto& req=reqCtx->template get<Request>();

                    // check if connection was destroyed
                    auto ctx=wCtx.lock();
                    if (!ctx)
                    {
                        //! @todo log
                        closeRequest(reqCtx);
                        return;
                    }

                    // cancel connection monitoring
                    connection.cancel();

                    // close connection if needed
                    if (req.closeConnection)
                    {
                        closeConnection(ctx,connection);

                        //! @todo log
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
            if (m_closed)
            {
                //! @todo Log that server is closed
                closeRequest(reqCtx);
                return;
            }
            auto& req=reqCtx->template get<Request>();

            // serialize response
            auto ec=req.response.serialize();
            if (ec)
            {
                //! @todo Report internal server error
                //! @todo log error

                closeRequest(reqCtx);

                // wait for next request
                waitForRequest(std::move(ctx),connection);
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
                    if (m_closed)
                    {
                        closeRequest(reqCtx);
                        return;
                    }

                    // handle error
                    if (ec)
                    {
                        //! @todo Log eror?
                        closeRequest(reqCtx);
                        resetConnection(ctx);
                        return;
                    }

                    // send message
                    connection.send(
                        std::move(reqCtx),
                        req.response.buffers(),
                        [ctx{std::move(ctx)},self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Request>> reqCtx, const Error& ec, size_t,common::SpanBuffers)
                        {
                            // handle error
                            if (ec)
                            {
                                //! @todo Log eror?
                                closeRequest(reqCtx);
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

        void closeRequest(common::SharedPtr<RequestContext<Request>>& reqCtx)
        {
            auto& req=reqCtx->template get<Request>();
            req.close();
        }

        template <typename ConnectionContext,typename Connection>
        void monitorConnection(common::SharedPtr<ConnectionContext> ctx, Connection& connection)
        {
            auto self=this->shared_from_this();
            connection.waitForRead(
                std::move(ctx),
                [&connection,self{std::move(self)},this](common::SharedPtr<ConnectionContext> ctx, const Error&)
                {
                    // this callback is invoked only if either connection is broken or unexpected data is received
                    closeConnection(ctx,connection);
                }
            );
        }

        template <typename ConnectionContext,typename Connection>
        void closeConnection(const common::SharedPtr<ConnectionContext>& ctx, Connection& connection)
        {
            connection.close();
            resetConnection(ctx);
        }

        template <typename ConnectionContext>
        void resetConnection(const common::SharedPtr<ConnectionContext>& ctx)
        {
            m_connectionsStore->removeConnection(ctx->id());
        }

        std::shared_ptr<ConnectionsStoreT> m_connectionsStore;
        std::shared_ptr<DispatcherT> m_dispatcher;
        std::shared_ptr<AuthDispatcherT> m_authDispatcher;
        bool m_closed;
        const common::pmr::AllocatorFactory* m_allocatorFactory;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVER_H
