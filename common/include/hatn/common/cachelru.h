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

#include <hatn/common/lru.h>
#include <hatn/common/pmr/allocatorfactory.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename KeyT,
         typename ItemT,
         typename CompT=std::less<KeyT>
         >
class MapStorage
{
    public:

        using Type=MapStorage<KeyT,ItemT,CompT>;

        explicit MapStorage(
                const pmr::AllocatorFactory* factory=pmr::AllocatorFactory::getDefault(),
                const CompT& comp=CompT{}
            ) : m_map(comp,factory->objectAllocator<typename mapT::value_type>())
        {}

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
        ItemT* item(const KeyT& key)
        {
            auto it=m_map.find(key);
            if (it==m_map.end())
            {
                return nullptr;
            }
            return &it->second;
        }

        /**
             * @brief Find item by key
             * @param key Key
             * @return Found item
             *
             * @throws out_of_range if cache does not contain such item
             */
        const ItemT* item(const KeyT& key) const
        {
            const auto* self=const_cast<const Type*>(this);
            return const_cast<ItemT*>(self->item(key));
        }

        /**
             * @brief Do operation on each element of the cache
             * @param handler Handler to invoke for each element
             * @return Count of handler invokations
             *
             * If handler returns false then iterating will be broken.
             */
        template <typename HandlerFn>
        size_t each(const HandlerFn& handler)
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

        template <typename HandlerFn>
        size_t each(const HandlerFn& handler) const
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

        ItemT& doInsert(ItemT&& item)
        {
            auto inserted=m_map.insert_or_assign(item.key(),std::move(item));
            return inserted.first->second;
        }

        template <class... Args1, class... Args2>
        ItemT& doEmplace(std::piecewise_construct_t,
                        std::tuple<Args1...> keyArgs,
                        std::tuple<Args2...> valueArgs
                        )
        {
            auto inserted=m_map.emplace(std::piecewise_construct,std::move(keyArgs),std::move(valueArgs));
            return inserted.first->second;
        }

        void doRemove(const KeyT& key)
        {
            m_map.erase(key);
        }

        void doClear()
        {
            m_map.clear();
        }

    private:

        using mapT=pmr::map<KeyT,ItemT,CompT>;
        mapT m_map;
};

/**
 * @brief LRU cache
 *
 * Actual cache items must be of type CacheLRU::Item where Item will inherit from
 * ItemT type used by application.
 *
 */
template <typename KeyT,
         typename ItemT,
         typename DefaultCapacity=LruDefaultCapacity,
         typename CompT=std::less<KeyT>
         >
class CacheLru : public MapStorage<KeyT,LruItem<KeyT,ItemT>,CompT>,
                 public Lru<KeyT,ItemT,MapStorage<KeyT,LruItem<KeyT,ItemT>,CompT>,DefaultCapacity>
{
    public:

        using Type=CacheLru<KeyT,ItemT,DefaultCapacity,CompT>;

        using BaseStorage=MapStorage<KeyT,LruItem<KeyT,ItemT>,CompT>;
        using BaseLru=Lru<KeyT,ItemT,BaseStorage,DefaultCapacity>;

        using Item=typename BaseLru::Item;

        /**
         * @brief Constructor
         * @param capacity Capacity of the cache
         * @param factory Allocator factory
         */
        explicit CacheLru(
                size_t capacity=DefaultCapacity::value,
                const pmr::AllocatorFactory* factory=pmr::AllocatorFactory::getDefault(),
                const CompT& comp=CompT{}
            ) : BaseStorage(factory,comp),
                BaseLru(*this,capacity)
        {}

        /**
         * @brief Constructor
         * @param factory Allocator factory
         */
        explicit CacheLru(
                const pmr::AllocatorFactory* factory
            ) : CacheLru(DefaultCapacity::value,factory)
        {}

        //! Destructor
        ~CacheLru()=default;

        CacheLru(const CacheLru&)=delete;
        CacheLru& operator=(const CacheLru&)=delete;

        CacheLru(CacheLru&&) =default;
        CacheLru& operator=(CacheLru&&) =default;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNCACHELRU_H
