/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/dispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIDISPATCHER_H
#define HATNAPIDISPATCHER_H

#include <hatn/common/objecttraits.h>
#include <hatn/common/error.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/allocatoronstack.h>
#include <hatn/common/thread.h>
#include <hatn/common/flatmap.h>

#include <hatn/api/api.h>
#include <hatn/api/service.h>
#include <hatn/api/server/env.h>
#include <hatn/api/server/serverrequest.h>
#include <hatn/api/server/servicerouter.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename RequestT=Request<>>
using DispatchCb=RouteCb<RequestT>;

template <typename Traits, typename EnvT=BasicEnv, typename RequestT=Request<EnvT>>
class Dispatcher : public common::WithTraits<Traits>,
                   public std::enable_shared_from_this<Dispatcher<Traits,EnvT,RequestT>>
{
    public:

        using Env=EnvT;
        using Request=RequestT;
        using Self=Dispatcher<Traits,EnvT,Request>;

        template <typename ...TraitsArgs>
        Dispatcher(
                TraitsArgs&&... traitsArgs
            )
            : common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...)
        {}

        void dispatch(
                common::SharedPtr<RequestContext<Request>> reqCtx,
                DispatchCb<Request> cb
            )
        {
            auto& req=reqCtx->template get<Request>();
            req.thread()->execAsync(
                [reqCtx{std::move(reqCtx)},cb{std::move(cb)},this]()
                {
                    auto cb1=[cb{std::move(cb)}](common::SharedPtr<RequestContext<Request>> reqCtx)
                    {
                        auto& req=reqCtx->template get<Request>();
                        if (!req.routed)
                        {
                            // report error that no route is found
                            req.response.setStatus(protocol::ResponseStatus::RoutingError);
                        }
                        cb(std::move(reqCtx));
                    };
                    this->traits().dispatch(std::move(reqCtx),std::move(cb1));
                }
            );
        }
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIDISPATCHER_H
