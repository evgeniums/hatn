/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/threadqueueinterfaceimpl.cpp
 *
 *     Interface of threads queue.
 *
 */

#ifndef HATNTHREADQUEUEINTERFACEIMPL_H
#define HATNTHREADQUEUEINTERFACEIMPL_H

#include <hatn/common/threadqueueinterface.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace detail
{
    template <typename TaskType>
    struct ThreadQueueInterfaceTraits
    {
        thread_local static ThreadQueueInterface<TaskType>* CurrentThreadInterface;
    };
    template <typename TaskType>
    thread_local ThreadQueueInterface<TaskType>* ThreadQueueInterfaceTraits<TaskType>::CurrentThreadInterface=nullptr;
}

//---------------------------------------------------------------
template <typename TaskType>
void ThreadQueueInterface<TaskType>::setCurrentThreadInterface(ThreadQueueInterface<TaskType> *interface)
{
    detail::ThreadQueueInterfaceTraits<TaskType>::CurrentThreadInterface=interface;
    Assert(detail::ThreadQueueInterfaceTraits<TaskType>::CurrentThreadInterface==interface,"Failed to set current thread queue interface");
}

//---------------------------------------------------------------
template <typename TaskType>
ThreadQueueInterface<TaskType>* ThreadQueueInterface<TaskType>::currentThreadInterface() noexcept
{
    return detail::ThreadQueueInterfaceTraits<TaskType>::CurrentThreadInterface;
}

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END

#endif // HATNTHREADQUEUEINTERFACEIMPL_H
