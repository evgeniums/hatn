/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/plaintcpmicroservice.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIPLAINTCPMICROSERVICE_H
#define HATNAPIPLAINTCPMICROSERVICE_H

#include <hatn/api/server/env.h>
#include <hatn/api/server/plaintcpserver.h>
#include <hatn/api/server/microservicebuilder.h>
#include <hatn/api/server/networkmicroservice.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename EnvT=BasicEnv>
struct PlainTcpMicroServiceConfig
{
    using Env=EnvT;

    using ConnectionCtx=PlainTcpConnectionContextT<Env>;
    using Connection=PlainTcpConnection;
    using NetworkServerCtx=PlainTcpServerContextT<Env>;
    using NetworkServer=PlainTcpServerT<Env>;

    static Result<common::SharedPtr<NetworkServerCtx>> makeAndInitNetworkServer(
        lib::string_view name,
        common::SharedPtr<Env> env,
        const HATN_APP_NAMESPACE::BaseApp& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    );
};

template <typename EnvConfigT=EnvConfig,
         typename DispatcherT=ServiceDispatcher<typename EnvConfigT::Env>,
         typename AuthDispatcherT=AuthDispatcher<typename EnvConfigT::Env>
         >
using PlainTcpMicroServiceT=NetworkMicroService<PlainTcpMicroServiceConfig<typename EnvConfigT::Env>,
                                                  EnvConfigT,
                                                  DispatcherT,
                                                  AuthDispatcherT
                                                  >;

using PlainTcpMicroService=PlainTcpMicroServiceT<>;

using PlainTcpMicroServiceV=MicroServiceV<PlainTcpMicroService>;

template <typename EnvConfigT, typename DispatcherT,typename AuthDispatcherT>
struct PlainTcpMicroServiceBuilderTraits
{
    static auto makeMicroService(std::string name, std::shared_ptr<DispatcherT> dispatcher, std::shared_ptr<AuthDispatcherT> authDispatcher)
    {
        return std::make_shared<MicroServiceV<PlainTcpMicroServiceT<EnvConfigT,DispatcherT,AuthDispatcherT>>>(
                std::move(name),
                std::move(dispatcher),
                std::move(authDispatcher)
            );
    }
};

template <typename EnvConfigT=EnvConfig,
         typename DispatcherT=ServiceDispatcher<typename EnvConfigT::Env>,
         typename AuthDispatcherT=AuthDispatcher<typename EnvConfigT::Env>
         >
using PlainTcpMicroServiceBuilderT=MicroServiceBuilder<PlainTcpMicroServiceBuilderTraits<EnvConfigT,DispatcherT,AuthDispatcherT>,
                                                         DispatcherT,AuthDispatcherT
                                                         >;
using PlainTcpMicroServiceBuilder=PlainTcpMicroServiceBuilderT<>;

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIPLAINTCPMICROSERVICE_H
