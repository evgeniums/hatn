/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/authdispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIAUTHDISPATCHER_H
#define HATNAPIAUTHDISPATCHER_H

#include <functional>

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

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename EnvT=SimpleEnv>
using DispatchCb=std::function<void (common::SharedPtr<RequestContext<EnvT>> request, bool closeConnection)>;

template <typename Traits, typename EnvT=SimpleEnv>
class Dispatcher : public common::WithTraits<Traits>
{
    public:

        using Env=EnvT;

        template <typename ...TraitsArgs>
        Dispatcher(
                common::Thread* thread,
                TraitsArgs&&... traitsArgs
            )
            : Traits(this,std::forward<TraitsArgs>(traitsArgs)...),
              m_thread(thread)
        {}

        void dispatch(
                common::SharedPtr<RequestContext<Env>> request,
                DispatchCb<Env> cb
            )
        {
            // auto wCtx=common::toWeakPtr(ctx);
            // m_thread->execAsync(
            //     [wCtx{std::move(wCtx)},request{std::move(request)},cb{std::move(cb)},this]()
            //     {
            //         auto ctx=wCtx.lock();
            //         if (ctx)
            //         {
            //             this->traits().dispatch(std::move(wCtx),std::move(request),std::move(cb));
            //         }
            //     }
            // );
        }

        common::Thread* thread() const
        {
            return m_thread;
        }

    private:

        common::Thread* m_thread;
};

#if 0

template <typename ConnectionT, typename ConnectionContextT, typename ServiceRouter, typename TopicRouter>
class DispatcherDefaultTraits
{
    template <typename ConnectionT1, typename ConnectionContextT1>
    using TraitsT=DispatcherDefaultTraits<ConnectionT1,ConnectionContextT1,ServiceRouter,TopicRouter>;
    using erT=Dispatcher<ConnectionT,ConnectionContextT,TraitsT>;

    public:

        using ConnectionContext=ConnectionContextT;
        using Connection=ConnectionT;

        DispatcherDefaultTraits(
                erT* dispatcher
            ) : m_dispatcher(dispatcher)
        {}

        void dispatch(
            const common::SharedPtr<ConnectionContext>& ctx,
            common::SharedPtr<RequestContext> request,
            ServiceCb<ConnectionContext> cb
            )
        {
            //! @todo Implement default request router
        }

    private:

        template <typename ConnectionT1, typename ConnectionContextT1>
        using TraitsT=DispatcherDefaultTraits<ConnectionT1,ConnectionContextT1,ServiceRouter,TopicRouter>;

        Dispatcher<ConnectionT,ConnectionContextT,TraitsT>* m_dispatcher;

        std::shared_ptr<ServiceRouter> m_serviceRouter;
        std::shared_ptr<TopicRouter> m_topicRouter;
};

#endif

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTHDISPATCHER_H
