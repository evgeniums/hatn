/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/ipp/threadq.ipp
 *
 *     Interface of threads queue.
 *
 */

#ifndef HATNTHREADQUEUEINTERFACEL_IPP
#define HATNTHREADQUEUEINTERFACEL_IPP

#include <hatn/common/threadq.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace detail
{
    template <typename TaskT, template <typename> class Traits>
    struct ThreadQWrapper
    {
        thread_local static ThreadQ<TaskT,Traits>* current;
    };
    template <typename TaskT, template <typename> class Traits>
    thread_local ThreadQ<TaskT,Traits>* ThreadQWrapper<TaskT,Traits>::current=nullptr;
}

//---------------------------------------------------------------
template <typename TaskT, template <typename> class Traits>
void ThreadQ<TaskT,Traits>::setCurrent(ThreadQ<TaskT,Traits> *interface)
{
    detail::ThreadQWrapper<TaskT,Traits>::current=interface;
}

//---------------------------------------------------------------
template <typename TaskT, template <typename> class Traits>
ThreadQ<TaskT,Traits>* ThreadQ<TaskT,Traits>::current() noexcept
{
    return detail::ThreadQWrapper<TaskT,Traits>::current;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNTHREADQUEUEINTERFACEL_IPP
