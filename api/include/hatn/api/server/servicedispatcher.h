/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/servicedispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVICEDISPATCHER_H
#define HATNAPISERVICEDISPATCHER_H

#include <hatn/api/server/dispatcher.h>
#include <hatn/api/server/servicerouter.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename EnvT=BasicEnv, typename RequestT=Request<EnvT>>
class ServiceDispatcherTraits
{
    public:

        using Env=EnvT;
        using Request=RequestT;
        using Self=ServiceDispatcherTraits<Env,Request>;
        using DispatcherType=Dispatcher<Self,Env,Request>;

        ServiceDispatcherTraits(
                std::shared_ptr<ServiceRouter<Env,Request>> serviceRouter
            ) : m_serviceRouter(std::move(serviceRouter))
        {}

        void dispatch(
                common::SharedPtr<RequestContext<Request>> reqCtx,
                DispatchCb<Request> cb
            )
        {
            m_serviceRouter->route(
                std::move(reqCtx),
                std::move(cb)
            );
        }

    private:

        std::shared_ptr<ServiceRouter<Env,Request>> m_serviceRouter;
};

template <typename EnvT=BasicEnv, typename RequestT=Request<EnvT>>
using ServiceDispatcher=Dispatcher<ServiceDispatcherTraits<EnvT,RequestT>,EnvT,RequestT>;

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVICEDISPATCHER_H
