/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/plaintcpserver.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIPLAINTCPSERVER_H
#define HATNAPIPLAINTCPSERVER_H

#include <hatn/network/asio/tcpstream.h>

#include <hatn/api/api.h>
#include <hatn/api/connection.h>
#include <hatn/api/tenancy.h>
#include <hatn/api/server/env.h>
#include <hatn/api/server/tcpserver.h>

HATN_API_NAMESPACE_BEGIN

namespace server
{

using PlainTcpConnection=Connection<HATN_NETWORK_NAMESPACE::asio::TcpStream>;

template <typename EnvT=BasicEnv>
using PlainTcpConnectionContextT=common::TaskContextType<HATN_NETWORK_NAMESPACE::asio::TcpStream,
                                                          PlainTcpConnection,
                                                          WithEnv<EnvT>,
                                                          HATN_LOGCONTEXT_NAMESPACE::Context
                                                          >;
using PlainTcpConnectionContext=PlainTcpConnectionContextT<>;

template <typename EnvT=BasicEnv>
class PlainTcpConnectionTraits
{
    public:

        PlainTcpConnectionTraits(TcpServer<PlainTcpConnectionTraits<EnvT>>*)
        {}

        using Env=EnvT;
        using ConnectionContext=PlainTcpConnectionContextT<Env>;

        template <typename ServerContextT>
        auto makeContext(
                common::SharedPtr<ServerContextT> ctx
            ) const
        {
            auto& server=ctx->template get<TcpServer<PlainTcpConnectionTraits>>();
            auto connectionCtx=common::makeTaskContextType<PlainTcpConnectionContextT<Env>>(
                common::subcontexts(
                    common::subcontext(server.env()->threads()->randomThread()),
                    common::subcontext(),
                    common::subcontext(),
                    common::subcontext()
                )
            );
            auto& withEnv=connectionCtx->template get<WithEnv<EnvT>>();
            withEnv.setEnv(server.envShared());
            auto& tcpStream=connectionCtx->template get<HATN_NETWORK_NAMESPACE::asio::TcpStream>();
            auto& connection=connectionCtx->template get<PlainTcpConnection>();
            connection.setStreams(&tcpStream);
            return connectionCtx;
        }

        template <typename ServerContextT>
        auto allocateContext(
            const common::pmr::polymorphic_allocator<PlainTcpConnectionContextT<Env>>& allocator,
            common::SharedPtr<ServerContextT> ctx
            ) const
        {
            auto& server=ctx->template get<TcpServer<PlainTcpConnectionTraits>>();
            auto connectionCtx=common::allocateTaskContextType<PlainTcpConnectionContextT<Env>>(
                allocator,
                common::subcontexts(
                    common::subcontext(server.env()->template get<Threads>().threads()->randomThread()),
                    common::subcontext(),
                    common::subcontext(),
                    common::subcontext()
                    )
                );
            auto& withEnv=connectionCtx->template get<WithEnv<EnvT>>();
            withEnv.setEnv(server.envShared());
            auto& tcpStream=connectionCtx->template get<HATN_NETWORK_NAMESPACE::asio::TcpStream>();
            auto& connection=connectionCtx->template get<PlainTcpConnection>();
            connection.setStreams(&tcpStream);
            return connectionCtx;
        }

        static HATN_NETWORK_NAMESPACE::asio::TcpSocket& connectionSocket(common::SharedPtr<ConnectionContext>& ctx)
        {
            auto& tcpStream=ctx->template get<HATN_NETWORK_NAMESPACE::asio::TcpStream>();
            return tcpStream.socket();
        }

        static auto& env(common::SharedPtr<ConnectionContext>& ctx)
        {
            return ctx->template get<Env>();
        }
};
template <typename Env=BasicEnv>
using PlainTcpServerT=TcpServer<PlainTcpConnectionTraits<Env>>;

using PlainTcpServer=PlainTcpServerT<>;

inline auto makePlainTcpServerContext(HATN_COMMON_NAMESPACE::ThreadQWithTaskContext* thread, lib::string_view name={})
{
    return HATN_COMMON_NAMESPACE::makeTaskContext<
        HATN_API_NAMESPACE::server::PlainTcpServer,
        HATN_LOGCONTEXT_NAMESPACE::Context
        >(
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(thread),
            HATN_COMMON_NAMESPACE::subcontext()
            ),
        name
        );
}

inline auto makePlainTcpServerContext(const HATN_NETWORK_NAMESPACE::asio::TcpServerConfig* config, HATN_COMMON_NAMESPACE::ThreadQWithTaskContext* thread, lib::string_view name={})
{
    return HATN_COMMON_NAMESPACE::makeTaskContext<
        HATN_API_NAMESPACE::server::PlainTcpServer,
        HATN_LOGCONTEXT_NAMESPACE::Context
        >(
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(config,thread),
            HATN_COMMON_NAMESPACE::subcontext()
            ),
        name
        );
}

using PlainTcpServerContext=HATN_COMMON_NAMESPACE::TaskContextType<
    HATN_API_NAMESPACE::server::PlainTcpServer,
    HATN_LOGCONTEXT_NAMESPACE::Context
    >;

inline auto allocatePlainTcpServerContext(const common::pmr::polymorphic_allocator<PlainTcpServerContext>& allocator, HATN_COMMON_NAMESPACE::ThreadQWithTaskContext* thread, lib::string_view name={})
{
    return HATN_COMMON_NAMESPACE::allocateTaskContextType<PlainTcpServerContext>(
        allocator,
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(thread),
            HATN_COMMON_NAMESPACE::subcontext()
            ),
        name
        );
}

inline auto allocatePlainTcpServerContext(const common::pmr::polymorphic_allocator<PlainTcpServerContext>& allocator, const HATN_NETWORK_NAMESPACE::asio::TcpServerConfig* config, HATN_COMMON_NAMESPACE::ThreadQWithTaskContext* thread, lib::string_view name={})
{
    return HATN_COMMON_NAMESPACE::allocateTaskContextType<PlainTcpServerContext>(
        allocator,
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(config,thread),
            HATN_COMMON_NAMESPACE::subcontext()
            ),
        name
        );
}

}

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::server::PlainTcpConnection,HATN_API_EXPORT)
HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::server::PlainTcpServer,HATN_API_EXPORT)

#endif // HATNAPIPLAINTCPSERVER_H
