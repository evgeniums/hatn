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
#include <hatn/api/server/tcpserver.h>

HATN_API_NAMESPACE_BEGIN

namespace server
{

using PlainTcpConnection=Connection<HATN_NETWORK_NAMESPACE::asio::TcpStream>;
using PlainTcpConnectionContext=common::TaskContextType<HATN_NETWORK_NAMESPACE::asio::TcpStream,PlainTcpConnection,HATN_LOGCONTEXT_NAMESPACE::Context>;

class PlainTcpConnectionTraits
{
    public:

        using ConnectionContext=PlainTcpConnectionContext;

        template <typename ServerContextT>
        auto makeContext(
                common::SharedPtr<ServerContextT> ctx
            ) const
        {
            auto& server=ctx->template get<TcpServer<PlainTcpConnectionTraits>>();
            auto connectionCtx=common::makeTaskContextType<PlainTcpConnectionContext>(
                common::subcontexts(
                    common::subcontext(server.thread()),
                    common::subcontext(),
                    common::subcontext()
                )
            );
            auto& tcpStream=connectionCtx->template get<HATN_NETWORK_NAMESPACE::asio::TcpStream>();
            auto& connection=connectionCtx->template get<PlainTcpConnection>();
            connection.setStreams(&tcpStream);
            return connectionCtx;
        }

        HATN_NETWORK_NAMESPACE::asio::TcpSocket& connectionSocket(common::SharedPtr<ConnectionContext>& ctx) const
        {
            auto& tcpStream=ctx->get<HATN_NETWORK_NAMESPACE::asio::TcpStream>();
            return tcpStream.socket();
        }
};

using PlainTcpServer=TcpServer<PlainTcpConnectionTraits>;

inline auto makePlainTcpServerContext(HATN_COMMON_NAMESPACE::Thread* thread, lib::string_view name={})
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

inline auto makePlainTcpServerContext(const HATN_NETWORK_NAMESPACE::asio::TcpServerConfig* config, HATN_COMMON_NAMESPACE::Thread* thread, lib::string_view name={})
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

}

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::server::PlainTcpConnection,HATN_API_EXPORT)
HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::server::PlainTcpServer,HATN_API_EXPORT)

#endif // HATNAPIPLAINTCPSERVER_H
