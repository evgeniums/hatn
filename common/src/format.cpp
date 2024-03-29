/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/format.cpp
 *
 *     String formating wrappers to use with fmtlib
 *
 */

#include <hatn/common/threadwithqueueimpl.h>
#include <hatn/common/threadqueueinterfaceimpl.h>

#include <hatn/common/format.h>

HATN_COMMON_NAMESPACE_BEGIN

template class HATN_COMMON_EXPORT ThreadQueueInterface<TaskInlineContext<FmtAllocatedBufferChar>>;
template class HATN_COMMON_EXPORT ThreadWithQueue<TaskInlineContext<FmtAllocatedBufferChar>>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
