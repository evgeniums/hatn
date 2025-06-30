/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/makeasynccallback.h
  *
  */

/****************************************************************************/

#ifndef HATNMAKEASYNCCALLBACK_H
#define HATNMAKEASYNCCALLBACK_H

#include <hatn/common/threadwithqueue.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/contextlogger.h>

HATN_NAMESPACE_BEGIN

template <typename CallbackT>
struct postAsyncCallbackT
{
    template <typename ContextT, typename ...Args>
    void operator()(common::SharedPtr<ContextT> ctx, Args&&... args) const
    {
        auto ts=hana::make_tuple(ctx,std::forward<Args>(args)...);
        common::postAsyncTask(
            thread,
            ctx,
            [ts{std::move(ts)},cb{callback}](const common::SharedPtr<common::TaskContext>&)
            {
                hana::unpack(std::move(ts),std::move(cb));
            }
        );
    }

    template <typename ContextT, typename ...Args>
    void operator()(const char* startScopeName, ContextT ctx, Args&&... args) const
    {
        auto ts=hana::make_tuple(ctx,std::forward<Args>(args)...);
        common::postAsyncTask(
            thread,
            ctx,
            [ts{std::move(ts)},cb{callback},ctx,startScopeName](const common::SharedPtr<common::TaskContext>&)
            {
                ctx->onAsyncHandlerEnter();

                {
                    HATN_CTX_SCOPE(startScopeName)
                    hana::unpack(std::move(ts),std::move(cb));
                }

                ctx->onAsyncHandlerExit();
            }
        );
    }

    postAsyncCallbackT(
        CallbackT callback={},
        common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current()
        ) : callback(std::move(callback)),thread(thread)
    {}

    CallbackT callback;
    common::ThreadQWithTaskContext* thread;
};

struct makeAsyncCallbackT
{
    template <typename CallbackT>
    auto operator()(
            CallbackT callback,
            common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current()
        ) const
    {
        return postAsyncCallbackT<CallbackT>{std::move(callback),thread};
    }
};
constexpr makeAsyncCallbackT makeAsyncCallback{};

HATN_NAMESPACE_END

#endif // HATNMAKEASYNCCALLBACK_H
