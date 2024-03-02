/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/memorypool/poolcachegen.h
  *
  *     Generator and storage cache of global memory pools
  *
  */

/****************************************************************************/

#ifndef HATNPOOLCACHEGEN_H
#define HATNPOOLCACHEGEN_H

#include <iostream>

#include <functional>
#include <map>
#include <memory>
#include <set>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/types.h>
#include <hatn/common/classuid.h>
#include <hatn/common/sharedlocker.h>

#include <hatn/common/memorypool/pool.h>
#include <hatn/common/memorypool/memblocksize.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace memorypool {

/**
 * @brief Cache and generator of memory pools
 *
 * Default implementation returns pool for allocation of data of exact requested size
 */
template <typename PoolT>
class PoolCacheGen
{
    public:

        using PoolCreatorFn=std::function<std::shared_ptr<PoolT> (const memorypool::PoolContext::Parameters&)>;
        static std::shared_ptr<PoolT> defaultPoolCreator(const memorypool::PoolContext::Parameters& params)
        {
            return std::make_shared<PoolT>(params);
        }

        /**
         * @brief Constructor
         * @param poolType Pool type
         * @param bucketUsefulSize Size of a bucket to allocate
         */
        PoolCacheGen(
            size_t bucketUsefulSize=DEFAULT_BUCKET_USEFUL_SIZE,
            size_t minBlockSize=MemBlockSize::DEFAULT_MIN_BLOCK_SIZE,
            size_t maxBlockSize=MemBlockSize::DEFAULT_MAX_BLOCK_SIZE
        ) : m_bucketUsefulSize(bucketUsefulSize),
            m_sizes(minBlockSize,maxBlockSize),
            m_poolCreator(&PoolCacheGen<PoolT>::defaultPoolCreator)
        {}

        /**
         * @brief Find in cache or create a pool for certain block size
         * @param size Useful size of a block
         * @param exactSize If false then find the first pool the requested size fits the chunk size of,
         *                  otherwise use only pool whose chunk size exactly matches the requested size
         * @return Found or created pool
         */
        inline std::shared_ptr<PoolT> pool(size_t size, bool exactSize=false)
        {
            KeyRange key={size,size};
            auto p=findInCache(key,exactSize);
            if (!p)
            {
                p=exactSize?createExactSizePool(key):createMemSizedPool(key);
            }
            return p;
        }

        inline void setBucketUsefulSize(
            size_t size
        )
        {
            Assert(m_pools.empty(),"Size may be set only before any pools were generated!");
            m_bucketUsefulSize=size;
        }

        inline size_t bucketUsefulSize() const noexcept
        {
            return m_bucketUsefulSize;
        }

        //! Owned pools of range sizes
        inline const std::map<KeyRange,std::shared_ptr<PoolT>>& pools() const noexcept
        {
            return m_pools;
        }
        //! Owned pools of exact sizes
        inline const std::map<KeyRange,std::shared_ptr<PoolT>>& exactPools() const noexcept
        {
            return m_exactPools;
        }

        //! Calculate pool parametes for object size
        inline memorypool::PoolContext::Parameters poolParameters(size_t objectSize) const noexcept
        {
            auto chunkCount=m_bucketUsefulSize/objectSize;
            if (chunkCount==0)
            {
                chunkCount=1;
            }
            return memorypool::PoolContext::Parameters(objectSize,chunkCount);
        }

        const MemBlockSize& sizes() const
        {
            return m_sizes;
        }
        MemBlockSize& sizes()
        {
            return m_sizes;
        }

        void setPoolCreator(PoolCreatorFn creator) noexcept
        {
            m_poolCreator=std::move(creator);
        }
        PoolCreatorFn poolCreator() const noexcept
        {
            return m_poolCreator;
        }

    protected:

        std::shared_ptr<PoolT> createExactSizePool(KeyRange& key)
        {
            auto pool=m_poolCreator(poolParameters(key.to));
            putInCache(pool,key,true);
            return pool;
        }
        std::shared_ptr<PoolT> createMemSizedPool(KeyRange& key)
        {
            auto it=sizes().sizes().find(key);
            if (it!=sizes().sizes().end())
            {
                key=*it;
                auto pool=m_poolCreator(poolParameters(it->to));
                putInCache(pool,key,false);
                return pool;
            }
            return std::shared_ptr<PoolT>();
        }

    private:

        inline void putInCache(std::shared_ptr<PoolT>& pool, KeyRange key, bool exactSize)
        {
            SharedLocker::ExclusiveScope l(m_locker);

            auto& pools=exactSize?m_exactPools:m_pools;
            auto it=pools.find(key);
            if (it==pools.end())
            {
                pools.insert(std::make_pair(std::move(key),pool));
            }
            else
            {
                pool=it->second;
            }
        }

        inline std::shared_ptr<PoolT> findInCache(const KeyRange& key,bool exactSize) const noexcept
        {
            SharedLocker::SharedScope l(m_locker);

            auto& pools=exactSize?m_exactPools:m_pools;
            auto it=pools.find(key);
            if (it!=pools.end())
            {
                return it->second;
            }
            else if (key.from==key.to)
            {
                auto& pools1=!exactSize?m_exactPools:m_pools;
                auto it1=pools1.find(key);
                if (it1!=pools1.end() && key.to==it1->first.to)
                {
                    return it1->second;
                }
            }
            return std::shared_ptr<PoolT>();
        }

        static size_t DEFAULT_BUCKET_USEFUL_SIZE; // 256*1024;

        size_t m_bucketUsefulSize;
        mutable SharedLocker m_locker;

        std::map<KeyRange,std::shared_ptr<PoolT>> m_pools;
        std::map<KeyRange,std::shared_ptr<PoolT>> m_exactPools;

        MemBlockSize m_sizes;

        PoolCreatorFn m_poolCreator;
};
template <typename PoolT>
size_t PoolCacheGen<PoolT>::DEFAULT_BUCKET_USEFUL_SIZE=32*1024;

//---------------------------------------------------------------
        } // namespace memorypool
    HATN_COMMON_NAMESPACE_END
#endif // HATNPOOLCACHEGEN_H
