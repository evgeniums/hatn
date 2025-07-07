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

#include <hatn/logcontext/postasync.h>
#include <hatn/logcontext/makeasynccallback.h>

#include <hatn/api/api.h>
#include <hatn/api/service.h>
#include <hatn/api/server/env.h>
#include <hatn/api/server/serverrequest.h>
#include <hatn/api/server/servicerouter.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename RequestT=Request<>>
using DispatchCb=RouteCb<RequestT>;

struct DispatcherScopes
{
    constexpr static const char* dispatch="dispatch";
    constexpr static const char* dispatchCb="dispatch::cb";
};

template <typename Traits, typename EnvT=BasicEnv, typename RequestT=Request<EnvT>, typename Scopes=DispatcherScopes>
class Dispatcher : public common::WithTraits<Traits>,
                   public std::enable_shared_from_this<Dispatcher<Traits,EnvT,RequestT,Scopes>>
{
    public:

        using Env=EnvT;
        using Request=RequestT;
        using Self=Dispatcher<Traits,EnvT,Request>;

        using common::WithTraits<Traits>::WithTraits;

        void dispatch(
                common::SharedPtr<RequestContext<Request>> reqCtx,
                DispatchCb<Request> callback
            )
        {
            auto cb=makeAsyncCallback(std::move(callback));
            auto& req=reqCtx->template get<Request>();
            auto* reqPtr=&req;
            req.routed=false;
            auto self=this->shared_from_this();
            postAsync(
                Scopes::dispatch,
                reqPtr->thread(),
                std::move(reqCtx),
                [cb{std::move(cb)},this,self=std::move(self)](auto reqCtx)
                {
                    auto cb1=[cb{std::move(cb)}](common::SharedPtr<RequestContext<Request>> reqCtx)
                    {
                        auto& req=reqCtx->template get<Request>();
                        if (!req.routed)
                        {
                            // report error that no route is found
                            req.response.setStatus(protocol::ResponseStatus::RoutingError);
                        }
                        cb(Scopes::dispatchCb,std::move(reqCtx));
                    };
                    this->traits().dispatch(std::move(reqCtx),std::move(cb1));
                }
            );
        }
};

template <typename DispatcherT>
class DispatchersStore
{
    public:

        using Dispatcher=DispatcherT;

        void registerDispatcher(
            std::string name,
            std::shared_ptr<Dispatcher> dispatcher
            )
        {
            m_dispatchers[std::move(name)]=std::move(dispatcher);
        }

        std::shared_ptr<Dispatcher> dispatcher(
                const std::string& name
            ) const
        {
            auto it=m_dispatchers.find(name);
            if (it!=m_dispatchers.end())
            {
                return it->second;
            }
            return std::shared_ptr<Dispatcher>{};
        }


    private:

        std::map<std::string,std::shared_ptr<Dispatcher>> m_dispatchers;
};


} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIDISPATCHER_H
