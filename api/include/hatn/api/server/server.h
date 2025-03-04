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

template <typename Dispatcher, typename AuthDispatcher, typename EnvT=SimpleEnv>
class Server : public std::enable_shared_from_this<Server<Dispatcher,EnvT>>
{
    public:

        using Env=EnvT;

        Server(                
                std::shared_ptr<Dispatcher> dispatcher,
                std::shared_ptr<AuthDispatcher> authDispatcher={},
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_dispatcher(std::move(dispatcher)),
                m_authDispatcher(std::move(authDispatcher)),
                m_closed(false),
                m_allocatorFactory(factory)
        {}

        void close()
        {
            m_closed=true;
        }

        bool isClosed() const noexcept
        {
            return m_closed;
        }

        /**
         * @brief Handle new connection by server.
         * @param ctx Connection context. Note that the context is not owned by the server and must be stored somewhere else outside the server.
         * @param connection Connection.
         * @param waitNextConnection Callback for waiting for the next connection.
         */
        template <typename ConnectionContext, typename Connection>
        void handleNewConnection(common::SharedPtr<ConnectionContext> ctx,
                                 Connection& connection,
                                 std::function<void ()> waitNextConnection
                                )
        {
            if (m_closed)
            {
                return;
            }

            //! @todo maybe log

            waitForRequest(std::move(ctx),connection);
            waitNextConnection();
        }

        template <typename ConnectionContext, typename Connection>
        void waitForRequest(common::SharedPtr<ConnectionContext> ctx, Connection& connection)
        {
            if (m_closed)
            {
                return;
            }

            // create request
            auto reqCtx=allocateRequestContext(m_allocatorFactory);
            auto& req=reqCtx->template get<Request<Env>>();
            req.connectionCtx=ctx;

            // copy env from connection ctx to request
            req.env=ctx->template get<WithEnv<Env>>().envShared();

            // recv header
            auto self=this->shared_from_this();
            connection.recv(
                std::move(reqCtx),
                req.header.data(),
                req.header.size(),
                [ctx{std::move(ctx)},self{std::move(self)},this,&connection,&req](common::SharedPtr<RequestContext<Env>> reqCtx, const Error& ec, size_t)
                {
                    // handle error
                    if (ec)
                    {
                        //! @todo Log eror?

                        //! @todo log
                        closeRequest(reqCtx);

                        return;
                    }

                    // if no message then wait for the next request
                    if (req.header.messageSize()==0)
                    {
                        //! @todo log
                        closeRequest(reqCtx);

                        waitForRequest(std::move(ctx));
                        return;
                    }

                    if (req.header.messageSize()>req->env->maxMessageSize())
                    {
                        //! @todo report error to client
                        //! @todo log error?

                        //! @todo log
                        connection.close();
                        closeRequest(reqCtx);
                        return;
                    }

                    // receive message
                    req.rawData()->resize(req.header.messageSize());
                    connection.recv(
                        std::move(reqCtx),
                        req.rawData()->data(),
                        req.rawData()->size(),
                        [ctx{std::move(ctx)},self{std::move(self)},this,&connection,&req](common::SharedPtr<RequestContext<Env>> reqCtx, const Error& ec, size_t)
                        {
                            // handle error
                            if (ec)
                            {
                                //! @todo Log eror?

                                //! @todo log
                                closeRequest(reqCtx);

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

                                waitForRequest(std::move(ctx));
                                return;
                            }

                            // auth request if auth dispatcher is set
                            if (m_authDispatcher)
                            {
                                authRequest(std::move(reqCtx),connection);
                            }
                            else
                            {
                                dispatchRequest(std::move(reqCtx),connection);
                            }
                        }
                    );
                }
            );
        }

    private:

        template <typename Connection>
        void authRequest(common::SharedPtr<RequestContext<Env>> reqCtx, Connection& connection)
        {
            if (m_closed)
            {
                //! @todo log
                closeRequest(reqCtx);

                return;
            }

            auto self=this->shared_from_this();
            m_authDispatcher->dispatch(std::move(reqCtx),
                                   [self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Env>> reqCtx, bool closeConnection)
                                   {
                                        if (m_closed)
                                        {
                                           //! @todo log
                                           closeRequest(reqCtx);

                                           return;
                                        }
                                        auto& req=reqCtx->template get<Request<Env>>();

                                        // check if connection was destroyed
                                        auto ctx=req.connectionCtx.lock();
                                        if (!ctx)
                                        {
                                            //! @todo log
                                            closeRequest(reqCtx);

                                            return;
                                        }

                                        // close connection if needed
                                        if (closeConnection)
                                        {
                                            //! @todo log
                                            connection.close();
                                            closeRequest(reqCtx);
                                            return;
                                        }

                                        // check auth status
                                        if (req.response.status()==ResponseStatus::OK)
                                        {
                                            // auth is ok, dispatch request
                                            dispatchRequest(std::move(req),connection);
                                        }
                                        else
                                        {
                                            // auth failed, send response
                                            sendResponse(std::move(ctx),std::move(reqCtx),connection);
                                        }
                                   }
                                );
        }

        template <typename Connection>
        void dispatchRequest(common::SharedPtr<RequestContext<Env>> reqCtx, Connection& connection)
        {
            if (m_closed)
            {
                //! @todo log
                closeRequest(reqCtx);

                return;
            }

            auto self=this->shared_from_this();
            m_dispatcher->dispatch(std::move(reqCtx),
                                   [self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Env>> reqCtx, bool closeConnection)
                {
                    if (m_closed)
                    {
                       //! @todo log
                       closeRequest(reqCtx);
                       return;
                    }
                    auto& req=reqCtx->template get<Request<Env>>();

                    // check if connection was destroyed
                    auto ctx=req.connectionCtx.lock();
                    if (!ctx)
                    {
                        //! @todo log
                        closeRequest(reqCtx);
                        return;
                    }

                    // close connection if needed
                    if (closeConnection)
                    {
                        connection.close();

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
        void sendResponse(common::SharedPtr<ConnectionContext> ctx, common::SharedPtr<RequestContext<Env>> reqCtx, Connection& connection)
        {
            if (m_closed)
            {
                //! @todo Log that server is closed
                closeRequest(reqCtx);
                return;
            }
            auto& req=reqCtx->template get<Request<Env>>();

            // serialize response
            auto ec=req.response.serialize();
            if (ec)
            {
                //! @todo Report internal server error
                //! @todo log error

                closeRequest(reqCtx);

                // wait for next request
                waitForRequest(std::move(ctx));
            }

            auto self=this->shared_from_this();

            // send header
            req.header.setMessageSize(req.response.size());
            connection.send(
                std::move(reqCtx),
                req.header.data(),
                req.header.size(),
                [ctx{std::move(ctx)},self{std::move(self)},this,&connection,&req](common::SharedPtr<RequestContext<Env>> reqCtx, const Error& ec, size_t,common::SpanBuffers)
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
                        return;
                    }

                    // send message
                    connection.send(
                        std::move(reqCtx),
                        req.response.buffers(),
                        [ctx{std::move(ctx)},self{std::move(self)},this](common::SharedPtr<RequestContext<Env>> reqCtx, const Error& ec, size_t,common::SpanBuffers)
                        {
                            // handle error
                            if (ec)
                            {
                                //! @todo Log eror?
                                closeRequest(reqCtx);
                                return;
                            }

                            // close request
                            closeRequest(reqCtx);

                            // wait for next request
                            waitForRequest(std::move(ctx));
                        }
                    );
                }
            );
        }

        void closeRequest(common::SharedPtr<RequestContext<Env>>& reqCtx)
        {
            //! @todo Close request context and write log
        }

        bool m_closed;
        std::shared_ptr<Dispatcher> m_dispatcher;
        std::shared_ptr<AuthDispatcher> m_authDispatcher;
        const common::pmr::AllocatorFactory* m_allocatorFactory;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVER_H
