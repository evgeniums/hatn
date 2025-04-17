/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/networkmicroservice.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPINETWORKMICROSERVICE_IPP
#define HATNAPINETWORKMICROSERVICE_IPP

#include <hatn/api/server/networkmicroservice.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

//---------------------------------------------------------------

template <typename MicroServiceT,
         typename EnvT,
         typename DispatcherT,
         typename AuthDispatcherT,
         typename NetworkMicroServiceConfigT>
Error NetworkMicroServiceTraits<MicroServiceT,EnvT,DispatcherT,AuthDispatcherT,NetworkMicroServiceConfigT>::start(
        lib::string_view name,
        common::SharedPtr<Env> env,
        const HATN_APP_NAMESPACE::App& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    )
{
    // make server
    auto connectionsStore=std::make_shared<ConnectionsStore>();
    m_server=std::make_shared<Server>(std::move(connectionsStore),m_microservice->dispatcher(),m_microservice->authDispatcher());

    // make and init network server
    auto networkServerCtx=NetworkMicroServiceConfigT::makeAndInitNetworkServer(name,env,app,configTree,configTreePath);
    if (networkServerCtx)
    {
        //! @todo Log error
        return networkServerCtx.takeError();
    }
    m_networkServerCtx=networkServerCtx.takeValue();
    auto& networkServer=m_networkServerCtx->template get<NetworkServer>();
    networkServer.setEnv(env);

    // set handler of new network connection
    auto onNewNetworkConnection=[server{std::weak_ptr{m_server}}](common::SharedPtr<ConnectionCtx> connectionCtx, const Error& ec, auto cb)
    {
        auto serv=server.lock();
        if (!serv)
        {
            return;
        }

        if (ec)
        {
            //! @todo Log that connections broken?
            return;
        }

        auto& connection=connectionCtx->template get<Connection>();
        serv->handleNewConnection(connectionCtx,connection,cb);
    };
    networkServer.setConnectionHandler(onNewNetworkConnection);

    // run network server
    auto ec=networkServer.run(m_networkServerCtx);
    if (ec)
    {
        //! @todo Log error
        return ec;
    }

    // done
    return OK;
}

//---------------------------------------------------------------

template <typename MicroServiceT,
         typename EnvT,
         typename DispatcherT,
         typename AuthDispatcherT,
         typename NetworkMicroServiceConfigT>
void NetworkMicroServiceTraits<MicroServiceT,EnvT,DispatcherT,AuthDispatcherT,NetworkMicroServiceConfigT>::close()
{
    if (m_server)
    {
        m_server->close();
    }

    if (m_networkServerCtx)
    {
        auto& networkServer=m_networkServerCtx->template get<NetworkServer>();
        auto ec=networkServer.close();
        //! @todo Log error?
        std::ignore=ec;
    }
}

//---------------------------------------------------------------

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPINETWORKMICROSERVICE_IPP
