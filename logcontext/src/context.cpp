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
  *      Contains definition log context.
  *
  */

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/context.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

template class HATN_LOGCONTEXT_EXPORT ContextT<DefaultConfig,ThreadCursorData>;
template class HATN_LOGCONTEXT_EXPORT ContextWrapperT<Context>;

HATN_LOGCONTEXT_NAMESPACE_END

HATN_TASK_CONTEXT_DEFINE(HATN_LOGCONTEXT_NAMESPACE::Context)