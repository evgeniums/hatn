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
#include <hatn/api/server/request.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

class ServiceRouterTraits
{
    public:

        void route(
                common::SharedPtr<RequestContext> request,
                MessageRouteCb cb
            )
        {
            auto it=m_routes.find(*request);
            if (it==m_routes.end())
            {
                cb(std::move(request),{});
                return;
            }
            it->second(std::move(request),std::move(cb));
        }

        void addService(Service service, MessageRouteFh handler)
        {
            m_routes[std::move(service)]=std::move(handler);
        }

    private:

        common::FlatMap<Service,MessageRouteFh,std::less<Service>> m_routes;
};

class ServiceRouter : public RequestsRouter<ServiceRouterTraits>
{
    public:

        using RequestsRouter<ServiceRouterTraits>::RequestsRouter;

        void addService(Service service, MessageRouteFh handler)
        {
            this->traits().addService(std::move(service),std::move(handler));
        }
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVICEROUTER_H
