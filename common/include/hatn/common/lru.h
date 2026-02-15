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

        LruItem(
                KeyT key,
                ItemT&& item
            ) : ItemT(std::move(item)),
                m_key(std::move(key))
        {}

        LruItem(
                KeyT key,
                const ItemT& item
            ) : ItemT(item),
                m_key(std::move(key))
        {}

        template <typename ...Args>
        explicit LruItem(
            KeyT key,
            Args&&... args
            ) : ItemT(std::forward<Args>(args)...),
                m_key(std::move(key))
        {}

        template <typename ...Args>
        explicit LruItem(
                Args&&... args
            ) : ItemT(std::forward<Args>(args)...)
        {}

        const KeyT& key() const noexcept
        {
            return m_key;
        }

        template <class... Args1, class... Args2>
        LruItem(
            std::piecewise_construct_t,
            std::tuple<Args1...> keyArgs,
            std::tuple<Args2...> itemArgs
            ) : LruItem(keyArgs,std::make_index_sequence<sizeof...(Args1)>{},
                      itemArgs,std::make_index_sequence<sizeof...(Args2)>{}
                      )
        {}

        void setDisplaceHandler(std::function<void(ItemT*)> handler)
        {
            m_onDisplace=handler;
        }

        const auto& displaceHandler() const
        {
            return m_onDisplace;
        }

    private:

        template<class KeyArgsTuple, std::size_t... ks, class ItemArgsTuple, std::size_t... is>
        LruItem(const KeyArgsTuple& kt, std::index_sequence<ks...>,
                const ItemArgsTuple& it, std::index_sequence<is...>
                ) : ItemT(std::get<is>(it)...),
                    m_key(std::get<ks>(kt)...)
        {}

        KeyT m_key;
        std::function<void(ItemT*)> m_onDisplace;
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
            checkFull();
            const auto& inserted=m_storage.doInsert(std::move(item));
            auto& itm=const_cast<Item&>(inserted);
            m_queue.push_back(itm);
            return itm;
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
            return emplaceItem(std::move(key),std::forward<T>(item));
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
        Item& emplaceItem(KeyT key, Args&&... args)
        {
            checkFull();
            auto& inserted=m_storage.doEmplace(std::piecewise_construct,
                          std::forward_as_tuple(key),
                          std::forward_as_tuple(key,std::forward<Args>(args)...)
                          );
            auto& item=const_cast<Item&>(inserted);
            m_queue.push_back(item);
            return item;
        }

        //! Check if the cache is full
        bool isFull() const noexcept
        {
            if (m_capacity==0)
            {
                return false;
            }

            if (!m_storage.empty() && m_storage.size()==m_capacity)
            {
                return true;
            }
            return false;
        }

        //! Get the most recently used item
        Item* mruItem()
        {
            if (m_storage.empty())
            {
                throw std::out_of_range("Cache is empty");
            }
            return &m_queue.back();
        }

        //! Get the least recently used item
        Item* lruItem()
        {
            if (m_storage.empty())
            {
                return nullptr;
            }
            return &m_queue.front();
        }

        //! Get the most recently used item
        const Item* mruItem() const
        {
            if (m_storage.empty())
            {
                return nullptr;
            }
            return &m_queue.back();
        }

        //! Get the least recently used item
        const Item* lruItem() const
        {
            if (m_storage.empty())
            {
                return nullptr;
            }
            return &m_queue.front();
        }

        template <typename KeyT1>
        void removeItem(const KeyT1& key, Item& item)
        {
            if (item.displaceHandler())
            {
                item.displaceHandler()(&item);
            }

            m_queue.erase(m_queue.iterator_to(item));
            m_storage.doRemove(key);
        }

        /**
         * @brief Remove item from the cache
         * @param item Item to remove
         */
        void removeItem(Item& item)
        {
            auto key=item.key();
            removeItem(key,item);
        }

        template <typename KeyT1>
        void removeItem(const KeyT1& key)
        {
            auto* item=m_storage.item(key);
            if (item!=nullptr)
            {
                removeItem(key,*item);
            }
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

        void checkFull()
        {
            if (isFull())
            {
                m_queue.pop_front_and_dispose(
                    [this](Item* lruItem)
                    {
                        if (lruItem->displaceHandler())
                        {
                            lruItem->displaceHandler()(lruItem);
                        }
                        m_storage.doRemove(lruItem->key());
                    }
                );
            }
        }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNLRU_H
