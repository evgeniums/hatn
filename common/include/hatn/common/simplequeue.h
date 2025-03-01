/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/simplequeue.h
  *
  *     Simple queue.
  */

/****************************************************************************/

#ifndef HATNSIMPLEQUEUE_H
#define HATNSIMPLEQUEUE_H

#include <hatn/common/common.h>
#include <hatn/common/queuetmpl.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename T>
class SimpleQueueTraits
{
    public:

        using Queue=QueueTmpl<T,SimpleQueueTraits<T>>;
        using Item=QueueItemTmpl<T>;

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
        SimpleQueueTraits(Queue* queue) noexcept
            : m_first(nullptr),
              m_last(nullptr),
              m_size(0),
              m_queue(queue)
        {}

        //! Dtor
        ~SimpleQueueTraits()
        {
            doClear();
        }

        //! Push item to queue
        void pushInternalItem(Item* item) noexcept
        {
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
            if (m_queue->isStatsEnabled())
            {
                updateSizeStats();
            }
        }

        //! Dequeue item without destroying it
        Item* popItem() noexcept
        {
            Item* first=nullptr;
            {
                if (m_size!=0)
                {
                    first=m_first;
                    m_first=m_first->m_next.load(std::memory_order_relaxed);
                    if (m_first==nullptr)
                    {
                        m_last=nullptr;
                    }
                    m_size--;
                    if (m_queue->isStatsEnabled())
                    {
                        updateEnqueuedDurationStats(first);
                        updateSizeStats();
                    }
                }
            }
            return first;
        }

        T* front() noexcept
        {
            if (m_first==nullptr)
            {
                return nullptr;
            }
            return &m_first->m_val;
        }

        //! Get queued size
        size_t size() const noexcept
        {
            return m_size;
        }

        //! Check if queue is empty
        bool isEmpty() const noexcept
        {
            return m_size==0;
        }

        //! Clear the queue
        void clear() noexcept
        {
            doClear();
        }

        //! Read statistics
        void readStats(size_t& maxSize,size_t& minSize,int64_t& maxDuration,int64_t& minDuration) noexcept
        {
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
        void resetStats() noexcept
        {
            m_stats.reset();
        }

    private:

        //! Clear the queue
        inline void doClear() noexcept
        {
            // clear all vars in locked mode
            Item* first=nullptr;
            {
                first=m_first;
                m_first=nullptr;
                m_last=nullptr;
                m_size=0;
                if (m_queue->isStatsEnabled())
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

        Queue* m_queue;
};

template <typename T>
using SimpleQueue=QueueTmpl<T,SimpleQueueTraits<T>>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNSIMPLEQUEUE_H
