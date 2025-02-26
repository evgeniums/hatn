/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/tcpserver.h
  *
  */

/****************************************************************************/

#ifndef HATNAPITCPSERVER_H
#define HATNAPITCPSERVER_H

#include <hatn/common/objecttraits.h>

#include <hatn/network/asio/tcpserver.h>
#include <hatn/network/asio/tcpstream.h>

#include <hatn/api/api.h>
#include <hatn/api/connection.h>

HATN_API_NAMESPACE_BEGIN

namespace server
{

template <typename Traits>
class TcpServer : public HATN_NETWORK_NAMESPACE::asio::TcpServer,
                  public common::WithTraits<Traits>
{
    public:

        using Base=HATN_NETWORK_NAMESPACE::asio::TcpServer;

        using ConnectionContext=typename Traits::ConnectionContext;
        using ConnectionHandler=std::function<void (common::SharedPtr<ConnectionContext> ctx, const Error& ec)>;

        template <typename ...TraitsArgs>
        TcpServer
        (
            const HATN_NETWORK_NAMESPACE::asio::TcpServerConfig* config,
            common::Thread* thread,
            TraitsArgs&& ...traitsArgs
        ) : Base(thread,config),
            common::WithTraits<Traits>(std::forward<TraitsArgs>(traitsArgs)...)
        {}

        template <typename ...TraitsArgs>
        TcpServer
        (
            common::Thread* thread,
            TraitsArgs&& ...traitsArgs
        ) : Base(thread),
            common::WithTraits<Traits>(std::forward<TraitsArgs>(traitsArgs)...)
        {}

        void setConnectionHandler(ConnectionHandler handler)
        {
            handleNewConnection=std::move(handler);
        }

        template <typename ServerContextT>
        Error run(
                common::SharedPtr<ServerContextT> serverCtx,
                const HATN_NETWORK_NAMESPACE::asio::TcpEndpoint& endpoint
            )
        {
            auto ec=this->listen(endpoint);
            if (ec)
            {
                return ec;
            }

            acceptNext(std::move(serverCtx));
            return OK;
        }

        template <typename ServerContextT>
        auto makeContext(
                common::SharedPtr<ServerContextT> ctx
            )
        {
            return this->traits().makeContext(std::move(ctx));
        }

        HATN_NETWORK_NAMESPACE::asio::TcpSocket& connectionSocket(common::SharedPtr<ConnectionContext>& ctx)
        {
            return this->traits().connectionSocket(ctx);
        }

    private:

        ConnectionHandler handleNewConnection;

        template <typename ServerContextT>
        void acceptNext(
            common::SharedPtr<ServerContextT> serverCtx
            )
        {
            auto connectionCtx=makeContext(serverCtx);
            auto cb=[connectionCtx,serverCtx{std::move(serverCtx)},this](const Error& ec)
            {
                if (ec)
                {
                    this->handleNewConnection(std::move(connectionCtx),ec);
                    return;
                }

                this->thread()->execAsync(
                    [connectionCtx{std::move(connectionCtx)},serverCtx,this]()
                    {
                        std::ignore=serverCtx;
                        connectionCtx->onAsyncHandlerEnter();
                        HATN_CTX_SCOPE("tcpserveraccept")
                        this->handleNewConnection(connectionCtx,Error{});
                        connectionCtx->onAsyncHandlerExit();
                    }
                );

                this->acceptNext(std::move(serverCtx));
            };
            this->accept(connectionSocket(connectionCtx),std::move(cb));
        }
};

using PlainConnectionContext=common::TaskContextType<HATN_NETWORK_NAMESPACE::asio::TcpStream,HATN_LOGCONTEXT_NAMESPACE::Context>;

class TcpConnectionTraits
{
    public:

        using ConnectionContext=PlainConnectionContext;

        template <typename ServerContextT>
        auto makeContext(
                common::SharedPtr<ServerContextT> ctx
            ) const
        {
            auto& server=ctx->template get<TcpServer<TcpConnectionTraits>>();
            return common::makeTaskContext<HATN_NETWORK_NAMESPACE::asio::TcpStream,HATN_LOGCONTEXT_NAMESPACE::Context>(
                common::subcontexts(
                    common::subcontext(server.thread()),
                    common::subcontext()
                )
            );
        }

        HATN_NETWORK_NAMESPACE::asio::TcpSocket& connectionSocket(common::SharedPtr<ConnectionContext>& ctx) const
        {
            auto& tcpStream=ctx->get<HATN_NETWORK_NAMESPACE::asio::TcpStream>();
            return tcpStream.socket();
        }
};

using PlainTcpServer=TcpServer<TcpConnectionTraits>;

}

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::server::PlainTcpServer,HATN_API_EXPORT)

#endif // HATNAPITCPSERVER_H
