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

#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/lruttl.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * @brief LRU cache with TTL.
 *
 * Actual cache items must be of type CacheLruTtl::Item where Item will inherit from
 * ItemT type used by application.
 *
 * @note All methods are not thread safe except for those marked as "thread safe" in description, e.g. clear() or each().
 * Use lock()/unlock() methods to guard access in multithreaded environment.
 *
 */
template <typename KeyT, typename ItemT, typename DefaultCapacity=LruDefaultCapacity, typename CompT=std::less<KeyT>>
class CacheLruTtl : public LruTtl<KeyT,ItemT,MapStorage<KeyT,LruTtlItem<KeyT,ItemT>,CompT>,DefaultCapacity>
{
    public:

        using Type=CacheLruTtl<KeyT,ItemT,DefaultCapacity,CompT>;

        using BaseLru=LruTtl<KeyT,ItemT,MapStorage<KeyT,LruTtlItem<KeyT,ItemT>,CompT>,DefaultCapacity>;

        /**
         * @brief Constructor
         * @param ttl Time to live in milliseconds
         * @param thread Thread the cache lives in
         * @param capacity Capacity of the cache
         * @param factory Allocator factory
         */
        explicit CacheLruTtl(
                uint64_t ttl,
                Thread* thread=Thread::currentThreadOrMain(),
                size_t capacity=DefaultCapacity::value,
                const pmr::AllocatorFactory* factory=pmr::AllocatorFactory::getDefault()
            ) : BaseLru(ttl,thread,capacity,factory)
        {}

        /**
         * @brief Constructor
         * @param ttl Time to live in milliseconds
         * @param thread Thread the cache lives in
         * @param factory Allocator factory
         */
        CacheLruTtl(
                uint64_t ttl,
                size_t capacity,
                const common::pmr::AllocatorFactory* factory=pmr::AllocatorFactory::getDefault()
            ) : CacheLruTtl(ttl,Thread::currentThreadOrMain(),capacity,factory)
        {}

        /**
         * @brief Constructor
         * @param ttl Time to live in milliseconds
         * @param factory Allocator factory
         * @param thread Thread the cache lives in
         */
        CacheLruTtl(
                uint64_t ttl,
                const common::pmr::AllocatorFactory* factory,
                Thread* thread=Thread::currentThreadOrMain()
            ) : CacheLruTtl(ttl,thread,DefaultCapacity::value,factory)
        {}
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNCACHELRUTTL_H
