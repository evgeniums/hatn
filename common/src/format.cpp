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

#include <hatn/common/ipp/threadq.ipp>
#include <hatn/common/ipp/threadcategoriespool.ipp>
#include <hatn/common/ipp/threadwithqueue.ipp>

#include <hatn/common/format.h>

HATN_COMMON_NAMESPACE_BEGIN

template class HATN_COMMON_EXPORT ThreadQ<TaskInlineContext<FmtAllocatedBufferChar>,ThreadWithQueueTraits>;
template class HATN_COMMON_EXPORT ThreadWithQueue<TaskInlineContext<FmtAllocatedBufferChar>>;
template class HATN_COMMON_EXPORT ThreadWithQueueTraits<TaskInlineContext<FmtAllocatedBufferChar>>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
