/*
   Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/memorypool/preallocatedbucket.ipp
  *
  *     Bucket with pre-allocated memory that can be reused for allocation/deallocation of data blocks
  *
  */

/****************************************************************************/

#ifndef HATNPREALLOCATEDBUCKET_IPP
#define HATNPREALLOCATEDBUCKET_IPP

#include <hatn/common/memorypool/pool.h>

#include <hatn/common/memorypool/preallocatedbucket.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace memorypool {

/********************** PreallocatedBucketTraits **************************/

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
PreallocatedBucketTraits<PoolT,SyncT>::PreallocatedBucketTraits(
        PreallocatedBucket<PoolT,SyncT>* bucket,
        size_t chunkSize,
        size_t chunkCount
    ) : BaseT(bucket),
        m_next(nullptr),
        m_chunkCount(chunkCount),
        m_freeCount(static_cast<int>(chunkCount)),
        m_writeCursor(0),
        m_readCursor(0),
        m_firstReadCursor(0),
        m_alignedChunkSize(PoolWithConfig::alignedChunkSize(chunkSize)),
        m_data(new char[chunkCount*m_alignedChunkSize]),
        m_freeChunks(chunkCount)
{
}

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
PreallocatedBucketTraits<PoolT,SyncT>::~PreallocatedBucketTraits()
{
    delete [] m_data;
}

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
void* PreallocatedBucketTraits<PoolT,SyncT>::allocate()
{
    auto freeCount=m_freeCount.fetch_sub(1,std::memory_order_acq_rel);
    if (freeCount<=0)
    {
        // no available chunks
        m_freeCount.fetch_add(1,std::memory_order_acq_rel);
        return nullptr;
    }

    auto cursor=m_firstReadCursor.fetch_add(1,std::memory_order_acq_rel);
    if (cursor<m_chunkCount)
    {
        // use fresh chunk from initial array
        return reinterpret_cast<void*>(m_data+m_alignedChunkSize*cursor);
    }
    m_firstReadCursor.fetch_sub(1,std::memory_order_relaxed);

    // reuse chunk that was deallocated earlier
    cursor=m_readCursor.fetch_add(1,std::memory_order_acq_rel)%m_chunkCount;
    return SyncT::template atomic_load_explicit<void*>(&m_freeChunks[cursor],std::memory_order_consume);
}

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
void PreallocatedBucketTraits<PoolT,SyncT>::deallocate(void *chunk) noexcept
{
    m_deallocateLock.lock();
    auto cursor=m_writeCursor.fetch_add(1,std::memory_order_acq_rel)%m_chunkCount;
    SyncT::template atomic_store_explicit<void*>(&m_freeChunks[cursor],chunk,std::memory_order_relaxed);
    m_freeCount.fetch_add(1,std::memory_order_acq_rel);
    m_deallocateLock.unlock();
}

/********************** PreallocatedBucket **************************/

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
void* PreallocatedBucket<PoolT,SyncT>::allocate()
{
    return this->traits().allocate();
}

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
void PreallocatedBucket<PoolT,SyncT>::deallocate(void* chunk) noexcept
{
    this->traits().deallocate(chunk);
}

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
size_t PreallocatedBucket<PoolT,SyncT>::freeCount() const noexcept
{
    return this->traits().freeCount();
}

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
bool PreallocatedBucket<PoolT,SyncT>::isEmpty() const noexcept
{
    return this->traits().isEmpty();
}

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
size_t PreallocatedBucket<PoolT,SyncT>::chunkCount() const noexcept
{
   return this->traits().chunkCount();
}

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
PreallocatedBucket<PoolT,SyncT>* PreallocatedBucket<PoolT,SyncT>::next() const noexcept
{
    return this->traits().next();
}

//---------------------------------------------------------------
template <typename PoolT, typename SyncT>
void PreallocatedBucket<PoolT,SyncT>::setNext(PreallocatedBucket<PoolT,SyncT>* next) noexcept
{
    this->traits().setNext(next);
}

//---------------------------------------------------------------
} // namespace memorypool

HATN_COMMON_NAMESPACE_END

#endif // HATNPREALLOCATEDBUCKET_IPP
