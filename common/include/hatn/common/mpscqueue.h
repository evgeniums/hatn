/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/mpscqueue.h
  *
  *     Multpile Producer Single Consumer Queue.
  *
  */

/****************************************************************************/

#ifndef HATNMPSCQUEUE_H
#define HATNMPSCQUEUE_H

#include <chrono>
#include <algorithm>

#include <hatn/common/common.h>
#include <hatn/common/queue.h>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

HATN_COMMON_NAMESPACE_BEGIN

//! Multiple producers single consumer queue
/**
 *  Almost lock-free
 */
template <typename T>
class MPSCQueue : public Queue<T>
{
    public:

        //! Ctor
        MPSCQueue(
                pmr::memory_resource* memResource=pmr::get_default_resource()
            ) noexcept :  Queue<T>(memResource),
                          m_head(nullptr),
                          m_tail(nullptr),
                          m_size(0),
                          m_enableStats(false),
                          m_maxSize(0),
                          m_minSize(0),
                          m_maxDuration(0),
                          m_minDuration(0)
        {
        }

        //! Dtor
        virtual ~MPSCQueue()
        {
            doClear();
        }

        MPSCQueue(const MPSCQueue&)=delete;
        MPSCQueue(MPSCQueue&&) =delete;
        MPSCQueue& operator=(const MPSCQueue&)=delete;
        MPSCQueue& operator=(MPSCQueue&&) =delete;

        //! Push item to queue
        virtual void pushInternalItem(typename Queue<T>::Item* item) noexcept override final
        {            
            m_size.fetch_add(1,std::memory_order_acquire);

            if (m_enableStats)
            {
                updateSizeStats();
            }

            // put the item to tail
            typename Queue<T>::Item* tail=nullptr;
            while(!m_tail.compare_exchange_weak(tail,item,std::memory_order_acquire))
            {
            }

            if (tail!=nullptr)
            {
                // set the item as next for the last tail
                tail->m_next.store(item,std::memory_order_release);
            }
            else
            {
                // set the item as head
                m_head.store(item,std::memory_order_relaxed);
            }
        }

        //! Dequeue item without destroying it
        virtual typename Queue<T>::Item* popItem() noexcept override final
        {
            auto item=m_head.load(std::memory_order_relaxed);
            if (item!=nullptr)
            {                
                // clear tail if head==tail
                auto tmpItem=item;
                if (m_tail.compare_exchange_strong(tmpItem,nullptr,std::memory_order_acquire))
                {
                    // clear head
                    m_head.compare_exchange_strong(tmpItem,nullptr,std::memory_order_relaxed);
                }
                else
                {
                    // read next item
                    typename Queue<T>::Item* next=nullptr;
                    // here we have minor spin-lock, but hope it is very fast and of rare case
                    // we are sure that either the next is already set or it will be set soon to the tail because the item points to the head now
                    // and the head is not equal to the tail in this if-else-branch
                    do
                    {
                        next=item->m_next.load(std::memory_order_relaxed);
                    } while (next==nullptr);

                    // put the next item to head
                    m_head.store(next,std::memory_order_release);
                }

                // decrement size
                m_size.fetch_sub(1,std::memory_order_release);

                // update statistics
                if (m_enableStats)
                {
                    updateSizeStats();
                    updateEnqueuedDurationStats(item);
                }
            }
            return item;
        }

        //! Get approximate size
        virtual size_t size() const noexcept override final
        {
            return m_size.load(std::memory_order_relaxed);
        }

        //! Check if queue is empty
        virtual bool isEmpty() const noexcept override final
        {
            return m_head.load(std::memory_order_relaxed)==nullptr;
        }

        /**
         * @brief Clear the queue
         *
         * Note, that clearing is not thread safe and must be performed only from consumer thread or in destructor.
         * If producers are still active then only approximately initial size of queue will be cleared.
         */
        virtual void clear() noexcept override final
        {
            doClear();
        }

        //! Read statistics
        virtual void readStats(size_t& maxSize,size_t& minSize,int64_t& maxDuration,int64_t& minDuration) noexcept override final
        {
            maxSize=m_maxSize.load(std::memory_order_relaxed);
            minSize=m_minSize.load(std::memory_order_relaxed);
            maxDuration=m_maxDuration.load(std::memory_order_relaxed);
            minDuration=m_minDuration.load(std::memory_order_relaxed);
        }

        //! Reset statistics
        virtual void resetStats() noexcept override final
        {
            doResetStats();
        }

        //! Build queue of self type
        virtual Queue<T>* buildQueue(pmr::memory_resource* memResource=nullptr) override
        {
            return new MPSCQueue<T>(memResource?memResource:this->allocator().resource());
        }

    private:

        //! Reset statistics
        inline void doResetStats() noexcept
        {
            m_maxDuration.store(0,std::memory_order_relaxed);
            m_minDuration.store(0,std::memory_order_relaxed);
            m_maxSize.store(0,std::memory_order_relaxed);
            m_minSize.store(0,std::memory_order_relaxed);
        }
        inline void doClear() noexcept
        {
            auto maxCount=m_size.load(std::memory_order_acquire)+16; // 16 is just because the size can be approximate rather than exact
            bool enableStats=m_enableStats;
            m_enableStats=false;
            size_t i=0;
            auto item=this->popItem();
            while (item && i++<maxCount)
            {
                item->drop();
                item=this->popItem();
            }
            if (enableStats)
            {
                doResetStats();
                m_enableStats=enableStats;
            }
        }

        inline void updateSizeStats() noexcept
        {
            // approximate statistics, so relaxed is ok

            auto size=m_size.load(std::memory_order_relaxed);
            auto minSize=m_minSize.load(std::memory_order_relaxed);
            auto newMinSize=std::min(minSize,size);
            m_minSize.compare_exchange_strong(minSize,newMinSize,std::memory_order_relaxed);

            auto maxSize=m_maxSize.load(std::memory_order_relaxed);
            auto newMaxSize=std::max(maxSize,size);
            m_maxSize.compare_exchange_strong(maxSize,newMaxSize,std::memory_order_relaxed);
        }

        inline void updateEnqueuedDurationStats(typename Queue<T>::Item* item) noexcept
        {
            // approximate statistics, so relaxed is ok

            auto now=std::chrono::high_resolution_clock::now();
            auto duration=static_cast<int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now-item->m_enqueuedTime).count());

            auto minDuration=m_minDuration.load(std::memory_order_relaxed);
            auto newMinDuration=std::min(minDuration,duration);
            m_minDuration.compare_exchange_strong(minDuration,newMinDuration,std::memory_order_relaxed);

            auto maxDuration=m_maxDuration.load(std::memory_order_relaxed);
            auto newMaxDuration=std::max(maxDuration,duration);
            m_maxDuration.compare_exchange_strong(maxDuration,newMaxDuration,std::memory_order_relaxed);
        }

        std::atomic<typename Queue<T>::Item*> m_head;
        std::atomic<typename Queue<T>::Item*> m_tail;
        std::atomic<size_t> m_size;

        bool m_enableStats;

        std::atomic<size_t> m_maxSize;
        std::atomic<size_t> m_minSize;
        std::atomic<int64_t> m_maxDuration;
        std::atomic<int64_t> m_minDuration;
};

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNMPSCQUEUE_H
