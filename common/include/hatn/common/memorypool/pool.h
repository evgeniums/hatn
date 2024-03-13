/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/memorypool/pool.h
  *
  *     Base class for memory pools
  *
  */

/****************************************************************************/

#ifndef HATNMEMORYPOOL__H
#define HATNMEMORYPOOL__H

#include <stddef.h>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/classuid.h>
#include <hatn/common/objecttraits.h>
#include <hatn/common/logger.h>

#include <hatn/common/memorypool/rawblock.h>

DECLARE_LOG_MODULE_EXPORT(mempool,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN
namespace memorypool {

//! Memory pool statistics
struct Stats
{
    size_t usedChunkCount;
    size_t allocatedChunkCount;
    size_t maxBucketChunkCount;
    size_t minBucketChunkCount;
    size_t maxBucketUsedChunkCount;
    size_t minBucketUsedChunkCount;

    //! Ctor
    Stats(
        )  noexcept :
            usedChunkCount(0),
            allocatedChunkCount(0),
            maxBucketChunkCount(0),
            minBucketChunkCount(std::numeric_limits<size_t>::max()),
            maxBucketUsedChunkCount(0),
            minBucketUsedChunkCount(std::numeric_limits<size_t>::max())
    {
    }

    //! Reset statistics for use
    inline void reset() noexcept
    {
        usedChunkCount=0;
        allocatedChunkCount=0;
        maxBucketChunkCount=0;
        minBucketChunkCount=std::numeric_limits<size_t>::max();
        maxBucketUsedChunkCount=0;
        minBucketUsedChunkCount=std::numeric_limits<size_t>::max();
    }

    //! Clear statistics
    inline void clear() noexcept
    {
        usedChunkCount=0;
        allocatedChunkCount=0;
        maxBucketChunkCount=0;
        minBucketChunkCount=0;
        maxBucketUsedChunkCount=0;
        minBucketUsedChunkCount=0;
    }
};

//! Pool configuration.
class PoolConfig
{
    public:

        //! Pool parameters
        struct Parameters
        {
            Parameters(size_t chunkSize,size_t chunkCount=1024):chunkSize(chunkSize),chunkCount(chunkCount)
            {}
            size_t chunkSize;
            size_t chunkCount;
        };

        PoolConfig(
                const Parameters& params
            ) : m_chunkSize(params.chunkSize),
                m_initialChunksPerBucket(params.chunkCount),
                m_dynamicBucketSizeEnable(true),
                m_maxBucketSize(0x1000000)
        {}

        //! Enable/disable dynamic bucket size, defaule true
        inline void setDynamicBucketSizeEnabled(bool enable) noexcept
        {
            m_dynamicBucketSizeEnable=enable;
        }

        //! Check if dynamic bucket size is enabled
        inline bool isDynamicBucketSizeEnabled() const noexcept
        {
            return m_dynamicBucketSizeEnable;
        }

        //! Set max bucket size (in bytes) for dynamic bucket sizing, default 16MBytes
        inline void setMaxBucketSize(size_t size) noexcept
        {
            m_maxBucketSize=size;
        }

        //! Get max bucket size
        inline size_t maxBucketSize() const noexcept
        {
            return m_maxBucketSize;
        }

        //! Get size of a chunk
        inline size_t chunkSize() const noexcept
        {
            return m_chunkSize;
        }

        //! Get initial chunk count pet bucket
        inline size_t initialChunkCountPerBucket() const noexcept
        {
            return m_initialChunksPerBucket;
        }

    private:

        size_t m_chunkSize;
        size_t m_initialChunksPerBucket;
        bool m_dynamicBucketSizeEnable;
        size_t m_maxBucketSize;
};

class PoolWithConfig
{
    public:

        PoolWithConfig(PoolConfig* cfg=nullptr):m_cfg(cfg)
        {}

        //! Enable/disable dynamic bucket size, defaule true
        inline void setDynamicBucketSizeEnabled(bool enable) noexcept
        {
            m_cfg->setDynamicBucketSizeEnabled(enable);
        }

        //! Check if dynamic bucket size is enabled
        inline bool isDynamicBucketSizeEnabled() const noexcept
        {
            return m_cfg->isDynamicBucketSizeEnabled();
        }

        //! Set max bucket size (in bytes) for dynamic bucket sizing, default 16MBytes
        inline void setMaxBucketSize(size_t size) noexcept
        {
            m_cfg->setMaxBucketSize(size);
        }

        //! Get max bucket size
        inline size_t maxBucketSize() const noexcept
        {
            return m_cfg->maxBucketSize();
        }

        //! Get size of a chunk
        inline size_t chunkSize() const noexcept
        {
            return m_cfg->chunkSize();
        }

        //! Get initial chunk count pet bucket
        inline size_t initialChunkCountPerBucket() const noexcept
        {
            return m_cfg->initialChunkCountPerBucket();
        }

        inline static size_t alignedChunkSize(size_t chunkSize) noexcept
        {
            auto size=chunkSize+sizeof(RawBlock);
            size_t remainder=size%alignof(RawBlock);
            if (remainder!=0)
            {
                size+=alignof(RawBlock)-remainder;
            }
            return size;
        }


    protected:

        void setConfig(PoolConfig* cfg)
        {
            m_cfg=cfg;
        }

    private:

        PoolConfig* m_cfg;
};

template <typename PoolT, typename BucketTraitsT>
class Bucket;

//! Base memory pool class
template <typename Traits>
class Pool : public WithTraits<Traits>,
             public PoolWithConfig
{
    public:

        static_assert(sizeof(memorypool::RawBlock)>=8,"Invalid size of RawBlock");

        using BucketT=typename Traits::BucketT;

        //! Ctor
        template <typename ... Args>
        Pool(
            const PoolConfig::Parameters& params,
            Args&& ...traitsArgs
        ) noexcept : WithTraits<Traits>(std::forward<Args>(traitsArgs)...),
                     PoolWithConfig(&m_cfgImpl),
                     m_cfgImpl(params)
        {}

        //! Allocate block of raw data
        inline RawBlock* allocateRawBlock()
        {
            return this->traits().allocateRawBlock();
        }

        //! Deallocate block of raw data
        void deallocateRawBlock(RawBlock* rawBlock) noexcept
        {
            this->traits().deallocateRawBlock(rawBlock);
        }

        //! Get buckets count
        size_t bucketsCount() const noexcept
        {
            return this->traits().bucketsCount();
        }

        //! Get unused buckets count
        size_t unusedBucketsCount() const noexcept
        {
            return this->traits().unusedBucketsCount();
        }

        //! Check if pool is empty
        bool isEmpty() const noexcept
        {
            return this->traits().isEmpty();
        }

        //! Get statistics
        void getStats(Stats& stats) const noexcept
        {
            return this->traits().getStats(stats);
        }

        //! Get actual average allocated size per chunk
        size_t allocatedChunkSize() const noexcept
        {
            return this->traits().allocatedChunkSize();
        }

        /**
         * @brief Clear pool
         *
         * Call it only when pool is empty, i.e. no chunk is used
         */
        inline void clear()
        {
            return this->traits().clear();
        }

        //! Deallocate chunk
        static inline void destroy(RawBlock* rawBlock) noexcept
        {
            if (rawBlock->bucket()!=nullptr)
            {
                auto bucket=static_cast<BucketT*>(rawBlock->bucket());
                bucket->pool().deallocateRawBlock(rawBlock);
            }
            else
            {
                ::operator delete(rawBlock);
            }
        }

    private:

        PoolConfig m_cfgImpl;
};

template <typename ObjectT,typename PoolT>
class ObjectPool : public PoolT
{
    public:

        static_assert(alignof(ObjectT)<=alignof(RawBlock),"Unsupported alignement");

        template <typename ... Args>
        ObjectPool(Args&& ...args) : PoolT(std::forward<Args>(args)...)
        {
            Assert(this->chunkSize()==sizeof(ObjectT),"Chunk size must be the same as sizeof(ObjectT)");
        }

        template <typename ... Args>
        ObjectT* create(Args&& ...args)
        {
            auto block=this->allocateRawBlock();
            auto obj=new(block->data()) ObjectT(std::forward<Args>(args)...);
            return obj;
        }

        void destroy(ObjectT *p) noexcept
        {
            p->~ObjectT();
            auto rawBlock=reinterpret_cast<memorypool::RawBlock*>(reinterpret_cast<uintptr_t>(p)-sizeof(memorypool::RawBlock));
            this->deallocateRawBlock(rawBlock);
        }
};

//---------------------------------------------------------------
        } // namespace memorypool
HATN_COMMON_NAMESPACE_END
#endif // HATNMEMORYPOOL__H
