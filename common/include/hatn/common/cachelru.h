/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/cachelru.h
  *
  *      LRU cache.
  *
  */

/****************************************************************************/

#ifndef HATNCACHELRU_H
#define HATNCACHELRU_H

#include <functional>

#include <boost/intrusive/list.hpp>
#include <hatn/common/pmr/allocatorfactory.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * @brief LRU cache
 *
 * Actual cache items must be of type CacheLRU::Item where Item will inherit from
 * ItemT type used by application.
 *
 */
template <typename KeyT,typename ItemT>
class CacheLRU
{
    public:

        using Type=CacheLRU<KeyT,ItemT>;

        /**
         * @brief Default capacity of the cache
         */
        constexpr static const size_t CAPACITY=16;

        /**
         * @brief Cache item
         */
        class Item : public ItemT, public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>
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
         * @brief Constructor
         * @param capacity Capacity of the cache
         * @param factory Allocator factory
         */
        explicit CacheLRU(
                size_t capacity=CAPACITY,
                common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_capacity(capacity),
                m_map(factory->objectAllocator<typename mapT::value_type>())
        {}

        /**
         * @brief Constructor
         * @param factory Allocator factory
         */
        explicit CacheLRU(
                common::pmr::AllocatorFactory* factory
            ) : CacheLRU(CAPACITY,factory)
        {}

        //! Dtor
        ~CacheLRU()=default;

        CacheLRU(const CacheLRU&)=delete;
        CacheLRU& operator=(const CacheLRU&)=delete;

        CacheLRU(CacheLRU&&) =default;
        CacheLRU& operator=(CacheLRU&&) =default;

        //! Get current cache size
        size_t size() const noexcept
        {
            return m_map.size();
        }

        //! Check if cahce is empty
        bool empty() const noexcept
        {
            return m_map.empty();
        }

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
         *      auto& displacedItem=cache.oldestItem();
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
                m_queue.pop_front_and_dispose([this](Item* mruItem){m_map.erase(mruItem->key());});
            }
#if __cplusplus < 201703L
            auto key=item.key();
            m_map[key]=std::move(item);
            return m_map[key];
#else
            auto&& key=item.key();
            auto inserted=m_map.insert_or_assign(std::move(key),std::move(item));
            m_queue.push_back(inserted.second->second);
            return inserted.second->second;
#endif
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
                m_queue.pop_front_and_dispose([this](Item* mruItem){m_map.erase(mruItem->key());});
            }
            auto inserted=m_map.emplace(std::piecewise_construct,
                          std::forward_as_tuple(key),
                          std::forward_as_tuple(std::forward<Args>(args)...)
                          );
            inserted.first->second.setKey(key);
            m_queue.push_back(inserted.first->second);
            return inserted.first->second;
        }

        //! Check if the cache is full
        bool isFull() const noexcept
        {
            if (!empty() && size()==m_capacity)
            {
                return true;
            }
            return false;
        }

        //! Get the most recently used item
        Item& mruItem()
        {
            if (empty())
            {
                throw std::out_of_range("Cache is empty");
            }
            return m_queue.front();
        }

        //! Get the least recently used item
        Item& lruItem()
        {
            if (empty())
            {
                throw std::out_of_range("Cache is empty");
            }
            return m_queue.back();
        }

        //! Get the most recently used item
        const Item& mruItem() const
        {
            if (empty())
            {
                throw std::out_of_range("Cache is empty");
            }
            return m_queue.front();
        }

        //! Get the least recently used item
        const Item& lruItem() const
        {
            if (empty())
            {
                throw std::out_of_range("Cache is empty");
            }
            return m_queue.back();
        }

        /**
         * @brief Remove item from the cache
         * @param item Item to remove
         */
        void removeItem(Item& item)
        {
            m_queue.erase(m_queue.iterator_to(item));
            m_map.erase(item.key());
        }

        /**
         * @brief Ckear cache
         */
        void clear()
        {
            m_queue.clear();
            m_map.clear();
        }

        /**
         * @brief Set capacity of the cache
         * @param capacity
         *
         * If cache is not empty then new capacity can not be less than the current size.
         */
        void setCapacity(size_t capacity)
        {
            Assert(empty() || capacity>=size(),"Can not decrease capacity of non empty cache");
            m_capacity=capacity;
        }
        //! Get cache capacity
        size_t capacity() const noexcept
        {
            return m_capacity;
        }

        /**
         * @brief Check if cache contains an item
         * @param key Key
         * @return Operation status
         */
        bool hasItem(const KeyT& key) const
        {
            return m_map.find(key)!=m_map.end();
        }

        /**
         * @brief Find item by key
         * @param key Key
         * @return Found item
         *
         * @throws out_of_range if cache does not contain such item
         */
        Item& item(const KeyT& key)
        {
            auto it=m_map.find(key);
            if (it==m_map.end())
            {
                throw std::out_of_range("No such item in the cache");
            }
            return it->second;
        }

        /**
         * @brief Find item by key
         * @param key Key
         * @return Found item
         *
         * @throws out_of_range if cache does not contain such item
         */
        const Item& item(const KeyT& key) const
        {
            return const_cast<Type*>(this)->item(key);
        }

        /**
         * @brief Do operation on each element of the cache
         * @param handler Handler to invoke for each element
         * @return Count of handler invokations
         *
         * If handler returns false then iterating will be broken.
         */
        size_t each(const std::function<bool (Item&)>& handler)
        {
            size_t count=0;
            for (auto&& it: m_map)
            {
                if (!handler(it.second))
                {
                    return count;
                }
                ++count;
            }
            return count;
        }

    private:

        size_t m_capacity;

        boost::intrusive::list<Item> m_queue;

        using mapT=common::pmr::map<KeyT,Item>;
        mapT m_map;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNCACHELRU_H
