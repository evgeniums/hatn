/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/lruttl.h
  *
  */

/****************************************************************************/

#ifndef HATNLRUTTL_H
#define HATNLRUTTL_H

#include <hatn/common/thread.h>
#include <hatn/common/locker.h>
#include <hatn/common/elapsedtimer.h>
#include <hatn/common/asiotimer.h>
#include <hatn/common/cachelru.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename ItemT>
class TtlItem : public ItemT
{
    public:

        using ItemT::ItemT;

        TtlItem(ItemT&& item) : ItemT(std::move(item))
        {}

        TtlItem(const ItemT& item) : ItemT(item)
        {}

        ElapsedTimer elapsed;        
};

template <typename KeyT, typename ItemT>
using LruTtlItem=LruItem<KeyT,TtlItem<ItemT>>;

/**
 * @brief Base template implementing LRU with expiration policy to use in caches.
 *
 * Actual cache items must be of type CacheLruTtl::Item where Item will inherit from
 * ItemT type used by application.
 *
 * @note All methods are not thread safe except for those marked "thread safe" in description, e.g. clear() or each().
 * Use lock()/unlock() methods to guard access in multithreaded environment.
 *
 */
template <typename KeyT, typename ItemT, typename StorageT, typename DefaultCapacity=LruDefaultCapacity>
class LruTtl
{
    public:

        using Type=LruTtl<KeyT,ItemT,StorageT,DefaultCapacity>;
        using LruType=Lru<KeyT,TtlItem<ItemT>,StorageT,DefaultCapacity>;

        using Item=LruTtlItem<KeyT,ItemT>;

    private:

        struct LruTtl_p
        {
            template <typename ...Args>
            LruTtl_p(
                    uint64_t ttl,
                    Thread* thread,
                    size_t capacity,
                    Args&&... args
                ) : storage(std::forward<Args>(args)...),
                    lru(storage,capacity),
                    ttlMs(ttl),
                    timer(thread),
                    timerPeriodMs(1000)
            {
                timer.setSingleShot(false);
                timer.setAutoAsyncGuardEnabled(false);
            }

            MutexLock mutex;
            StorageT storage;
            LruType lru;
            uint64_t ttlMs;
            AsioDeadlineTimer timer;
            uint32_t timerPeriodMs;

            bool empty() const noexcept
            {
                return storage.empty();
            }

            bool isExpired(const Item& item) const noexcept
            {
                return item.elapsed.elapsed().totalMilliseconds > ttlMs;
            }

            void isExpired(const Item& item, bool& expired) const noexcept
            {
                expired=isExpired(item);
            }

            const Item* lruItem(bool returnExpired=false) const
            {
                const auto* item=lru.lruItem();
                if (item==nullptr)
                {
                    return nullptr;
                }
                if (isExpired(*item) && !returnExpired)
                {
                    return nullptr;
                }
                return item;
            }

            Item* lruItem(bool returnExpired=false)
            {
                auto* item=lru.lruItem();
                if (item==nullptr)
                {
                    return nullptr;
                }
                if (isExpired(*item) && !returnExpired)
                {
                    return nullptr;
                }
                return item;
            }

            const Item* mruItem(bool returnExpired=false) const
            {
                const auto* item=lru.mruItem();
                if (item==nullptr)
                {
                    return nullptr;
                }
                if (isExpired(*item) && !returnExpired)
                {
                    return nullptr;
                }
                return item;
            }

            Item* mruItem(bool returnExpired=false)
            {
                auto* item=lru.mruItem();
                if (item==nullptr)
                {
                    return nullptr;
                }
                if (isExpired(*item) && !returnExpired)
                {
                    return nullptr;
                }
                return item;
            }

            void removeItem(Item& item)
            {
                lru.removeItem(item);
            }

            void clear()
            {
                lru.clear();
            }
        };

    public:

        /**
         * @brief Constructor
         * @param ttl Time to live in milliseconds
         * @param thread Thread the cache lives in
         * @param capacity Capacity of the cache
         * @param args Arguments forwarded to storage
         */
        template <typename ...Args>
        explicit LruTtl(
                uint64_t ttl,
                Thread* thread,
                size_t capacity,
                Args&& ...args
            ) : pimpl(std::make_shared<LruTtl_p>(ttl,thread,capacity,std::forward<Args>(args)...))
        {}

        //! Destructor
        ~LruTtl()
        {
            stop();
        }

        LruTtl(const LruTtl&)=delete;
        LruTtl& operator=(const LruTtl&)=delete;

        LruTtl(LruTtl&&) =default;
        LruTtl& operator=(LruTtl&&) =default;

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
            auto p=pimpl;
            pimpl->timer.start(
                [p](AsioDeadlineTimer::Status status)
                {
                    if (status!=AsioDeadlineTimer::Status::Timeout)
                    {
                        p->clear();
                        return;
                    }

                    MutexScopedLock l{p->mutex};

                    while(!p->empty())
                    {
                        auto* item=p->lruItem(true);
                        if (item==nullptr)
                        {
                            break;
                        }
                        if (!p->isExpired(*item))
                        {
                            break;
                        }
                        p->removeItem(*item);
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
            pimpl->ttlMs=ttl;
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
            return pimpl->ttlMs;
        }

        //! Get current cache size
        size_t size() const noexcept
        {
            return pimpl->storage.size();
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
            pimpl->lru.touchItem(item);
            item.elapsed.reset();
        }

        /**
         * @brief Touch item by key
         * @param key Key
         */
        void touch(const KeyT& key)
        {
            getAndTouch(key);
        }

        /**
         * @brief Put item to the cache
         * @param item Item to add
         *
         * The item's key must be already set in the item.
         * If cache is already full then the least recently used item will be displaced from the cache.
         * If some action on the item is required before displacing then use isFull() and oldestItem() before
         * adding a new item to the cache, e.g.:
         * <pre>
         * if (cache.isFull())
         * {
         *      auto* displacedItem=cache.lruItem();
         *      // do something with the item
         * }
         * cache.pushItem(newItem);
         * </pre>
         *
         * @attention Key must be set in the item before adding to cache, otherwise undefined behaviour is expected
         */
        Item& pushItem(Item item)
        {
            return pimpl->lru.pushItem(std::move(item));
        }

        /**
         * @brief Add item to the cache
         * @param key Item's key
         * @param item Item to add
         *
         * This is overloaded method, @see pushItem(Item&& item)
         */
        template <typename T>
        Item& pushItem(KeyT key, T&& item)
        {
            return pimpl->lru.pushItem(std::move(key),std::forward<T>(item));
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
            return pimpl->lru.emplaceItem(key,std::forward<Args>(args)...);
        }

        //! Check if the cache is full
        bool isFull() const noexcept
        {
            return pimpl->lru.isFull();
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
        Item* mruItem()
        {
            return pimpl->mruItem();
        }

        //! Get the least recently used item
        Item* lruItem()
        {
            return pimpl->lruItem();
        }

        //! Get the most recently used item
        const Item* mruItem() const
        {
            return pimpl->mruItem();
        }

        //! Get the least recently used item
        const Item* lruItem() const
        {
            return pimpl->lruItem();
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
            pimpl->lru.clear();
        }

        /**
         * @brief Set capacity of the cache
         * @param capacity
         *
         * If cache is not empty then new capacity can not be less than the current size.
         */
        void setCapacity(size_t capacity)
        {
            pimpl->lru.setCapacity(capacity);
        }

        //! Get cache capacity
        size_t capacity() const noexcept
        {
            return pimpl->lru.capacity();
        }

        /**
         * @brief Check if cache contains an item
         * @param key Key
         * @return Operation status
         */
        bool hasItem(const KeyT& key) const
        {
            return item(key)!=nullptr;
        }

        /**
         * @brief Find item by key
         * @param key Key
         * @return Found item
         */
        Item* get(const KeyT& key)
        {
            const auto* self=const_cast<const Type*>(this);
            return const_cast<Item*>(self->item(key));
        }

        Item* item(const KeyT& key)
        {
            return get(key);
        }

        /**
         * @brief Find item by key
         * @param key Key
         * @return Found item
         *
         */
        const Item* get(const KeyT& key) const
        {
            auto* itm=pimpl->storage.item(key);
            if (itm==nullptr)
            {
                return nullptr;
            }
            if (isExpired(*itm))
            {
                return nullptr;
            }
            return itm;
        }

        const Item* item(const KeyT& key) const
        {
            return get(key);
        }

        /**
         * @brief Find item by key
         * @param key Key
         * @return Found item
         *
         */
        Item* getAndTouch(const KeyT& key)
        {
            auto* itm=pimpl->storage.item(key);
            if (itm==nullptr)
            {
                return nullptr;
            }
            if (isExpired(*itm))
            {
                return nullptr;
            }
            touchItem(*itm);
            return itm;
        }

        /**
         * @brief Remove item by key
         * @param key Key
         */
        void remove(const KeyT& key)
        {
            auto itm=item(key);
            if (itm!=nullptr)
            {
                removeItem(*itm);
            }
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
        template <typename HandlerFn>
        size_t each(const HandlerFn& handler)
        {
            MutexScopedLock l{pimpl->mutex};
            auto hndlr=[&handler,this](auto& item)
            {
                if (!this->isExpired(item))
                {
                    return handler(item);
                }
                return true;
            };
            return pimpl->storage.each(hndlr);
        }

        template <typename HandlerFn>
        size_t each(const HandlerFn& handler) const
        {
            MutexScopedLock l{pimpl->mutex};
            auto hndlr=[&handler,this](const auto& item)
            {
                if (!this->isExpired(item))
                {
                    return handler(item);
                }
                return true;
            };
            return pimpl->storage.each(hndlr);
        }

        auto queueBegin() const
        {
            return pimpl->lru.queueBegin();
        }

        auto queueBegin()
        {
            return pimpl->lru.queueBegin();
        }

        auto queueEnd() const
        {
            return pimpl->lru.queueEnd();
        }

        auto queueEnd()
        {
            return pimpl->lru.queueEnd();
        }

        auto queueIterator(const Item& item) const
        {
            return pimpl->lru.queueIterator(item);
        }

        auto queueIterator(Item& item)
        {
            return pimpl->lru.queueIterator(item);
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

        std::shared_ptr<LruTtl_p> pimpl;
};

HATN_COMMON_NAMESPACE_END

#endif // HATNLRUTTL_H
