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

HATN_LOGCONTEXT_NAMESPACE_BEGIN

HATN_LOGCONTEXT_NAMESPACE_END

namespace {
    thread_local static HATN_LOGCONTEXT_NAMESPACE::Context* TSInstance_Context{nullptr};
}

HATN_COMMON_NAMESPACE_BEGIN

HATN_LOGCONTEXT_NAMESPACE::Context* ThreadSubcontext<TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>::value() noexcept
{
    return TSInstance_Context;
}

void ThreadSubcontext<TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>::setValue(HATN_LOGCONTEXT_NAMESPACE::Context* val) noexcept
{
    if (val==nullptr)
    {
        TSInstance_Context=nullptr;
        return;
    }
    TSInstance_Context=val->actualCtx();
}

void ThreadSubcontext<TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>::reset() noexcept
{
    TSInstance_Context=nullptr;
}

// template class ThreadSubcontext<TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>;

HATN_COMMON_NAMESPACE_END

// HATN_TASK_CONTEXT_DEFINE(HATN_LOGCONTEXT_NAMESPACE::Context,Context)
