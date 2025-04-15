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
        using ConnectionHandler=std::function<void (common::SharedPtr<ConnectionContext> ctx, const Error& ec, std::function<void (const Error&)> cb)>;

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
            m_handleNewConnection=std::move(handler);
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
        Error run(
                common::SharedPtr<ServerContextT> serverCtx
            )
        {
            return run(std::move(serverCtx),m_serverEndpoint);
        }

        void setServerEndpoint(HATN_NETWORK_NAMESPACE::asio::TcpEndpoint ep)
        {
            m_serverEndpoint=std::move(ep);
        }

        const HATN_NETWORK_NAMESPACE::asio::TcpEndpoint& serverEndpoint() const noexcept
        {
            return m_serverEndpoint;
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

        ConnectionHandler m_handleNewConnection;
        HATN_NETWORK_NAMESPACE::asio::TcpEndpoint m_serverEndpoint;

        template <typename ServerContextT>
        void acceptNext(
                common::SharedPtr<ServerContextT> serverCtx
            )
        {
            auto connectionCtx=allocateContext(serverCtx);
            if (this->env())
            {
                auto& envLogger=this->env()->template get<Logger>();
                auto& logCtx=connectionCtx->template get<HATN_LOGCONTEXT_NAMESPACE::Context>();
                logCtx.setLogger(envLogger.logger());
            }

            auto& socket=connectionSocket(connectionCtx);
            auto cb=[connectionCtx,serverCtx{std::move(serverCtx)},this,&socket](const Error& ec)
            {
                if (ec)
                {
                    this->m_handleNewConnection(std::move(connectionCtx),ec,[](const Error&){});
                    return;
                }

                connectionCtx->onAsyncHandlerEnter();

                HATN_CTX_SCOPE("tcpserveraccept")

                //! @todo Fill connection parameters

                boost::system::error_code ec1;
                auto ep=socket.socket().remote_endpoint(ec1);
                if (!ec1)
                {
                    HATN_CTX_PUSH_VAR("r_ip",ep.address().to_string())
                    HATN_CTX_PUSH_VAR("r_port",ep.port())
                }

                HATN_CTX_DETAILS("connection accepted","tcpserver");

                auto selfCtxW=common::toWeakPtr(this->sharedMainCtx());
                this->m_handleNewConnection(connectionCtx,Error{},
                  [selfCtxW{std::move(selfCtxW)},this,serverCtx{std::move(serverCtx)}](const Error& ec)
                  {
                    if (ec)
                    {
                        return;
                    }

                    auto selfCtx=selfCtxW.lock();
                    if (!selfCtx)
                    {
                        return;
                    }
                    this->acceptNext(std::move(serverCtx));
                  }
                );

                connectionCtx->onAsyncHandlerExit();
            };
            this->accept(socket,std::move(cb));
        }

        const common::pmr::AllocatorFactory* m_allocatorFactory=common::pmr::AllocatorFactory::getDefault();
};

}

HATN_API_NAMESPACE_END

#endif // HATNAPITCPSERVER_H
