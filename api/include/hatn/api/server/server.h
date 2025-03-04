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

#include <hatn/common/objecttraits.h>
#include <hatn/common/error.h>
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
                std::shared_ptr<AuthDispatcher> authDispatcher={}
            ) : m_dispatcher(std::move(dispatcher)),
                m_authDispatcher(std::move(authDispatcher)),
                m_closed(false)
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
        template <typename ConnectionContext,typename Connection>
        void handleNewConnection(common::SharedPtr<ConnectionContext> ctx,
                                 Connection& connection,
                                 std::function<void ()> waitNextConnection
                                )
        {
            if (m_closed)
            {
                return;
            }

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

            // cope env from connection ctx to request

            // recv header
            connection.recv();

            // recv request

            // parse request

            // auth request if auth dispatcher is set

            // dispatch request
        }

    private:

        template <typename Connection>
        void authRequest(common::SharedPtr<RequestContext<Env>> req, Connection& connection)
        {
            if (m_closed)
            {
                return;
            }

            auto self=this->shared_from_this();
            m_authDispatcher->dispatch(std::move(req),
                                   [self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Env>> req, bool closeConnection)
                                   {
                                        // check if connection was destroyed
                                        auto ctx=req->connectionCtx.lock();
                                        if (!ctx)
                                        {
                                            return;
                                        }

                                        // close connection if needed
                                        if (closeConnection)
                                        {
                                            connection.close();
                                            return;
                                        }

                                        // wait for next request
                                        if (req->response.status()==ResponseStatus::OK)
                                        {
                                            // auth is ok
                                            dispatchRequest(std::move(req),connection);
                                        }
                                        else
                                        {
                                            // auth failed
                                            //! @todo send response
                                            waitForRequest(std::move(ctx));
                                        }
                                   }
                                );
        }

        template <typename Connection>
        void dispatchRequest(common::SharedPtr<RequestContext<Env>> req, Connection& connection)
        {
            if (m_closed)
            {
                return;
            }

            auto self=this->shared_from_this();
            m_dispatcher->dispatch(std::move(req),
                                   [self{std::move(self)},this,&connection](common::SharedPtr<RequestContext<Env>> req, bool closeConnection)
                {
                    // check if connection was destroyed
                    auto ctx=req->connectionCtx.lock();
                    if (!ctx)
                    {
                        return;
                    }

                    // close connection if needed
                    if (closeConnection)
                    {
                        connection.close();
                        return;
                    }

                    //! @todo send response

                    // wait for next request
                    waitForRequest(std::move(ctx));
                }
            );
        }

        bool m_closed;
        std::shared_ptr<Dispatcher> m_dispatcher;
        std::shared_ptr<AuthDispatcher> m_authDispatcher;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVER_H
