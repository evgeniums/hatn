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
#include <hatn/api/server/serverservice.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename EnvT=BasicEnv, typename RequestT=Request<EnvT>>
class ServiceRouterTraits
{
    public:

        using Env=EnvT;
        using Request=RequestT;

        void route(
                common::SharedPtr<RequestContext<Request>> reqCtx,
                RouteCb<Request> cb
            )
        {
            auto& req=reqCtx->template get<Request>();

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

        void registerService(Service service, RouteFh<Request> handler)
        {
            m_routes[std::move(service)]=std::move(handler);
        }

        void registerLocalService(std::shared_ptr<ServerService<Request>> localService)
        {
            auto handler=[localService](common::SharedPtr<RequestContext<RequestT>> request, RouteCb<RequestT> cb)
            {
                localService->handleRequest(std::move(request),std::move(cb));
            };
            m_routes[Service{localService->name(),localService->version()}]=std::move(handler);
        }

    private:

        common::FlatMap<Service,RouteFh<Request>,std::less<WithNameAndVersion<protocol::ServiceNameLengthMax>>> m_routes;
};

template <typename EnvT=BasicEnv, typename RequestT=Request<EnvT>>
class ServiceRouter : public RequestRouter<ServiceRouterTraits<EnvT,RequestT>,EnvT,RequestT>
{
    public:

        using Env=EnvT;
        using Request=RequestT;

        using RequestRouter<ServiceRouterTraits<Env,Request>,Env,Request>::RequestRouter;

        void registerService(Service service, RouteFh<Request> handler)
        {
            this->traits().registerService(std::move(service),std::move(handler));
        }

        void registerLocalService(std::shared_ptr<ServerService<Request>> localService)
        {
            this->traits().registerLocalService(std::move(localService));
        }

        void registerLocalServices(std::initializer_list<std::shared_ptr<ServerService<Request>>> localServices)
        {
            for (auto&& it: localServices)
            {
                this->traits().registerLocalService(it);
            }
        }
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVICEROUTER_H
