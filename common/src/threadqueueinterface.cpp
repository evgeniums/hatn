/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/threadqueueinterface.cpp
 *
 *     Interface of threads queue.
 *
 */

#include <hatn/common/threadqueueinterfaceimpl.h>

HATN_COMMON_NAMESPACE_BEGIN

#ifndef _THREAD_QUEUE_INTERFACE
#define _THREAD_QUEUE_INTERFACE
template class HATN_COMMON_EXPORT ThreadQueueInterface<Task>;
template class HATN_COMMON_EXPORT ThreadQueueInterface<TaskWithContext>;
#endif

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
