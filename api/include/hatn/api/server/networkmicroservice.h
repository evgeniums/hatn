/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/networkmicroservice.h
  *
  */

/****************************************************************************/

#ifndef HATNAPINETWORKMICROSERVICE_H
#define HATNAPINETWORKMICROSERVICE_H

#include <hatn/common/sharedptr.h>

#include <hatn/app/baseapp.h>

#include <hatn/api/api.h>
#include <hatn/api/server/connectionsstore.h>
#include <hatn/api/server/server.h>

#include <hatn/api/server/microservice.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename ConnectionCtxT,
         typename ConnectionT,
         typename NetworkServerCtxT,
         typename NetworkServerT
         >
struct NetworkMicroServiceConfig
{
    using ConnectionCtx=ConnectionCtxT;
    using Connection=ConnectionT;
    using NetworkServerCtx=NetworkServerCtxT;
    using NetworkServer=NetworkServerT;

    static common::SharedPtr<NetworkServerCtx> makeNetworkServer();
};

template <typename NetworkMicroserviceConfig,
         typename EnvT,
         typename DispatcherT=ServiceDispatcher<EnvT>,
         typename AuthDispatcherT=AuthDispatcher<EnvT>
         >
class NetworkMicroServiceTraits
{
    public:

        using Env=EnvT;

        using Dispatcher=DispatcherT;
        using AuthDispatcher=AuthDispatcherT;

        using ConnectionCtx=typename NetworkMicroserviceConfig::ConnectionCtx;
        using Connection=typename NetworkMicroserviceConfig::Connection;
        using ConnectionsStore=ConnectionsStore<ConnectionCtx,Connection>;
        using NetworkServerCtx=typename NetworkMicroserviceConfig::ServerCtx;
        using NetworkServer=typename NetworkMicroserviceConfig::Server;

        using Server=server::Server<Dispatcher,ConnectionsStore,AuthDispatcher,Env>;
        using SeviceRouter=server::ServiceRouter<Env>;

        Error start(
            common::SharedPtr<Env> env,
            const HATN_APP_NAMESPACE::BaseApp& app,
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
        );

        void close();

        Server& server()
        {
            return *m_server;
        }

        const Server& server() const
        {
            return *m_server;
        }

    private:

        common::SharedPtr<NetworkServerCtx> m_networkServerCtx;
        std::shared_ptr<Server> m_server;
};

template <typename NetworkMicroserviceConfig,
         typename EnvConfigT,
         typename DispatcherT=ServiceDispatcher<typename EnvConfigT::Env>,
         typename AuthDispatcherT=AuthDispatcher<typename EnvConfigT::Env>
         >
class NetworkMicroService : public MicroServiceT<
                                    NetworkMicroServiceTraits<NetworkMicroserviceConfig,typename EnvConfigT::Env,DispatcherT,AuthDispatcherT>,
                                    EnvConfigT,
                                    DispatcherT,
                                    AuthDispatcherT
                                >
{
    public:

        using Base=MicroServiceT<
            NetworkMicroServiceTraits<NetworkMicroserviceConfig,typename EnvConfigT::Env,DispatcherT,AuthDispatcherT>,
            EnvConfigT,
            DispatcherT,
            AuthDispatcherT
            >;

        using Base::Base;

        auto& server()
        {
            return this->traits().server();
        }

        const auto& server() const
        {
            return this->traits().server();
        }
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPINETWORKMICROSERVICE_H
