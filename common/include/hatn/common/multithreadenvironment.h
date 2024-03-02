/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/*
    
*/
/** @file common/multithreadenvironment.h
 *
 *     Environment with pools of categorized threads.
 *
 */
/****************************************************************************/

#ifndef HATNMULTITHREADENVIRONMENT_H
#define HATNMULTITHREADENVIRONMENT_H

#include <hatn/common/common.h>
#include <hatn/common/environment.h>
#include <hatn/common/threadcategoriespool.h>
#include <hatn/common/thread.h>
#include <hatn/common/threadqueueinterface.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Multi thread environment with pools of categorized threads
class MultiThreadEnvironment : public Environment
{
    public:

        //! Pool of basic threads
        inline ThreadCategoriesPool<Thread>& threadPool() noexcept
        {
            return m_threadPool;
        }

        //! Pool of threads with queues of plain tasks
        inline TaskThreadCategoriesPool& taskThreadPool() noexcept
        {
            return m_taskThreadPool;
        }

        //! Pool of threads with queues of tasks with contexts
        TaskWithContextThreadCategoriesPool& taskWithContextThreadPool() noexcept
        {
            return m_taskWithContextThreadPool;
        }

    private:

        ThreadCategoriesPool<Thread> m_threadPool;
        TaskThreadCategoriesPool m_taskThreadPool;
        TaskWithContextThreadCategoriesPool m_taskWithContextThreadPool;
};

HATN_COMMON_NAMESPACE_END

#endif // HATNMULTITHREADENVIRONMENT_H
