/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/plaintcpmicroservice.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPIPLAINTCPMICROSERVICE_IPP
#define HATNAPIPLAINTCPMICROSERVICE_IPP

#include <hatn/base/configobject.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/api/server/plaintcpmicroservice.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

HDU_UNIT(tcp_server_config,
    HDU_FIELD(ip_address,TYPE_STRING,1,true)
    HDU_FIELD(port,TYPE_UINT16,2,true)
    HDU_FIELD(max_pending_connections,TYPE_UINT32,3,false,boost::asio::ip::tcp::socket::max_listen_connections)
    HDU_FIELD(server_thread,TYPE_UINT32,4)
)

using TcpServerConfig=HATN_BASE_NAMESPACE::ConfigObject<tcp_server_config::type>;

//---------------------------------------------------------------

template <typename EnvT>
Result<common::SharedPtr<typename PlainTcpMicroServiceConfig<EnvT>::NetworkServerCtx>>
    PlainTcpMicroServiceConfig<EnvT>::makeAndInitNetworkServer(
        lib::string_view name,
        common::SharedPtr<Env> env,
        const HATN_APP_NAMESPACE::BaseApp& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    )
{
    TcpServerConfig config;

    //! @todo log config
    auto ec=config.loadConfig(configTree,configTreePath);
    if (ec)
    {
        auto ec1=apiLibError(ApiLibError::TCP_SERVER_CONFIG_FAILED);
        ec.stackWith(std::move(ec1));
        return ec;
    }
    uint16_t port=config.config().fieldValue(tcp_server_config::port);
    if (port==0)
    {
        ec=apiLibError(ApiLibError::TCP_SERVER_INVALID_IP_PORT);
        return ec;
    }

    // create allocator
    auto& factory=env->template get<AllocatorFactory>();
    auto allocator=factory.factory()->template objectAllocator<NetworkServerCtx>();

    // select server thread
    auto thread=app.appThread();
    if (config.config().isSet(tcp_server_config::server_thread))
    {
        thread=app.thread(config.config().fieldValue(tcp_server_config::server_thread));
    }

    // create server context
    auto tcpServerCtx=HATN_COMMON_NAMESPACE::allocateTaskContextType<NetworkServerCtx>(
        allocator,
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(thread),
            HATN_COMMON_NAMESPACE::subcontext(),
            HATN_COMMON_NAMESPACE::subcontext(config.config().fieldValue(tcp_server_config::max_pending_connections))
            ),
        name
    );

    // set asio server configuration
    const auto& asioServerConfig=tcpServerCtx->template get<HATN_NETWORK_NAMESPACE::asio::TcpServerConfig>();
    auto& tcpServer=tcpServerCtx->template get<NetworkServer>();
    tcpServer.setConfig(&asioServerConfig);

    try
    {
        HATN_NETWORK_NAMESPACE::asio::TcpEndpoint ep{config.config().fieldValue(tcp_server_config::ip_address),port};
        tcpServer.setServerEndpoint(std::move(ep));
    }
    catch (...)
    {
        ec=apiLibError(ApiLibError::TCP_SERVER_INVALID_IP_ADDRESS);
        return ec;
    }

    // done
    return tcpServerCtx;
}

//---------------------------------------------------------------

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIPLAINTCPMICROSERVICE_IPP
