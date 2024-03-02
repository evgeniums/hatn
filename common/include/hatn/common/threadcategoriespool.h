/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/threadcategoriespool.h
 *
 *     Pool of threads mapped to categories.
 *
 */
/****************************************************************************/

#ifndef HATNTHREADQUEUECATEGORIESPOOL_H
#define HATNTHREADQUEUECATEGORIESPOOL_H

#include <map>
#include <memory>

#include <hatn/common/common.h>
#include <hatn/common/threadcategory.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Pool of threads mapped to categories
template <typename ThreadType> class ThreadCategoriesPool
{
    public:

        //! Ctor
        ThreadCategoriesPool() =default;

        virtual ~ThreadCategoriesPool()=default;
        ThreadCategoriesPool(const ThreadCategoriesPool&)=delete;
        ThreadCategoriesPool(ThreadCategoriesPool&&) =default;
        ThreadCategoriesPool& operator=(const ThreadCategoriesPool&)=delete;
        ThreadCategoriesPool& operator=(ThreadCategoriesPool&&) =default;

        /**
         * @brief Get thread queue interface for category
         * @param category Category of the thread
         * @param priority Priority of the thread
         * @return Thread
         *
         * If no such priority in the category exists then will be used a thread with the lowest priority.
         * If no such category exists then will be returned default thread.
         */
        std::shared_ptr<ThreadType> threadShared(
            const ThreadCategory& category=ThreadCategory(),
            int priority=0
        ) const noexcept;

        /**
         * @brief Get thread queue interface for category
         * @param category Category of the thread
         * @param priority Priority of the thread
         * @return Thread
         *
         * If no such priority in the category exists then will be used a thread with the lowest priority.
         * If no such category exists then will be returned default thread.
         */
        inline ThreadType* thread(
            const ThreadCategory& category=ThreadCategory(),
            int priority=0
        ) const noexcept
        {
            return threadShared(category,priority).get();
        }

        inline std::shared_ptr<ThreadType> threadShared(
                    const ThreadCategoryAndPriority& categoryAndPriority
                ) noexcept
        {
            return threadShared(categoryAndPriority.category(),categoryAndPriority.priority());
        }

        inline ThreadType* thread(
                    const ThreadCategoryAndPriority& categoryAndPriority
                ) noexcept
        {
            return thread(categoryAndPriority.category(),categoryAndPriority.priority());
        }

        //! Insert thread
        /**
         * @brief Insert thread to pool
         * @param thread Thread to insaert
         * @param category Category of the thread
         * @param priority Pritirity of the thread
         *
         * If thread with the same category/pritority exists in the pool then it will be overwritten
         *
         */
        void insertThread(
            const std::shared_ptr<ThreadType>& thread,
            const ThreadCategory& category=ThreadCategory(),
            int priority=0
        );

        //! Set fallback thread if no category/priority found
        inline void setDefaultThread(std::shared_ptr<ThreadType> thread) noexcept
        {
            m_defaultThread=std::move(thread);
        }

        //! Get fallback thread
        inline std::shared_ptr<ThreadType> defaultThread() const noexcept
        {
            return m_defaultThread;
        }

        //! Clear pool
        inline void clear() noexcept
        {
            m_threads.clear();
        }

    private:

        std::map<ThreadCategory,std::map<int,std::shared_ptr<ThreadType>>> m_threads;
        std::shared_ptr<ThreadType> m_defaultThread;
};

HATN_COMMON_NAMESPACE_END

#endif // HATNTHREADQUEUECATEGORIESPOOL_H
