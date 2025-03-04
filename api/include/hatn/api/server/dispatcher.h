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
#include <hatn/api/server/request.h>
#include <hatn/api/server/servicerouter.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename EnvT=SimpleEnv>
using DispatchCb=RouteCb<EnvT>;

template <typename Traits, typename EnvT=SimpleEnv>
class Dispatcher : public common::WithTraits<Traits>,
                   public std::enable_shared_from_this<Dispatcher<Traits,EnvT>>
{
    public:

        using Env=EnvT;
        using Self=Dispatcher<Traits,EnvT>;

        template <typename ...TraitsArgs>
        Dispatcher(
                common::Thread* thread,
                TraitsArgs&&... traitsArgs
            )
            : Traits(this,std::forward<TraitsArgs>(traitsArgs)...),
              m_thread(thread)
        {}

        void dispatch(
                common::SharedPtr<RequestContext<Env>> reqCtx,
                DispatchCb<Env> cb
            )
        {
            m_thread->execAsync(
                [reqCtx{std::move(reqCtx)},cb{std::move(cb)},this]()
                {
                    auto cb1=[cb{std::move(cb)}](common::SharedPtr<RequestContext<Env>> reqCtx)
                    {
                        auto& req=reqCtx->template get<Request>();
                        if (!req.routed)
                        {
                            //! @todo report error that no route is found
                        }
                        else if (!req.response.unit.field(response::status).isSet())
                        {
                            //! @todo report internal error that response is not set
                        }
                        cb(std::move(reqCtx));
                    };
                    this->traits().dispatch(std::move(reqCtx),std::move(cb1));
                }
            );
        }

        common::Thread* thread() const
        {
            return m_thread;
        }

    private:

        common::Thread* m_thread;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIDISPATCHER_H
