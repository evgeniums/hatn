/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/plaintcpconnection.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIPLAINTCPCONNECTON_H
#define HATNAPIPLAINTCPCONNECTON_H

#include <hatn/logcontext/logcontext.h>

#include <hatn/api/api.h>
#include <hatn/api/connection.h>
#include <hatn/api/client/tcpclient.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

using PlainTcpConnection=Connection<TcpClient>;
using PlainTcpConnectionContext=common::TaskContextType<TcpClient,PlainTcpConnection,HATN_LOGCONTEXT_NAMESPACE::Context>;

inline auto makePlainTcpConnectionContext(
        std::vector<IpHostName> hosts,
        std::shared_ptr<IpHostResolver> resolver,
        HATN_COMMON_NAMESPACE::Thread* thread=common::Thread::currentThread(),
        std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle={},
        lib::string_view name={}
    )
{
    auto connectionCtx=HATN_COMMON_NAMESPACE::makeTaskContextType<PlainTcpConnectionContext>(
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(std::move(hosts),std::move(resolver),thread,std::move(shuffle)),
            HATN_COMMON_NAMESPACE::subcontext(),
            HATN_COMMON_NAMESPACE::subcontext()
            ),
        name
        );
    auto& tcpClient=connectionCtx->get<TcpClient>();
    auto& connection=connectionCtx->get<PlainTcpConnection>();
    connection.setStreams(&tcpClient);
    return connectionCtx;
}

inline auto makePlainTcpConnectionContext(
    std::vector<IpHostName> hosts,
    std::shared_ptr<IpHostResolver> resolver,
    HATN_COMMON_NAMESPACE::Thread* thread,
    lib::string_view name={}
    )
{
    return makePlainTcpConnectionContext(std::move(hosts),std::move(resolver),thread,{},name);
}

inline auto allocatePlainTcpConnectionContext(
    const HATN_COMMON_NAMESPACE::pmr::polymorphic_allocator<PlainTcpConnectionContext>& allocator,
    std::vector<IpHostName> hosts,
    std::shared_ptr<IpHostResolver> resolver,
    HATN_COMMON_NAMESPACE::Thread* thread=common::Thread::currentThread(),
    std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle={},
    lib::string_view name={}
    )
{
    auto connectionCtx=HATN_COMMON_NAMESPACE::allocateTaskContextType<PlainTcpConnectionContext>(
        allocator,
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(std::move(hosts),std::move(resolver),thread,std::move(shuffle)),
            HATN_COMMON_NAMESPACE::subcontext(),
            HATN_COMMON_NAMESPACE::subcontext()
            ),
        name
        );
    auto& tcpClient=connectionCtx->get<TcpClient>();
    auto& connection=connectionCtx->get<PlainTcpConnection>();
    connection.setStreams(&tcpClient);
    return connectionCtx;
}

inline auto allocatePlainTcpConnectionContext(
    const HATN_COMMON_NAMESPACE::pmr::polymorphic_allocator<PlainTcpConnectionContext>& allocator,
    std::vector<IpHostName> hosts,
    std::shared_ptr<IpHostResolver> resolver,
    HATN_COMMON_NAMESPACE::Thread* thread,
    lib::string_view name={}
    )
{
    return allocatePlainTcpConnectionContext(allocator,std::move(hosts),std::move(resolver),thread,{},name);
}

} // namespace client

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::client::TcpClient,HATN_API_EXPORT)
HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::client::PlainTcpConnection,HATN_API_EXPORT)

#endif // HATNAPITCPCONNECTON_H
