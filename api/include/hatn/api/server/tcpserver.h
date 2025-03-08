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
#include <hatn/common/threadwithqueue.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/network/asio/tcpserver.h>

#include <hatn/api/api.h>
#include <hatn/api/server/env.h>

HATN_API_NAMESPACE_BEGIN

namespace server
{

template <typename Traits>
class TcpServer : public WithEnv<typename Traits::Env>,
                  public HATN_NETWORK_NAMESPACE::asio::TcpServer,
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
            common::ThreadQWithTaskContext* thread,
            TraitsArgs&& ...traitsArgs
        ) : Base(thread,config),
            common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...)
        {}

        template <typename ...TraitsArgs>
        TcpServer
        (
            common::ThreadQWithTaskContext* thread,
            TraitsArgs&& ...traitsArgs
        ) : Base(thread),
            common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...)
        {}

        void setConnectionHandler(ConnectionHandler handler)
        {
            handleNewConnection=std::move(handler);
        }

        void setAllocatorFactory(const common::pmr::AllocatorFactory* factory) noexcept
        {
            m_allocatorFactory=factory;
        }

        const common::pmr::AllocatorFactory* allocatorFactory() const noexcept
        {
            return m_allocatorFactory;
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

        template <typename ServerContextT>
        auto allocateContext(
            common::SharedPtr<ServerContextT> ctx
            )
        {
            return this->traits().allocateContext(m_allocatorFactory->objectAllocator<ConnectionContext>(),std::move(ctx));
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
            auto connectionCtx=allocateContext(serverCtx);
            auto cb=[connectionCtx,serverCtx{std::move(serverCtx)},this](const Error& ec)
            {
                if (ec)
                {
                    this->handleNewConnection(std::move(connectionCtx),ec);
                    return;
                }

                Base::thread()->execAsync(
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

        const common::pmr::AllocatorFactory* m_allocatorFactory=common::pmr::AllocatorFactory::getDefault();
};

}

HATN_API_NAMESPACE_END

#endif // HATNAPITCPSERVER_H
