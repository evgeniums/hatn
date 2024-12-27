/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/cachelruttl.h
  *
  *      LRU cache with expiration.
  *
  */

/****************************************************************************/

#ifndef HATNCACHELRUTTL_H
#define HATNCACHELRUTTL_H

#include <hatn/common/thread.h>
#include <hatn/common/locker.h>
#include <hatn/common/elapsedtimer.h>
#include <hatn/common/asiotimer.h>
#include <hatn/common/cachelru.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * @brief LRU cache with TTL.
 *
 * Actual cache items must be of type CacheLruTtl::Item where Item will inherit from
 * ItemT type used by application.
 *
 * @note All methods are not thread safe except for those marked "thread safe" in description, e.g. clear() or each().
 * Use lock()/unlock() methods to guard access in multithreaded environment.
 *
 */
template <typename KeyT, typename ItemT, typename CompT=std::less<KeyT>>
class CacheLruTtl
{
    private:

        class TtlItem : public ItemT
        {
            public:

                using ItemT::ItemT;

                ElapsedTimer elapsed;
        };

    public:

        using Type=CacheLruTtl<KeyT,ItemT,CompT>;
        using CacheLru=CacheLRU<KeyT,TtlItem,CompT>;

        using Item=typename CacheLru::Item;

    private:

        struct CacheLruTtl_p
        {
            CacheLruTtl_p(
                    uint64_t ttl,
                    Thread* thread,
                    size_t capacity,
                    const pmr::AllocatorFactory* factory
                ) : cache(capacity,factory),
                    ttlMs(ttl),
                    timer(thread),
                    timerPeriodMs(1000)
            {
                timer.setSingleShot(false);
            }

            MutexLock mutex;
            CacheLru cache;
            uint64_t ttlMs;
            AsioDeadlineTimer timer;
            uint32_t timerPeriodMs;

            bool empty() const noexcept
            {
                return cache.empty();
            }

            bool isExpired(const Item& item) const noexcept
            {
                return item.elapsed.elapsed().totalMilliseconds > ttlMs;
            }

            void isExpired(const Item& item, bool& expired) const noexcept
            {
                expired=isExpired(item);
            }

            Item& lruItem(bool& expired)
            {
                auto& item=cache.lruItem();
                isExpired(item,expired);
                return item;
            }

            void removeItem(Item& item)
            {
                cache.removeItem(item);
            }
        };

    public:

        /**
         * @brief Constructor
         * @param ttl Time to live in milliseconds
         * @param thread Thread the cache lives in
         * @param capacity Capacity of the cache
         * @param factory Allocator factory
         */
        explicit CacheLruTtl(
                uint64_t ttl,
                Thread* thread=Thread::currentThread(),
                size_t capacity=CacheLru::CAPACITY,
                const pmr::AllocatorFactory* factory=pmr::AllocatorFactory::getDefault()
            ) : pimpl(std::make_shared<CacheLruTtl_p>(ttl,thread,capacity,factory))
        {}

        /**
         * @brief Constructor
         * @param ttl Time to live in milliseconds
         * @param thread Thread the cache lives in
         * @param factory Allocator factory
         */
        CacheLruTtl(
                uint64_t ttl,
                Thread* thread,
                const common::pmr::AllocatorFactory* factory=pmr::AllocatorFactory::getDefault()
            ) : CacheLruTtl(ttl,thread,CacheLru::CAPACITY,factory)
        {}

        /**
         * @brief Constructor
         * @param ttl Time to live in milliseconds
         * @param factory Allocator factory
         */
        CacheLruTtl(
                uint64_t ttl,
                const common::pmr::AllocatorFactory* factory
            ) : CacheLruTtl(ttl,Thread::currentThread(),CacheLru::CAPACITY,factory)
        {}

        //! Destructor
        ~CacheLruTtl()
        {
            stop();
        }

        CacheLruTtl(const CacheLRU&)=delete;
        CacheLruTtl& operator=(const CacheLRU&)=delete;

        CacheLruTtl(CacheLRU&&) =default;
        CacheLruTtl& operator=(CacheLRU&&) =default;

        //! Start TTL timer
        /**
         * @brief Start TTL timer.
         * @param timerPeriodMs Period of timer invokation in milliseconds. If 0 then use previously set period or default.
         *
         * Defaut period is 1000ms.
         * Thread safe.
         */
        void start(uint32_t timerPeriodMs=0)
        {
            if (timerPeriodMs!=0)
            {
                pimpl->timerPeriodMs=timerPeriodMs;
            }
            pimpl->timer.setPeriodUs(pimpl->timerPeriodMs*1000);
            pimpl->timer.start(
                [pimpl](AsioDeadlineTimer::Status status)
                {
                    if (status!=AsioDeadlineTimer::Status::Timeout)
                    {
                        return;
                    }

                    MutexScopedLock l{pimpl->mutex};

                    while(!pimpl->empty())
                    {
                        bool expired=false;
                        auto& item=pimpl->lruItem(expired);
                        if (!expired)
                        {
                            break;
                        }
                        pimpl->removeItem(item);
                    }
                }
            );
        }

        /**
         * @brief Stop TTL timer.
         *
         * Thread safe.
         */
        void stop()
        {
            pimpl->timer.reset();
            pimpl->timer.stop();
        }

        /**
         * @brief Set TTL of items.
         * @param ttl New TTL.
         *
         * New TTL will be applied to only to new inserted or touched items.
         * Thread safe.
         */
        void setTtl(uint64_t ttl) noexcept
        {
            MutexScopedLock l{pimpl->mutex};
            pimpl->ttl=ttl;
        }

        /**
         * @brief Get TTL of items.
         * @return TTL of items.
         *
         * Thread safe.
         */
        uint64_t ttl() const noexcept
        {
            MutexScopedLock l{pimpl->mutex};
            return pimpl->ttl;
        }

        //! Get current cache size
        size_t size() const noexcept
        {
            return pimpl->cache.size();
        }

        //! Check if cahce is empty
        bool empty() const noexcept
        {
            return pimpl->empty();
        }

        /**
         * @brief Set item as the most recently used
         * @param item
         */
        void touchItem(Item& item)
        {
            pimpl->cache.touchItem(item);
            item.elapsed.reset();
        }

        /**
         * @brief Add item to the cache
         * @param item Item to add
         *
         * The item's key must be already set in the item.
         * If cache is already full then the least recently used item will be displaced from the cache.
         * If some action on the item is required before displacing then use isFull() and oldestItem() before
         * adding a new item to the cache, e.g.:
         * <pre>
         * if (cache.isFull())
         * {
         *      auto& displacedItem=cache.lruItem();
         *      // do something with the item
         * }
         * cache.pushItem(newItem);
         * </pre>
         *
         * @attention Key must be set in the item before adding to cache, otherwise undefined behaviour is expected
         */
        Item& pushItem(Item&& item)
        {
            return pimpl->cache.pushItem(std::move(item));
        }

        /**
         * @brief Add item to the cache
         * @param key Item's key
         * @param item Item to add
         *
         * This is overloaded method, @see pushItem(Item&& item)
         */
        void pushItem(KeyT key,Item&& item)
        {
            return pimpl->cache.pushItem(std::move(key),std::forward<Args>(args)...);
        }

        /**
         * @brief Construct and add item to the cache
         * @param key Item's key
         * @param args Arguments for forwarding to item's constructor
         *
         * @see pushItem(Item&& item) for details about adding adding item.
         *
         */
        template <typename ...Args>
        Item& emplaceItem(const KeyT& key,Args&&... args)
        {
            return pimpl->cache.emplaceItem(key,std::forward<Args>(args)...);
        }

        //! Check if the cache is full
        bool isFull() const noexcept
        {
            return pimpl->cache.isFull();
        }

        bool isExpired(const Item& item) const noexcept
        {
            return pimpl->isExpired(item);
        }

        void isExpired(const Item& item, bool& expired) const noexcept
        {
            expired=isExpired(item);
        }

        //! Get the most recently used item
        Item& mruItem(bool& expired)
        {
            auto& item=pimpl->cache.mruItem();
            isExpired(item,expired);
            return item;
        }

        //! Get the least recently used item
        Item& lruItem(bool& expired)
        {
            return pimpl->lruItem(expired);
        }

        //! Get the most recently used item
        const Item& mruItem(bool& expired) const
        {
            auto& item=pimpl->cache.mruItem();
            isExpired(item,expired);
            return item;
        }

        //! Get the least recently used item
        const Item& lruItem(bool& expired) const
        {
            auto& item=pimpl->cache.lruItem();
            isExpired(item,expired);
            return item;
        }

        /**
         * @brief Remove item from the cache
         * @param item Item to remove
         */
        void removeItem(Item& item)
        {
            pimpl->removeItem(item);
        }

        /**
         * @brief Clear cache
         *
         * Thread safe.
         */
        void clear()
        {
            MutexScopedLock l{pimpl->mutex};
            pimpl->cache.clear();
        }

        /**
         * @brief Set capacity of the cache
         * @param capacity
         *
         * If cache is not empty then new capacity can not be less than the current size.
         */
        void setCapacity(size_t capacity)
        {
            pimpl->cache.setCapacity(capacity);
        }

        //! Get cache capacity
        size_t capacity() const noexcept
        {
            return pimpl->cache.capacity();
        }

        /**
         * @brief Check if cache contains an item
         * @param key Key
         * @return Operation status
         */
        bool hasItem(const KeyT& key) const
        {
            if (pimpl->cache.hasItem(key))
            {
                bool expired;
                item(key,expired);
                return !expired;
            }
            return false;
        }

        /**
         * @brief Find item by key
         * @param key Key
         * @return Found item
         *
         * @throws out_of_range if cache does not contain such item
         */
        Item& item(const KeyT& key, bool& expired)
        {
            auto& itm=pimpl->cache.item(key);
            isExpired(expired);
            return itm;
        }

        /**
         * @brief Find item by key
         * @param key Key
         * @return Found item
         *
         * @throws out_of_range if cache does not contain such item
         */
        const Item& item(const KeyT& key, bool& expired) const
        {
            const auto& itm=pimpl->cache.item(key);
            isExpired(expired);
            return itm;
        }

        /**
         * @brief Do operation on each element of the cache
         * @param handler Handler to invoke for each element
         * @return Count of handler invokations
         *
         * If handler returns false then iterating will be broken.
         *
         * Thread safe.
         */
        size_t each(const std::function<bool (Item&)>& handler)
        {
            MutexScopedLock l{mutex};
            auto hndlr=[&handler,this](Item& itm)
            {
                if (!isExpired(itm))
                {
                    return handler(item);
                }
                return true;
            };
            return pimpl->cache.each(hndlr);
        }

        auto queueBegin() const
        {
            return pimpl->cache.queueBegin();
        }

        auto queueBegin()
        {
            return pimpl->cache.queueBegin();
        }

        auto queueEnd() const
        {
            return pimpl->cache.queueEnd();
        }

        auto queueEnd()
        {
            return pimpl->cache.queueEnd();
        }

        auto queueIterator(const Item& item) const
        {
            return pimpl->cache.queueIterator(item);
        }

        auto queueIterator(Item& item)
        {
            return pimpl->cache.queueIterator(item);
        }

        /**
         * @brief Lock cache's mutex.
         *
         * Thread safe.
         */
        void lock()
        {
            pimpl->mutex.lock();
        }

        /**
         * @brief Unlock cache's mutex.
         *
         * Thread safe.
         */
        void unlock()
        {
            pimpl->mutex.unlock();
        }

    private:

        std::shared_ptr<CacheLruTtl_p> pimpl;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNCACHELRUTTL_H
