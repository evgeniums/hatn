/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/threadwithqueue.сpp
  *
  *     hatn thread with queue
  */

#include <iostream>

#include <hatn/common/mutexqueue.h>
#include <hatn/common/mpscqueue.h>

#include <hatn/common/threadwithqueue.h>
#include <hatn/common/ipp/threadq.ipp>
#include <hatn/common/ipp/threadcategoriespool.ipp>
#include <hatn/common/ipp/threadwithqueue.ipp>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------

template class HATN_COMMON_EXPORT ThreadWithQueueTraits<Task>;
template class HATN_COMMON_EXPORT ThreadWithQueueTraits<TaskWithContext>;
template class HATN_COMMON_EXPORT ThreadWithQueue<Task>;
template class HATN_COMMON_EXPORT ThreadWithQueue<TaskWithContext>;

template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadWithQueue<Task>>;
template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadWithQueue<TaskWithContext>>;

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
