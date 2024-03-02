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

#ifndef HATNQUEUE_H
#define HATNQUEUE_H

#include <atomic>
#include <chrono>
#include <memory>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/locker.h>
#include <hatn/common/pmr/pmrtypes.h>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

HATN_COMMON_NAMESPACE_BEGIN

//! Base queue storage item
struct QueueItem
{    
    //! Ctor
    QueueItem()=default;

    std::chrono::high_resolution_clock::time_point m_enqueuedTime;
    void* m_data=nullptr;
};

//! Base queue template class
template <typename T>
class Queue
{
    public:

        //! Storage item
        struct Item : public QueueItem
        {
            T m_val;
            std::atomic<Item*> m_next;

            //! Ctor
            Item(
                pmr::polymorphic_allocator<Item>& allocator,
                T data=T()
            ) noexcept : m_val(std::move(data)),
                         m_next(nullptr),
                         m_allocator(allocator)
            {}

            //! Call it to destroy the item
            void drop() noexcept
            {
                pmr::destroyDeallocate(this,m_allocator);
            }

            private:

                pmr::polymorphic_allocator<Item>& m_allocator;
        };

        //! Ctor
        Queue(
                pmr::memory_resource* memResource=pmr::get_default_resource()
            ) noexcept : m_postingRefCount(0),
                         m_enableStats(false),
                         m_allocator(memResource)
        {}

        virtual ~Queue()=default;

        Queue(const Queue&)=delete;
        Queue(Queue&&) =delete;
        Queue& operator=(const Queue&)=delete;
        Queue& operator=(Queue&&) =delete;

        /**
         * @brief Add data to the queue
         * @param val Item to enqueue, note that the item will be copied to the queue
         */
        inline void push(T val)
        {
            return pushItem(createItem(std::move(val)));
        }

        //! Prepare item
        inline Item* prepare(T val)
        {
            return createItem(std::move(val));
        }

        //! Prepare item
        inline Item* prepare()
        {
            return createItem();
        }

        //! Push prepared item to queue
        inline void pushItem(QueueItem* item) noexcept
        {
            item->m_enqueuedTime=std::chrono::high_resolution_clock::now();
            auto it=static_cast<Item*>(item);
            pushInternalItem(it);
        }

        //! Dequeue item without destroying it
        virtual Item* popItem() noexcept =0;

        //! Dequeue value and destroy item
        inline bool pop(T& val) noexcept
        {
            bool ok=false;
            auto item=popItem();
            if (item)
            {
                val=item->m_val;
                item->drop();
                ok=true;
            }
            return ok;
        }

        /**
         * @brief Dequeue and destroy item returning the value
         * @return Value
         *
         * @throws std::runtime_error If queue is empty
         */
        inline T pop()
        {
            auto item=popItem();
            if (!item)
            {
                Assert(false,"The queue is empty");
                throw std::runtime_error("The queue is empty!");
            }
            auto val=item->m_val;
            item->drop();
            return val;
        }

        //! Dequeue item without destroying it
        inline bool popValAndItem(T*& val,QueueItem*& item) noexcept
        {
            bool ok=false;
            auto it=popItem();
            if (it)
            {
                val=&it->m_val;
                ok=true;
            }
            item=it;
            return ok;
        }

        //! Free item
        inline void freeItem(QueueItem* item) noexcept
        {
            auto it=static_cast<Item*>(item);
            it->drop();
        }

        //! Get approximate size
        virtual size_t size() const noexcept =0;

        //! Check if queue is empty
        virtual bool isEmpty() const noexcept =0;

        /**
         * @brief Clear the queue
         */
        virtual void clear() noexcept =0;

        //! Read statistics
        virtual void readStats(size_t& maxSize,size_t& minSize,int64_t& maxDuration,int64_t& minDuration) noexcept =0;

        //! Enabled or disable statistics
        inline void setStatsEnabled(bool enable) noexcept
        {
            resetStats();
            m_enableStats=enable;
        }

        //! Check if statistics is enabled
        inline bool isStatsEnabled() const noexcept
        {
            return m_enableStats;
        }

        //! Reset statistics
        virtual void resetStats() noexcept =0;

        //! Increment count of threads that are posting to queue
        inline void incPostingRefCount() noexcept
        {
            m_postingRefCount.fetch_add(1,std::memory_order_acquire);
        }
        //! Decrement count of threads that are posting to queue
        inline void decPostingRefCount() noexcept
        {
            m_postingRefCount.fetch_sub(1,std::memory_order_release);
        }
        //! Get count of threads that are posting to queue
        inline int postingRefCount() noexcept
        {
            return m_postingRefCount.load(std::memory_order_acquire);
        }

        //! Build queue of self type
        virtual Queue<T>* buildQueue(pmr::memory_resource* memResource=nullptr) =0;

        //! Get allocator
        inline const pmr::polymorphic_allocator<Item>& allocator() const noexcept
        {
            return m_allocator;
        }

    protected:

        //! Push item to queue
        virtual void pushInternalItem(Item* item) noexcept =0;

        //! Create item from value
        template <typename ... Args>
        inline Item* createItem(Args... args)
        {
            auto item=m_allocator.allocate(1);
            m_allocator.construct(item,m_allocator,std::forward<Args>(args)...);
            return item;
        }

    private:

        std::atomic<int> m_postingRefCount;
        bool m_enableStats;
        pmr::polymorphic_allocator<Item> m_allocator;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNQUEUE_H
