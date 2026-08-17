/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/context.сpp
  *
  *  Contains definitions of log context.
  *
  */

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/context.h>

namespace {
    thread_local static HATN_LOGCONTEXT_NAMESPACE::Context* TSInstance_Context{nullptr};
    thread_local static HATN_LOGCONTEXT_NAMESPACE::Context* TSFallback_Context{nullptr};

    // Save/restore stack of the current log context of this thread.
    //
    // setValue(ctx)/setValue(nullptr) pairs nest routinely: a thread queue task wraps its
    // handler in beforeThreadProcessing()/afterThreadProcessing(), and the handler itself calls
    // onAsyncHandlerEnter()/onAsyncHandlerExit() again (postasync.h, makeasynccallback.h,
    // api client dequeue). Clearing the slot to nullptr on the inner exit - as this used to do -
    // left HATN_CTX_CURRENT() pointing at the thread's long-lived fallback context for the rest
    // of the outer handler, so every macro that re-resolves the current context
    // (HATN_CTX_SCOPE_ERROR/_LOCK/_UNLOCK, HATN_CTX_STACK_BARRIER_*, HATN_CTX_SCOPE_PUSH) acted
    // on a different object than the scope guard it belonged to, while the RAII guard - which
    // captures its Context* once - kept enter/leave on the intended one.
    //
    // Fixed size and no allocation so that setValue() stays noexcept. Nesting deeper than
    // MaxCtxStackDepth keeps counting (so the pairing never slips) but loses the saved pointer
    // and restores nullptr, i.e. degrades to the previous behaviour instead of corrupting.
    //
    // This assumes setValue(ctx) and setValue(nullptr) are properly paired on a thread, which
    // is how before/afterThreadProcessing() and onAsyncHandlerEnter/Exit() are written. An
    // unpaired exit is harmless (it restores an outer context, or nullptr once the stack is
    // empty); an unpaired enter leaks a level and can later restore a context that has already
    // been destroyed - but that same enter already leaves the current pointer dangling anyway,
    // so it is not a new failure mode.
    constexpr static const size_t MaxCtxStackDepth=64;
    thread_local static HATN_LOGCONTEXT_NAMESPACE::Context* TSStack_Context[MaxCtxStackDepth]{};
    thread_local static size_t TSStack_Depth=0;
}

HATN_LOGCONTEXT_NAMESPACE_BEGIN

void ThreadLocalFallbackContext::reset(Context* val) noexcept
{
    TSFallback_Context=val;
}

HATN_LOGCONTEXT_NAMESPACE_END

HATN_COMMON_NAMESPACE_BEGIN

HATN_LOGCONTEXT_NAMESPACE::Context* ThreadSubcontext<TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>::value() noexcept
{
    if (TSInstance_Context==nullptr)
    {
        return TSFallback_Context;
    }
    return TSInstance_Context;
}

void ThreadSubcontext<TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>::setValue(HATN_LOGCONTEXT_NAMESPACE::Context* val) noexcept
{
    if (val==nullptr)
    {
        // restore the context that was current before the matching setValue(ctx)
        if (TSStack_Depth>0)
        {
            TSStack_Depth--;
            TSInstance_Context=(TSStack_Depth<MaxCtxStackDepth)?TSStack_Context[TSStack_Depth]:nullptr;
        }
        else
        {
            TSInstance_Context=nullptr;
        }
        return;
    }

    if (TSStack_Depth<MaxCtxStackDepth)
    {
        TSStack_Context[TSStack_Depth]=TSInstance_Context;
    }
    TSStack_Depth++;
    TSInstance_Context=val->actualCtx();
}

void ThreadSubcontext<TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>::reset() noexcept
{
    TSInstance_Context=nullptr;
    TSStack_Depth=0;
}

HATN_COMMON_NAMESPACE_END
