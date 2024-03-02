/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/threadwithqueue.сpp
  *
  *     Dracosha thread with queue
  */

#include <iostream>

#include <hatn/common/types.h>

#include <hatn/common/mutexqueue.h>
#include <hatn/common/mpscqueue.h>

#include <hatn/common/threadcategoriespoolimpl.h>
#include <hatn/common/threadwithqueue.h>
#include <hatn/common/threadwithqueueimpl.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------
template class HATN_COMMON_EXPORT ThreadWithQueue<Task>;
template class HATN_COMMON_EXPORT ThreadWithQueue<TaskWithContext>;

template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadWithQueue<Task>>;
template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadWithQueue<TaskWithContext>>;

template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadQueueInterface<Task>>;
template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadQueueInterface<TaskWithContext>>;

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
