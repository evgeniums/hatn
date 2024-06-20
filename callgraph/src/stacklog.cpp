/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/stacklog.сpp
  *
  *      Contains definition of some stack log types.
  *
  */

#include <hatn/callgraph/callgraph.h>
#include <hatn/callgraph/stacklog.h>

HATN_TASK_CONTEXT_DEFINE(HATN_CALLGRAPH_NAMESPACE::stacklog::StackLog)

HATN_CALLGRAPH_NAMESPACE_BEGIN

namespace stacklog
{

template class HATN_CALLGRAPH_EXPORT StackLogT<DefaultConfig,FnCursorData,ThreadCursorData>;
template class HATN_CALLGRAPH_EXPORT StackLogWrapperT<StackLog>;

} // namespace stacklog

HATN_CALLGRAPH_NAMESPACE_END
