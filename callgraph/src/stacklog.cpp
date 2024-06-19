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

HATN_CALLGRAPH_NAMESPACE_BEGIN

namespace stacklog
{

template struct HATN_CALLGRAPH_EXPORT StackLogT<DefaultConfig,FnCursorData>;

} // namespace stacklog

HATN_CALLGRAPH_NAMESPACE_END
