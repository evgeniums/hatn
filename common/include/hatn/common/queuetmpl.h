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

#ifndef HATNQUEUETMPL_H
#define HATNQUEUETMPL_H

#include <atomic>
#include <chrono>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/locker.h>
#include <hatn/common/objecttraits.h>
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

template <typename T>
struct QueueItemTmpl : public QueueItem
{
    T m_val;
    std::atomic<QueueItemTmpl*> m_next;

    //! Ctor
    QueueItemTmpl(
        pmr::polymorphic_allocator<QueueItemTmpl>& allocator,
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

        pmr::polymorphic_allocator<QueueItemTmpl>& m_allocator;
};

//! Base queue template class
template <typename T, typename Traits>
class QueueTmpl : public WithTraits<Traits>
{
    public:

        //! Storage item
        using Item = QueueItemTmpl<T>;

        //! Ctor
        template <typename ...TraitsArgs>
        QueueTmpl(
                pmr::memory_resource* memResource,
                TraitsArgs&& ...traitsArgs
            ) noexcept : WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
                         m_postingRefCount(0),
                         m_enableStats(false),
                         m_allocator(memResource)
        {}

        template <typename ...TraitsArgs>
        QueueTmpl(
            TraitsArgs&& ...traitsArgs
            ) noexcept : WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
            m_postingRefCount(0),
            m_enableStats(false),
            m_allocator(pmr::get_default_resource())
        {}

        ~QueueTmpl()=default;

        QueueTmpl(const QueueTmpl&)=delete;
        QueueTmpl(QueueTmpl&&) =default;
        QueueTmpl& operator=(const QueueTmpl&)=delete;
        QueueTmpl& operator=(QueueTmpl&&) =default;

        /**
         * @brief Add data to the queue
         * @param val Item to enqueue, note that the item will be moved to the queue
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
        Item* popItem() noexcept
        {
            return this->traits().popItem();
        }

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
        inline size_t size() const noexcept
        {
            return this->traits().size();
        }

        //! Check if queue is empty
        inline bool isEmpty() const noexcept
        {
            return this->traits().isEmpty();
        }

        inline bool empty() const noexcept
        {
            return isEmpty();
        }

        /**
         * @brief Clear the queue
         */
        inline void clear() noexcept
        {
            this->traits().clear();
        }

        //! Read statistics
        inline void readStats(size_t& maxSize,size_t& minSize,int64_t& maxDuration,int64_t& minDuration) noexcept
        {
            this->traits().readStats(maxSize,minSize,maxDuration,minDuration);
        }

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
        inline void resetStats() noexcept
        {
            this->traits().resetStats();
        }

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

        //! Get allocator
        inline const pmr::polymorphic_allocator<Item>& allocator() const noexcept
        {
            return m_allocator;
        }

    protected:

        //! Push item to queue
        inline void pushInternalItem(Item* item) noexcept
        {
            this->traits().pushInternalItem(item);
        }

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
#endif // HATNQUEUETMPL_H
