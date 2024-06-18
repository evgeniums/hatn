/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/ipp/threadqueueinterface.ipp
 *
 *     Interface of threads queue.
 *
 */

#ifndef HATNTHREADQUEUEINTERFACEL_IPP
#define HATNTHREADQUEUEINTERFACEL_IPP

#include <hatn/common/threadqueueinterface.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace detail
{
    template <typename TaskT, template <typename> class Traits>
    struct ThreadQueueInterfaceWrapper
    {
        thread_local static ThreadQueueInterface<TaskT,Traits>* current;
    };
    template <typename TaskT, template <typename> class Traits>
    thread_local ThreadQueueInterface<TaskT,Traits>* ThreadQueueInterfaceWrapper<TaskT,Traits>::current=nullptr;
}

//---------------------------------------------------------------
template <typename TaskT, template <typename> class Traits>
void ThreadQueueInterface<TaskT,Traits>::setCurrent(ThreadQueueInterface<TaskT,Traits> *interface)
{
    detail::ThreadQueueInterfaceWrapper<TaskT,Traits>::current=interface;
}

//---------------------------------------------------------------
template <typename TaskT, template <typename> class Traits>
ThreadQueueInterface<TaskT,Traits>* ThreadQueueInterface<TaskT,Traits>::current() noexcept
{
    return detail::ThreadQueueInterfaceWrapper<TaskT,Traits>::current;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNTHREADQUEUEINTERFACEL_IPP
