/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/postasync.h
  *
  */

/****************************************************************************/

#ifndef HATNPOSTASYNC_H
#define HATNPOSTASYNC_H

#include <hatn/common/threadwithqueue.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/contextlogger.h>

HATN_NAMESPACE_BEGIN

struct postAsyncT
{
    template <typename HandlerT, typename CallbackT>
    void operator ()(
                    const char* startScopeName,
                    common::ThreadQWithTaskContext* thread,
                    common::SharedPtr<common::TaskContext> ctx,
                    HandlerT handler,
                    CallbackT callback
                    ) const
    {
        auto cb=[startScopeName,callback{std::move(callback)}](auto ctx,auto&&... args) mutable
        {
            auto ctxPtr=ctx.get();
            ctxPtr->onAsyncHandlerEnter();

            {
                HATN_CTX_SCOPE(startScopeName)
                callback(std::move(ctx),std::forward<decltype(args)>(args)...);
            }

            ctxPtr->onAsyncHandlerExit();
        };

        common::postAsyncTask(
            thread,
            ctx,
            [startScopeName,handler=std::move(handler)](auto ctx, auto cb)
            {
                auto ctxPtr=ctx.get();
                ctxPtr->onAsyncHandlerEnter();

                {
                    HATN_CTX_SCOPE(startScopeName)
                    handler(std::move(ctx),std::move(cb));
                }

                ctxPtr->onAsyncHandlerExit();
            },
            std::move(cb)
        );
    }

    template <typename HandlerT, typename ContextT>
    void operator ()(
                    const char* startScopeName,
                    common::ThreadQWithTaskContext* thread,
                    ContextT ctx,
                    HandlerT handler
                    ) const
    {
        common::postAsyncTask(
            thread,
            ctx,
            [startScopeName,handler=std::move(handler)](auto ctx)
            {
                auto ctxPtr=ctx.get();
                ctxPtr->onAsyncHandlerEnter();

                {
                    HATN_CTX_SCOPE(startScopeName)
                    handler(std::move(ctx));
                }

                ctxPtr->onAsyncHandlerExit();
            }
        );
    }
};
constexpr postAsyncT postAsync{};

HATN_NAMESPACE_END

#endif // HATNPOSTASYNC_H
