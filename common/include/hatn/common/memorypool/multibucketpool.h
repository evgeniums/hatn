/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/memorypool/multibucketpool.h
  *
  *     Pool with list of buckets of allocated memory
  *
  */

/****************************************************************************/

#ifndef HATNMULTIBUCKETPOOL_H
#define HATNMULTIBUCKETPOOL_H

#include <atomic>
#include <memory>
#include <functional>

#include <hatn/common/common.h>
#include <hatn/common/objectid.h>
#include <hatn/common/thread.h>
#include <hatn/common/locker.h>
#include <hatn/common/sharedlocker.h>
#include <hatn/common/asiotimer.h>
#include <hatn/common/syncinvoker.h>
#include <hatn/common/elapsedtimer.h>

#include <hatn/common/memorypool/pool.h>
#include <hatn/common/memorypool/preallocatedbucket.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace memorypool {

template <typename SyncInvoker> class MultiBucketPool;

template <typename SyncInvoker>
class MultiBucketPoolTraits : public WithTraits<SyncInvoker>
{
    public:

        using BucketT=PreallocatedBucket<MultiBucketPool<SyncInvoker>,typename SyncInvoker::SyncT>;
        using BucketPtrT=BucketT*;
        using PoolT=MultiBucketPool<SyncInvoker>;

        //! Ctor
        MultiBucketPoolTraits(
            Thread* thread,
            PoolT* pool,
            size_t initialChunksPerBucket
        );

        ~MultiBucketPoolTraits();

        MultiBucketPoolTraits(const MultiBucketPoolTraits& other)=delete;
        MultiBucketPoolTraits(MultiBucketPoolTraits&& other)=delete;
        MultiBucketPoolTraits& operator=(const MultiBucketPoolTraits&)=delete;
        MultiBucketPoolTraits& operator=(MultiBucketPoolTraits&&) =delete;

        //! Get buckets count
        size_t bucketsCount() const noexcept;

        //! Get unused buckets count
        size_t unusedBucketsCount() const noexcept;

        //! Check if pool is empty
        bool isEmpty() const noexcept;

        //! Get statistics
        void getStats(Stats& stats) const noexcept;

        //! Allocate chunk
        RawBlock* allocateRawBlock();

        //! Deallocate block of raw data
        void deallocateRawBlock(RawBlock* rawBlock) noexcept;

        void setGarbageCollectorEnabled(bool enable);
        bool isGarbageCollectorEnabled() const noexcept
        {
            return m_garbageCollectorEnable;
        }

        void setGarbageCollectorPeriod(uint32_t milliseconds);
        inline uint32_t garbageCOllectorPeriod() const noexcept
        {
            return m_garbageCollectorPeriod;
        }

        void clear();

        //! Get actual average allocated size per chunk
        size_t allocatedChunkSize() const noexcept;

        void runGarbageCollector();

    private:

        void doClear();

        struct AllocateResult
        {
            void* chunk;
            BucketT* bucket;
        };

        void* allocateChunk(BucketPtrT& bucket, bool requestSync=true);
        void* allocateChunkSync(BucketPtrT& bucket);
        AllocateResult allocateBucketAndChunkSync();

        void iterateBuckets(const std::function<bool (BucketT* bucket)>& each);
        void garbageCollectorSync(bool force=false);

        PoolT* m_pool;
        AsioDeadlineTimer m_gbCollectTimer;

        typename SyncInvoker::SyncT::template Atomic<BucketT*> m_headBucket;
        typename SyncInvoker::SyncT::template Atomic<int> m_bucketsCount;
        typename SyncInvoker::SyncT::template Atomic<int> m_scheduleDropCount;
        typename SyncInvoker::SyncT::template Atomic<size_t> m_scheduleDropChunkCount;

        size_t m_nextBucketChunkCount;

        bool m_garbageCollectorEnable;
        uint32_t m_garbageCollectorPeriod;

        bool m_createdBucketSinceLastGbCollecting;
        typename SyncInvoker::SyncT::template Atomic<bool> m_freedBucketSinceLastGbCollecting;

        std::map<BucketT*,uint32_t> m_dropSchedule;
};

//! Pool with list of buckets of allocated memory
template <class SyncInvoker>
class MultiBucketPool : public WithThread,
                        public Pool<MultiBucketPoolTraits<SyncInvoker>>
{
    public:

        //! Ctor
        MultiBucketPool(
            Thread* thread,
            const PoolContext::Parameters& params
        ) noexcept : WithThread(thread),
                     Pool<MultiBucketPoolTraits<SyncInvoker>>(params,thread,this,params.chunkCount),
                     m_dropBucketDelay(5)
        {
        }

        //! Ctor
        MultiBucketPool(
            const PoolContext::Parameters& params
        ) noexcept : MultiBucketPool(Thread::currentThreadOrMain(),params)
        {
        }

        inline void setGarbageCollectorEnabled(bool enable)
        {
            this->traits().setGarbageCollectorEnabled(enable);
        }
        inline bool isGarbageCollectorEnabled() const noexcept
        {
            return this->traits().isGarbageCollectorEnabled();
        }
        inline void setGarbageCollectorPeriod(uint32_t milliseconds)
        {
            this->traits().setGarbageCollectorPeriod(milliseconds);
        }
        inline uint32_t garbageCOllectorPeriod() const noexcept
        {
            return this->traits().garbageCOllectorPeriod();
        }

        inline void runGarbageCollector()
        {
            this->traits().runGarbageCollector();
        }

        inline static size_t alignedChunkSize(size_t chunkSize) noexcept
        {
            return MultiBucketPoolTraits<SyncInvoker>::alignedChunkSize(chunkSize);
        }

        inline uint32_t elapsedSecs() const noexcept
        {
            return m_elapsed.elapsedSecs();
        }

        inline uint32_t dropBucketDelay() const noexcept
        {
            return m_dropBucketDelay;
        }
        inline void setDropBucketDelay(uint32_t val) noexcept
        {
            m_dropBucketDelay=val;
        }

    private:

        uint32_t m_dropBucketDelay;
        ElapsedTimer m_elapsed;
};

using UnsynchronizedPool=MultiBucketPool<UnsynchronizedInvoker>;
using SynchronizedMutexPool=MultiBucketPool<SynchronizedMutexInvoker>;
using SynchronizedThreadPool=MultiBucketPool<SynchronizedThreadInvoker>;

//---------------------------------------------------------------
        } // namespace memory
    HATN_COMMON_NAMESPACE_END
#endif // HATNMULTIBUCKETPOOL_H
