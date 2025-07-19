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

    thread_local static int refCount=0;
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
        refCount--;
        TSInstance_Context=nullptr;

        // std::cout << "set log context nullptr: refCount=" << refCount << std::endl;

        return;
    }
    refCount++;
    // std::cout << "set log context: refCount=" << refCount << std::endl;
    TSInstance_Context=val->actualCtx();
}

void ThreadSubcontext<TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>::reset() noexcept
{
    TSInstance_Context=nullptr;
    refCount=0;
    // std::cout << "reset log context: refCount=" << refCount << std::endl;
}

HATN_COMMON_NAMESPACE_END
