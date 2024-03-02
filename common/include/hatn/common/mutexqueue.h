/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/queue.h
  *
  *     hatn queue.
  */

/****************************************************************************/

#ifndef HATNMUTEXQUEUE_H
#define HATNMUTEXQUEUE_H

#include <cmath>
#include <chrono>

#include <hatn/common/common.h>
#include <hatn/common/locker.h>
#include <hatn/common/queue.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Mutex queue
template <typename T>
class MutexQueue : public Queue<T>
{
    public:

        using Item=typename Queue<T>::Item;

        struct Stats
        {
            size_t maxSize;
            size_t minSize;
            int64_t maxDuration;
            int64_t minDuration;

            Stats() noexcept
                :
                  maxSize(0),
                  minSize(std::numeric_limits<size_t>::max()),
                  maxDuration(0),
                  minDuration(std::numeric_limits<int64_t>::max())
            {
            }

            inline void reset() noexcept
            {
                maxSize=0;
                minSize=std::numeric_limits<size_t>::max();
                maxDuration=0;
                minDuration=std::numeric_limits<int64_t>::max();
            }
        };

        /**
        * @brief Ctor
        * @param memResource Allocator's memory resource
        */
        MutexQueue(
            pmr::memory_resource* memResource=pmr::get_default_resource()
        ) noexcept : Queue<T>(memResource),
                     m_first(nullptr),
                     m_last(nullptr),
                     m_size(0)
        {}

        //! Dtor
        virtual ~MutexQueue()
        {
            doClear();
        }

        MutexQueue(const MutexQueue&)=delete;
        MutexQueue(MutexQueue&&) =delete;
        MutexQueue& operator=(const MutexQueue&)=delete;
        MutexQueue& operator=(MutexQueue&&) =delete;

        //! Push item to queue
        void pushInternalItem(Item* item) noexcept override final
        {
            MutexScopedLock l(m_lock);
            if (m_last!=nullptr)
            {
                m_last->m_next.store(item,std::memory_order_relaxed);
            }
            else
            {
                m_first=item;
            }
            m_last=item;
            m_size++;
            if (this->isStatsEnabled())
            {
                updateSizeStats();
            }
        }

        //! Dequeue item without destroying it
        virtual Item* popItem() noexcept override final
        {
            Item* first=nullptr;
            {
                MutexScopedLock l(m_lock);
                if (m_size!=0)
                {
                    first=m_first;
                    m_first=m_first->m_next.load(std::memory_order_relaxed);
                    if (m_first==nullptr)
                    {
                        m_last=nullptr;
                    }
                    m_size--;
                    if (this->isStatsEnabled())
                    {
                        updateEnqueuedDurationStats(first);
                        updateSizeStats();
                    }
                }
            }
            return first;
        }

        //! Get queued size
        virtual size_t size() const noexcept override final
        {
            MutexScopedLock l(m_lock);
            return m_size;
        }

        //! Check if queue is empty
        virtual bool isEmpty() const noexcept override final
        {
            MutexScopedLock l(m_lock);
            return m_size==0;
        }

        //! Clear the queue
        virtual void clear() noexcept override final
        {
            doClear();
        }

        //! Read statistics
        virtual void readStats(size_t& maxSize,size_t& minSize,int64_t& maxDuration,int64_t& minDuration) noexcept override final
        {
            MutexScopedLock l(m_lock);
            updateSizeStats();
            if (m_size!=0&&m_first!=nullptr)
            {
                updateEnqueuedDurationStats(m_first);
            }
            if (m_stats.minSize==std::numeric_limits<size_t>::max())
            {
                m_stats.minSize=0;
            }
            if (std::isgreaterequal(m_stats.minDuration,std::numeric_limits<int64_t>::max()))
            {
                m_stats.minDuration=0;
            }

            maxSize=m_stats.maxSize;
            minSize=m_stats.minSize;
            maxDuration=m_stats.maxDuration;
            minDuration=m_stats.minDuration;
        }

        //! Reset statistics
        virtual void resetStats() noexcept override final
        {
            MutexScopedLock l(m_lock);
            m_stats.reset();
        }

        //! Build queue of self type
        virtual Queue<T>* buildQueue(pmr::memory_resource* memResource=nullptr) override
        {
            return new MutexQueue<T>(memResource?memResource:this->allocator().resource());
        }

    private:

        //! Clear the queue
        inline void doClear() noexcept
        {
            // clear all vars in locked mode
            Item* first=nullptr;
            {
                MutexScopedLock l(m_lock);
                first=m_first;
                m_first=nullptr;
                m_last=nullptr;
                m_size=0;
                if (this->isStatsEnabled())
                {
                    m_stats.reset();
                }
            }
            // free memory blocks in unlocked mode
            while (first!=nullptr)
            {
                auto next=first->m_next.load(std::memory_order_relaxed);
                first->drop();
                first=next;
            }
        }

        inline void updateEnqueuedDurationStats(Item* item) noexcept
        {
            auto now=std::chrono::high_resolution_clock::now();
            auto duration=static_cast<int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now-item->m_enqueuedTime).count());
            m_stats.minDuration=std::min(m_stats.minDuration,duration);
            m_stats.maxDuration=std::max(m_stats.maxDuration,duration);
        }

        inline void updateSizeStats() noexcept
        {
            m_stats.minSize=std::min(m_stats.minSize,m_size);
            m_stats.maxSize=std::max(m_stats.maxSize,m_size);
        }

        Item* m_first;
        Item* m_last;
        size_t m_size;
        Stats m_stats;

        mutable MutexLock m_lock;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNMUTEXQUEUE_H
