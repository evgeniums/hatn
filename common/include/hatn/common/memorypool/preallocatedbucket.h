/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/memorypool/preallocatedbucket.h
  *
  *     Bucket with pre-allocated memory that can be reused for allocation/deallocation of data blocks
  *
  */

/****************************************************************************/

#ifndef HATNPREALLOCATEDBUCKET_H
#define HATNPREALLOCATEDBUCKET_H

#include <atomic>

#include <hatn/common/common.h>
#include <hatn/common/locker.h>
#include <hatn/common/objecttraits.h>

#include <hatn/common/memorypool/bucket.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace memorypool {

template <typename PoolT, typename SyncT> class PreallocatedBucket;

//! Traits for bucket with pre-allocated memory that can be reused for allocation/deallocation of data blocks
template <typename PoolT, typename SyncT>
class PreallocatedBucketTraits : public BucketTraitsBase<PreallocatedBucket<PoolT,SyncT>>
{
    public:

        using BucketT=PreallocatedBucket<PoolT,SyncT>;
        using BaseT=BucketTraitsBase<BucketT>;

        //! Ctor
        PreallocatedBucketTraits(
            BucketT* bucket,
            size_t chunkSize,
            size_t chunkCount
        );

        //! Dtor
        ~PreallocatedBucketTraits();

        PreallocatedBucketTraits(const PreallocatedBucketTraits&)=delete;
        PreallocatedBucketTraits(PreallocatedBucketTraits&&) =delete;
        PreallocatedBucketTraits& operator=(const PreallocatedBucketTraits&)=delete;
        PreallocatedBucketTraits& operator=(PreallocatedBucketTraits&&) =delete;

        void* allocate();
        void deallocate(void* chunk) noexcept;

        inline size_t freeCount() const noexcept
        {
            return m_freeCount.load(std::memory_order_relaxed);
        }

        inline bool isEmpty() const noexcept
        {
            bool empty=freeCount()==m_chunkCount;
            return empty;
        }

        inline size_t chunkCount() const noexcept
        {
           return m_chunkCount;
        }

        inline BucketT* next() const noexcept
        {
           return m_next.load(std::memory_order_acquire);
        }
        inline void setNext(BucketT* next) noexcept
        {
           m_next.store(next,std::memory_order_release);
        }

    private:

        typename SyncT::template Atomic<BucketT*> m_next;

        size_t m_chunkCount;
        typename SyncT::template Atomic<int> m_freeCount;
        typename SyncT::template Atomic<size_t> m_writeCursor;
        typename SyncT::template Atomic<size_t> m_readCursor;
        typename SyncT::template Atomic<size_t> m_firstReadCursor;
        typename SyncT::SpinLock m_deallocateLock;

        size_t m_alignedChunkSize;
        char* m_data;
        std::vector<typename SyncT::template Atomic<void*>> m_freeChunks;
};

//! Bucket with pre-allocated memory that can be reused for allocation/deallocation of data blocks
template <typename PoolT,typename SyncT>
class PreallocatedBucket : public Bucket<PoolT,PreallocatedBucketTraits<PoolT,SyncT>>
{
    public:

        PreallocatedBucket(
            PoolT& pool,
            size_t chunkSize,
            size_t chunkCount
        ) noexcept : Bucket<PoolT,PreallocatedBucketTraits<PoolT,SyncT>>(this,pool,chunkSize,chunkCount)
        {}

        void* allocate();
        void deallocate(void* chunk) noexcept;
        size_t freeCount() const noexcept;
        bool isEmpty() const noexcept;

        size_t chunkCount() const noexcept;
        PreallocatedBucket* next() const noexcept;
        void setNext(PreallocatedBucket* next) noexcept;
};

//---------------------------------------------------------------
        } // namespace memory
    HATN_COMMON_NAMESPACE_END
#endif // HATNPREALLOCATEDBUCKET_H
