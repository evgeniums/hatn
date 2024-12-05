/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/ipp/threadcategoriespool.ipp
 *
 *     Pool of threads mapped to categories.
 *
 */
/****************************************************************************/

#ifndef HATNTHREADQUEUECATEGORIESPOOL_IPP
#define HATNTHREADQUEUECATEGORIESPOOL_IPP

#include <hatn/common/threadcategoriespool.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename ThreadT>
std::shared_ptr<ThreadT> ThreadCategoriesPool<ThreadT>::threadShared(
            const ThreadCategory& category,
            int priority
        ) const noexcept
{
    std::shared_ptr<ThreadT> thread;

    auto it=m_threads.find(category);
    if (it!=m_threads.end())
    {
        auto it1=it->second.find(priority);
        if (it1==it->second.end())
        {
            it1=it->second.begin();
            if (it1!=it->second.end())
            {
                thread=it1->second;
            }
        }
        else
        {
            thread=it1->second;
        }
    }
    if (!thread)
    {
        thread=m_defaultThread;
    }

    return thread;
}

template <typename ThreadT>
void ThreadCategoriesPool<ThreadT>::insertThread(
    const std::shared_ptr<ThreadT>& thread,
    const ThreadCategory& category,
    int priority
)
{
    std::map<int,std::shared_ptr<ThreadT>> priorityThreads;
    auto it=m_threads.find(category);
    if (it!=m_threads.end())
    {
        priorityThreads=it->second;
    }
    priorityThreads[priority]=thread;
    m_threads[category]=priorityThreads;
}

HATN_COMMON_NAMESPACE_END

#endif // HATNTHREADQUEUECATEGORIESPOOL_IPP
