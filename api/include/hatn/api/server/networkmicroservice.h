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

    template <typename EnvT>
    static Result<common::SharedPtr<NetworkServerCtx>> makeAndInitNetworkServer(
        lib::string_view name,
        common::SharedPtr<EnvT> env,
        const HATN_APP_NAMESPACE::BaseApp& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    );
};

template <typename MicroServiceT,
         typename EnvT,
         typename DispatcherT,
         typename AuthDispatcherT,
         typename NetworkMicroServiceConfigT>
class NetworkMicroServiceTraits
{    
    public:

        using Env=EnvT;

        using Dispatcher=DispatcherT;
        using AuthDispatcher=AuthDispatcherT;

        using ConnectionCtx=typename NetworkMicroServiceConfigT::ConnectionCtx;
        using Connection=typename NetworkMicroServiceConfigT::Connection;
        using ConnectionsStore=ConnectionsStore<ConnectionCtx,Connection>;
        using NetworkServerCtx=typename NetworkMicroServiceConfigT::NetworkServerCtx;
        using NetworkServer=typename NetworkMicroServiceConfigT::NetworkServer;

        using Server=server::Server<ConnectionsStore,Dispatcher,AuthDispatcher,Env>;

        NetworkMicroServiceTraits(MicroServiceT* microservice) : m_microservice(microservice)
        {}

        Error start(
            lib::string_view name,
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

        MicroServiceT* m_microservice;

        common::SharedPtr<NetworkServerCtx> m_networkServerCtx;
        std::shared_ptr<Server> m_server;
};

template <typename NetworkMicroServiceConfigT,
         typename EnvConfigT,
         typename DispatcherT=ServiceDispatcher<typename EnvConfigT::Env>,
         typename AuthDispatcherT=AuthDispatcher<typename EnvConfigT::Env>
         >
class NetworkMicroService : public MicroServiceT<
                                    NetworkMicroServiceTraits<
                                        NetworkMicroService<NetworkMicroServiceConfigT,EnvConfigT,DispatcherT,AuthDispatcherT>,
                                        typename EnvConfigT::Env,
                                        DispatcherT,
                                        AuthDispatcherT,
                                        NetworkMicroServiceConfigT
                                    >,
                                    EnvConfigT,
                                    DispatcherT,
                                    AuthDispatcherT
                                >
{
    public:

        using Base=MicroServiceT<
            NetworkMicroServiceTraits<
                NetworkMicroService<NetworkMicroServiceConfigT,EnvConfigT,DispatcherT,AuthDispatcherT>,
                typename EnvConfigT::Env,
                DispatcherT,
                AuthDispatcherT,
                NetworkMicroServiceConfigT
                >,
            EnvConfigT,
            DispatcherT,
            AuthDispatcherT
            >;

        template <typename ...Args>
        NetworkMicroService(Args&& ...args) : Base(std::forward<Args>(args)...,this)
        {}

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
