/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/sharedlocker.h
 *
 *     Shared locker which allows either multiple parallel shared accesses or only one exclusive access
 *
 */
/****************************************************************************/

#ifndef HATNSHAREDLOCKER_H
#define HATNSHAREDLOCKER_H

#include <limits>

#include <atomic>
#include <mutex>

#include <hatn/common/common.h>
#include <hatn/common/thread.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Shared locker that allows either multiple parallel shared accesses or only one exclusive access
class SharedLocker final
{
    public:

        //! Lock shared access within scope
        class HATN_COMMON_EXPORT SharedScope final
        {
            public:

                SharedScope(SharedLocker& locker) noexcept : l(locker)
                {
                    l.acquireShared();
                }
                ~SharedScope()
                {
                    l.releaseShared();
                }

                SharedScope(const SharedScope&)=delete;
                SharedScope(SharedScope&&) =delete;
                SharedScope& operator=(const SharedScope&)=delete;
                SharedScope& operator=(SharedScope&&) =delete;

            private:

                SharedLocker& l;
        };

        //! Lock exclusive access within scope
        class HATN_COMMON_EXPORT ExclusiveScope final
        {
            public:

                ExclusiveScope(SharedLocker& locker) noexcept : l(locker)
                {
                    l.acquireExclusive();
                }

                ~ExclusiveScope()
                {
                    l.releaseExclusive();
                }


                ExclusiveScope(const ExclusiveScope&)=delete;
                ExclusiveScope(ExclusiveScope&&)=delete;
                ExclusiveScope& operator=(const ExclusiveScope&)=delete;
                ExclusiveScope& operator=(ExclusiveScope&&)=delete;

            private:

                SharedLocker& l;
        };

        //! Ctor
        SharedLocker() noexcept : m_sharedCounter(0),m_lockThread(this)
        {}

        ~SharedLocker()=default;
        SharedLocker(const SharedLocker&)=delete;
        SharedLocker(SharedLocker&&)=delete;
        SharedLocker& operator=(const SharedLocker&)=delete;
        SharedLocker& operator=(SharedLocker&&)=delete;

        //! Acquire locker for shared access
        inline void acquireShared()
        {
            if (m_sharedCounter.fetch_add(1,std::memory_order_acquire)>=0)
            {
                //acquired
                return;
            }

            // acquire later after other thread unlocks the mutex
            auto currentThread=Thread::currentThread();
            Assert(m_lockThread!=currentThread,"Recursive mutex lock in SharedLocker::acquireShared()");
            m_exclusiveMutex.lock();
            m_lockThread=currentThread;
            acquireShared();
            // not nullptr because currentThread() can be null in case of mainThread not running
            m_lockThread=this;
            m_exclusiveMutex.unlock();
        }

        /**
         * @brief Release locker after shared access
         *
         * @attention Call only one releaseShared() per single acquireShared(),
         *  otherwise the behaviour is undefined
         */
        inline void releaseShared() noexcept
        {
            m_sharedCounter.fetch_sub(1,std::memory_order_release);
        }

        //! Acquire locker for exclusive access
        inline void acquireExclusive()
        {
            auto currentThread=Thread::currentThread();
            Assert(m_lockThread!=currentThread,"Recursive mutex lock in SharedLocker::acquireExclusive()");
            m_exclusiveMutex.lock();
            int tmp=0;
            while (!m_sharedCounter.compare_exchange_weak(tmp,COUNTER_LOCK_NEGATIVE_VAL,std::memory_order_seq_cst))
            {
                tmp=0;
            }
            m_lockThread=currentThread;
        }

        /**
         * @brief Release locker after exclusive access
         *
         * @attention Call only one releaseExclusive() per single acquireExclusive(),
         *  otherwise the behaviour is undefined
         */
        inline void releaseExclusive() noexcept
        {
            // not nullptr because currentThread() can be null in case of mainThread not running
            m_lockThread=this;
            m_sharedCounter.store(0,std::memory_order_release);
            m_exclusiveMutex.unlock();
        }

    private:

        std::atomic<int> m_sharedCounter;
        std::mutex m_exclusiveMutex;
        void* m_lockThread;

        constexpr static const int COUNTER_LOCK_NEGATIVE_VAL=std::numeric_limits<int>::min();
};

HATN_COMMON_NAMESPACE_END

#endif // HATNSHAREDLOCKER_H
