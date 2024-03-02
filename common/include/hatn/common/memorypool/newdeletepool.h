/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/memorypool/newdeletepool.h
  *
  *     Simple memory pool using new and delete operators for each chunk
  *
  */

/****************************************************************************/

#ifndef HATNNEWDELETEPOOL_H
#define HATNNEWDELETEPOOL_H

#include <atomic>

#include <hatn/common/common.h>
#include <hatn/common/classuid.h>

#include <hatn/common/memorypool/pool.h>
#include <hatn/common/memorypool/bucket.h>
#include <hatn/common/pmr/poolmemoryresource.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace memorypool {

class NewDeletePool;
class NewDeletePoolBucket;

using NewDeletePoolBucketTraits=BucketTraitsBase<NewDeletePoolBucket>;
class NewDeletePoolBucket : public Bucket<NewDeletePool,NewDeletePoolBucketTraits>
{
    public:

        using Bucket<NewDeletePool,NewDeletePoolBucketTraits>::Bucket;
};

//! Simple memory pool using new and delete operators for each chunk
class NewDeletePoolTraits
{
    public:

        using BucketT=NewDeletePoolBucket;

        //! Ctor
        NewDeletePoolTraits(
            NewDeletePool* pool
        ) noexcept : m_pool(pool),
                     m_chunkCount(0)
        {}

        ~NewDeletePoolTraits()=default;
        NewDeletePoolTraits(const NewDeletePoolTraits&)=delete;
        NewDeletePoolTraits(NewDeletePoolTraits&&) =delete;
        NewDeletePoolTraits& operator=(const NewDeletePoolTraits&)=delete;
        NewDeletePoolTraits& operator=(NewDeletePoolTraits&&) =delete;

        //! Get buckets count
        inline size_t bucketsCount() const noexcept
        {
            return m_chunkCount;
        }

        //! Get unused buckets count
        size_t unusedBucketsCount() const noexcept
        {
            return 0;
        }

        //! Check if pool is empty
        inline bool isEmpty() const noexcept
        {
            return m_chunkCount.load(std::memory_order_relaxed)==0;
        }

        //! Get statistics
        inline void getStats(Stats& stats) const noexcept;

        //! Allocate chunk
        inline RawBlock* allocateRawBlock();

        //! Deallocate block of raw data
        inline void deallocateRawBlock(RawBlock* rawBlock) noexcept;

        //! Get actual average allocated size per chunk
        inline size_t allocatedChunkSize() const noexcept;

        inline void clear()
        {
            if (m_chunkCount!=0)
            {
                HATN_ERROR(mempool,"clear() is not supported by NewDeletePool, memory leak is possible");
            }
        }

    private:

        NewDeletePool* m_pool;
        std::atomic<size_t> m_chunkCount;
};

class NewDeletePool : public Pool<NewDeletePoolTraits>
{
    public:

        NewDeletePool(
            const PoolContext::Parameters& params
        ) noexcept :
            Pool(params,this),
            m_bucket(&m_bucket,*this)
        {}

        ~NewDeletePool()
        {
            if (!isEmpty())
            {
                HATN_ERROR(mempool,
                          HATN_FORMAT("Memory leak in NewDeletePool: leaked {} blocks of size {}, total {} bytes",
                                     traits().bucketsCount(),chunkSize(),traits().bucketsCount()*chunkSize())
                          );
            }
        }
        NewDeletePool(const NewDeletePool&)=delete;
        NewDeletePool(NewDeletePool&&) =delete;
        NewDeletePool& operator=(const NewDeletePool&)=delete;
        NewDeletePool& operator=(NewDeletePool&&) =delete;

        BucketT* bucket() const noexcept
        {
            return &(const_cast<NewDeletePool*>(this)->m_bucket);
        }

    private:

        BucketT m_bucket;
};

inline RawBlock* NewDeletePoolTraits::allocateRawBlock()
{
    m_chunkCount.fetch_add(1,std::memory_order_relaxed);
    auto block=static_cast<RawBlock*>(::operator new(m_pool->chunkSize()+sizeof(RawBlock)));
    block->setBucket(m_pool->bucket());
    return block;
}

//! Deallocate chunk
inline void NewDeletePoolTraits::deallocateRawBlock(RawBlock* rawBlock) noexcept
{
    m_chunkCount.fetch_sub(1,std::memory_order_relaxed);
    ::operator delete(rawBlock);
}

//! Get statistics
inline void NewDeletePoolTraits::getStats(Stats& stats) const noexcept
{
    stats.reset();
    stats.allocatedChunkCount=m_chunkCount.load(std::memory_order_relaxed);
    stats.usedChunkCount=stats.allocatedChunkCount;
    size_t bucketChunkCount=(stats.allocatedChunkCount==0)?0:1;
    stats.maxBucketChunkCount=bucketChunkCount;
    stats.minBucketChunkCount=bucketChunkCount;
    stats.maxBucketUsedChunkCount=bucketChunkCount;
    stats.minBucketUsedChunkCount=bucketChunkCount;
}

//! Get actual average allocated size per chunk
inline size_t NewDeletePoolTraits::allocatedChunkSize() const noexcept
{
    return m_pool->chunkSize();
}

//---------------------------------------------------------------
using NewDeletePoolResource=pmr::PoolMemoryResource<memorypool::NewDeletePool>;

//---------------------------------------------------------------
        } // namespace memorypool
    HATN_COMMON_NAMESPACE_END
#endif // HATNNEWDELETEPOOL_H
