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

#ifndef HATNAPISERVICEROUTER_H
#define HATNAPISERVICEROUTER_H

#include <hatn/common/objecttraits.h>
#include <hatn/common/error.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/allocatoronstack.h>
#include <hatn/common/thread.h>
#include <hatn/common/flatmap.h>

#include <hatn/api/api.h>
#include <hatn/api/service.h>
#include <hatn/api/server/requestrouter.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename EnvT=SimpleEnv>
class ServiceRouterTraits
{
    public:

        using Env=EnvT;

        void route(
                common::SharedPtr<RequestContext<Env>> reqCtx,
                RouteCb<EnvT> cb
            )
        {
            auto& req=reqCtx->template get<Request<Env>>();

            auto it=m_routes.find(req.serviceNameAndVersion());
            if (it==m_routes.end())
            {
                req.routed=false;
                cb(std::move(reqCtx));
                return;
            }
            req.routed=true;
            it->second(std::move(reqCtx),std::move(cb));
        }

        void registerService(Service service, RouteFh<Env> handler)
        {
            m_routes[std::move(service)]=std::move(handler);
        }

    private:

        common::FlatMap<Service,RouteFh<EnvT>,std::less<WithNameAndVersion<ServiceNameLengthMax>>> m_routes;
};

template <typename EnvT=SimpleEnv>
class ServiceRouter : public RequestRouter<ServiceRouterTraits<EnvT>>
{
    public:

        using Env=EnvT;

        using RequestRouter<ServiceRouterTraits<Env>>::RequestRouter;

        void registerService(Service service, RouteFh<Env> handler)
        {
            this->traits().registerService(std::move(service),std::move(handler));
        }
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVICEROUTER_H
