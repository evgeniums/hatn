/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/lru.h
  *
  */

/****************************************************************************/

#ifndef HATNLRU_H
#define HATNLRU_H

#include <utility>

#include <boost/intrusive/list.hpp>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename KeyT, typename ItemT>
class LruItem : public ItemT, public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>
{
    public:

        using ItemT::ItemT;

        void setKey(KeyT val) noexcept
        {
            m_key=std::move(val);
        }
        KeyT key() const noexcept
        {
            return m_key;
        }

    private:

        KeyT m_key;
};

/**
 * @brief Base template implementing LRU policy to use in caches.
 *
 * Actual cache items must be of type Lru::Item where Item will inherit from
 * ItemT type used by application.
 *
 */
using LruDefaultCapacity=std::integral_constant<size_t,16>;
template <typename KeyT, typename ItemT, typename StorageT, typename DefaultCapacity=LruDefaultCapacity>
class Lru
{
    public:

        using Type=Lru<KeyT,ItemT,StorageT,DefaultCapacity>;
        using StorageType=StorageT;

        using Item=LruItem<KeyT,ItemT>;

        /**
         * @brief Constructor
         * @param capacity Capacity of the cache
         * @param factory Allocator factory
         */
        explicit Lru(
                StorageType& storage,
                size_t capacity=DefaultCapacity::value
            ) : m_storage(storage),
                m_capacity(capacity)
        {}

        //! Dtor
        ~Lru()=default;

        Lru(const Lru&)=delete;
        Lru& operator=(const Lru&)=delete;

        Lru(Lru&&) =default;
        Lru& operator=(Lru&&) =default;

        /**
         * @brief Set item as the most recently used
         * @param item
         */
        void touchItem(Item& item)
        {
            if (&item!=&m_queue.back())
            {
                m_queue.erase(m_queue.iterator_to(item));
                m_queue.push_back(item);
            }
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
            Assert(m_queue.iterator_to(item)==m_queue.end(),"Can not add item that already is in some cache");

            if (isFull())
            {
                m_queue.pop_front_and_dispose([this](Item* lruItem){m_storage.doRemove(lruItem->key());});
            }
            auto&& key=item.key();
            auto& inserted=m_storage.doInsert(std::move(key),std::move(item));
            inserted.setKey(key);
            m_queue.push_back(inserted);
            return inserted;
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
            item.setKey(std::move(key));
            pushItem(std::move(item));
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
            if (isFull())
            {
                m_queue.pop_front_and_dispose([this](Item* lruItem){m_storage.doRemove(lruItem->key());});
            }
            auto& item=m_storage.doEmplace(std::piecewise_construct,
                          std::forward_as_tuple(key),
                          std::forward_as_tuple(std::forward<Args>(args)...)
                          );
            item.setKey(key);
            m_queue.push_back(item);
            return item;
        }

        //! Check if the cache is full
        bool isFull() const noexcept
        {
            if (!m_storage.empty() && m_storage.size()==m_capacity)
            {
                return true;
            }
            return false;
        }

        //! Get the most recently used item
        Item& mruItem()
        {
            if (m_storage.empty())
            {
                throw std::out_of_range("Cache is empty");
            }
            return m_queue.back();
        }

        //! Get the least recently used item
        Item& lruItem()
        {
            if (m_storage.empty())
            {
                throw std::out_of_range("Cache is empty");
            }
            return m_queue.front();
        }

        //! Get the most recently used item
        const Item& mruItem() const
        {
            if (m_storage.empty())
            {
                throw std::out_of_range("Cache is empty");
            }
            return m_queue.back();
        }

        //! Get the least recently used item
        const Item& lruItem() const
        {
            if (m_storage.empty())
            {
                throw std::out_of_range("Cache is empty");
            }
            return m_queue.front();
        }

        /**
         * @brief Remove item from the cache
         * @param item Item to remove
         */
        void removeItem(Item& item)
        {
            m_queue.erase(m_queue.iterator_to(item));
            m_storage.doRemove(item.key());
        }

        /**
         * @brief Clear cache
         */
        void clear()
        {
            m_queue.clear();
            m_storage.doClear();
        }

        /**
         * @brief Set capacity of the cache
         * @param capacity
         *
         * If cache is not empty then new capacity can not be less than the current size.
         */
        void setCapacity(size_t capacity)
        {
            Assert(m_storage.empty() || capacity>=m_storage.size(),"Can not decrease capacity of non empty cache");
            m_capacity=capacity;
        }

        //! Get cache capacity
        size_t capacity() const noexcept
        {
            return m_capacity;
        }

        auto queueBegin() const
        {
            return m_queue.begin();
        }

        auto queueBegin()
        {
            return m_queue.begin();
        }

        auto queueEnd() const
        {
            return m_queue.end();
        }

        auto queueEnd()
        {
            return m_queue.end();
        }

        auto queueIterator(const Item& item) const
        {
            return m_queue.iterator_to(item);
        }

        auto queueIterator(Item& item)
        {
            return m_queue.iterator_to(item);
        }

    private:

        StorageType& m_storage;
        size_t m_capacity;

        boost::intrusive::list<Item> m_queue;        
};

HATN_COMMON_NAMESPACE_END

#endif // HATNLRU_H
