/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
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
