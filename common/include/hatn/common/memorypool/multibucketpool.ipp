/*
   Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/memorypool/multibucketpool.ipp
  *
  *     Pool with list of buckets of allocated memory
  *
  */

/****************************************************************************/

#ifndef HATNMULTIBUCKETPOOL_IPP
#define HATNMULTIBUCKETPOOL_IPP

#include <hatn/common/memorypool/multibucketpool.h>
#include <hatn/common/memorypool/preallocatedbucket.ipp>
#include <hatn/common/makeshared.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace memorypool {

/********************** MultiBucketPoolTraits **************************/

//---------------------------------------------------------------
template <typename SyncInvoker>
MultiBucketPoolTraits<SyncInvoker>::MultiBucketPoolTraits(
        Thread* thread,
        PoolT* pool,
        size_t initialChunksPerBucket
    )  : WithTraits<SyncInvoker>(thread),
         m_pool(pool),
         m_gbCollectTimer(makeShared<AsioDeadlineTimer>(thread)),
         m_headBucket(nullptr),
         m_bucketsCount(0),
         m_scheduleDropCount(0),
         m_scheduleDropChunkCount(0),
         m_nextBucketChunkCount(initialChunksPerBucket),
         m_garbageCollectorEnable(false),
         m_garbageCollectorPeriod(15000),
         m_createdBucketSinceLastGbCollecting(false),
         m_freedBucketSinceLastGbCollecting(false)
{
    m_gbCollectTimer->setSingleShot(false);
    setGarbageCollectorEnabled(true);
}

//---------------------------------------------------------------
template <typename SyncInvoker>
MultiBucketPoolTraits<SyncInvoker>::~MultiBucketPoolTraits()
{
    m_gbCollectTimer->cancel();
    doClear();
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void MultiBucketPoolTraits<SyncInvoker>::clear()
{
    HATN_DEBUG_LVL(mempool,3,"clear in");

    this->traits().template execSync<void>(
        [this]()
        {
            doClear();
        }
    );

    HATN_DEBUG_LVL(mempool,3,"clear out");
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void MultiBucketPoolTraits<SyncInvoker>::doClear()
{
    HATN_DEBUG_LVL(mempool,3,"clear begin");

    auto bucket=m_headBucket.load(std::memory_order_relaxed);
    while (bucket!=nullptr)
    {
        auto tmp=bucket;
        bucket=bucket->next();
        delete tmp;
    }
    for (auto&& it:m_dropSchedule)
    {
        delete it.first;
    }

    m_headBucket.store(nullptr,std::memory_order_relaxed);
    m_bucketsCount.store(0,std::memory_order_relaxed);
    m_scheduleDropCount.store(0,std::memory_order_relaxed);
    m_scheduleDropChunkCount.store(0,std::memory_order_relaxed);
    m_dropSchedule.clear();
    m_createdBucketSinceLastGbCollecting=false;
    m_freedBucketSinceLastGbCollecting.store(false,std::memory_order_relaxed);

    HATN_DEBUG_LVL(mempool,3,"clear end");
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void* MultiBucketPoolTraits<SyncInvoker>::allocateChunk(BucketPtrT &actualBucket, bool requestSync)
{
    void* chunk=nullptr;
    auto each=[&chunk,&actualBucket](BucketT* bucket)
    {
        chunk=bucket->allocate();
        if (chunk!=nullptr)
        {
            actualBucket=bucket;
        }
        return (chunk==nullptr);
    };
    iterateBuckets(each);
    if (chunk==nullptr && requestSync)
    {
        chunk=allocateChunkSync(actualBucket);
    }
    return chunk;
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void* MultiBucketPoolTraits<SyncInvoker>::allocateChunkSync(BucketPtrT &actualBucket)
{
    AllocateResult result=allocateBucketAndChunkSync();
    actualBucket=result.bucket;
    return result.chunk;
}

//---------------------------------------------------------------
template <typename SyncInvoker>
typename MultiBucketPoolTraits<SyncInvoker>::AllocateResult MultiBucketPoolTraits<SyncInvoker>::allocateBucketAndChunkSync()
{
    return this->traits().template execSync<AllocateResult>(
        [this]()
        {
            // iterate to the last bucket in the chain
            auto nextBucket=m_headBucket.load(std::memory_order_acquire);
            BucketT* bucket=nullptr;
            size_t freeChunks=0;
            size_t minFreeChunks=m_pool->initialChunkCountPerBucket()/2;
            while (nextBucket!=nullptr)
            {
                bucket=nextBucket;

                // check if bucket has enough free chunks for allocation (enough means at least half of initial chunk count)
                freeChunks+=bucket->freeCount();
                if (freeChunks>=minFreeChunks)
                {
                    HATN_DEBUG_LVL(mempool,2,"Allocate in existing bucket");

                    // try to allocate chunk
                    BucketT* allocatedBucket=nullptr;
                    auto chunk=allocateChunk(allocatedBucket,false);
                    if (chunk!=nullptr)
                    {
                        return AllocateResult{chunk,allocatedBucket};
                    }
                }
                nextBucket=bucket->next();
            }

            BucketT* newBucket=nullptr;

            // try to reuse bucket scheduled for dropping
            auto minSizeIt=m_dropSchedule.begin();
            if (minSizeIt!=m_dropSchedule.end())
            {
                // find minimal suitable bucket
                auto minChunkCount=minSizeIt->first->chunkCount();
                for (auto it=minSizeIt;it!=m_dropSchedule.end();)
                {
                    auto chunkCount=it->first->chunkCount();
                    if (chunkCount<minChunkCount && chunkCount>=minFreeChunks)
                    {
                        minChunkCount=chunkCount;
                        minSizeIt=it;
                    }
                    ++it;
                }

                // reuse bucket
                if (minSizeIt->first->chunkCount()>=minFreeChunks)
                {
                    HATN_DEBUG_LVL(mempool,1,"Reuse bucket");

                    newBucket=minSizeIt->first;
                    m_dropSchedule.erase(minSizeIt);
                    m_scheduleDropChunkCount.fetch_sub(newBucket->chunkCount(),std::memory_order_release);
                    m_scheduleDropCount.fetch_sub(1,std::memory_order_release);
                }
            }

            // check if new bucket required
            if (newBucket==nullptr)
            {
                HATN_DEBUG_LVL(mempool,2,"Create new bucket");

                // calculate size of the next bucket
                size_t chunkCount=m_nextBucketChunkCount;
                if (m_pool->isDynamicBucketSizeEnabled())
                {
                    auto nextChunkCount=chunkCount*2;
                    if ((nextChunkCount*m_pool->allocatedChunkSize())<=m_pool->maxBucketSize())
                    {
                        m_nextBucketChunkCount=nextChunkCount;
                    }
                }

                // create new bucket
                newBucket=new BucketT(*m_pool,m_pool->chunkSize(),chunkCount);
                m_bucketsCount.fetch_add(1,std::memory_order_relaxed);

            }

            // allocate chunk
            auto chunk=newBucket->allocate();

            // add bucket to chain
            if (bucket!=nullptr)
            {
                bucket->setNext(newBucket);
            }
            else
            {
                m_headBucket.store(newBucket,std::memory_order_release);
            }
            m_createdBucketSinceLastGbCollecting=true;

            // return result
            return AllocateResult{chunk,newBucket};
        }
    );
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void MultiBucketPoolTraits<SyncInvoker>::iterateBuckets(const std::function<bool (BucketT* bucket)>& each)
{
    for (auto bucket=m_headBucket.load(std::memory_order_acquire);bucket!=nullptr;)
    {
        if (!each(bucket))
        {
            return;
        }
        bucket=bucket->next();
    }
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void MultiBucketPoolTraits<SyncInvoker>::garbageCollectorSync(bool force)
{
    HATN_DEBUG_LVL(mempool,3,"garbageCollectorSync in");

    this->traits().template execSync<void>(
        [this,force]()
        {
            HATN_DEBUG_LVL(mempool,3,"garbageCollectorSync run check drop");
            for (auto it=m_dropSchedule.begin();it!=m_dropSchedule.end();)
            {
                if (
                        it->first->isEmpty()
                        &&
                        ((m_pool->elapsedSecs()-it->second)>=m_pool->dropBucketDelay())
                   )
                {
                    HATN_DEBUG_LVL(mempool,1,"Delete bucket");
                    m_scheduleDropChunkCount.fetch_sub(it->first->chunkCount(),std::memory_order_release);

                    delete it->first;
                    it=m_dropSchedule.erase(it);
                    m_scheduleDropCount.fetch_sub(1,std::memory_order_release);
                }
                else
                {
                    ++it;
                }
            }

            // check if some buckets were removed and no buckets were added since last run
            if (
               force
                    ||
                    (
                    !m_createdBucketSinceLastGbCollecting
                         &&
                     m_freedBucketSinceLastGbCollecting.load(std::memory_order_acquire)
                    )
              )
            {
                HATN_DEBUG_LVL(mempool,3,"garbageCollectorSync run begin");

                bool tmpTrue=true;
                m_freedBucketSinceLastGbCollecting.compare_exchange_strong(tmpTrue,false,std::memory_order_acquire);

                auto count=static_cast<size_t>(m_bucketsCount.load(std::memory_order_relaxed));
                std::vector<BucketT*> buckets;
                buckets.reserve(count+16);
                std::vector<BucketT*> dropBuckets;
                dropBuckets.reserve(count+16);

                // construct buckets array
                BucketT* bucket=m_headBucket.load(std::memory_order_relaxed);
                while (bucket!=nullptr)
                {
                    buckets.push_back(bucket);
                    bucket=bucket->next();
                }
                count=buckets.size();

                // max chunk per bucket for the next bucket creation
                size_t maxChunkCount=m_pool->initialChunkCountPerBucket();

                // remove empty buckets
                for (int j=static_cast<int>(count)-1;j>=0;j--)
                {
                    bool removed=false;
                    bucket=buckets[j];
                    {
                        if (!bucket->isEmpty())
                        {
                            continue;
                        }

                        BucketT* prevBucket=nullptr;
                        if (j!=0)
                        {
                            prevBucket=buckets[j-1];
                        }
                        if (prevBucket)
                        {
                            prevBucket->setNext(bucket->next());
                        }
                        else
                        {
                            m_headBucket.store(bucket->next(),std::memory_order_release);
                        }
                    }

                    removed=true;
                    bucket->setNext(nullptr);
                    m_dropSchedule.emplace(std::make_pair(bucket,m_pool->elapsedSecs()));
                    m_bucketsCount.fetch_sub(1,std::memory_order_release);
                    m_scheduleDropChunkCount.fetch_add(bucket->chunkCount(),std::memory_order_release);
                    m_scheduleDropCount.fetch_add(1,std::memory_order_release);

                    HATN_DEBUG_LVL(mempool,1,"Drop bucket");

                    if (!removed)
                    {
                        maxChunkCount=(std::max)(bucket->chunkCount(),maxChunkCount);
                    }
                }
                if (m_pool->isDynamicBucketSizeEnabled())
                {
                    m_nextBucketChunkCount=maxChunkCount;
                }
            }
            else
            {
                m_createdBucketSinceLastGbCollecting=false;
            }

            HATN_DEBUG_LVL(mempool,3,"garbageCollectorSync run end");
        }
    );

    HATN_DEBUG_LVL(mempool,3,"garbageCollectorSync out");
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void MultiBucketPoolTraits<SyncInvoker>::setGarbageCollectorEnabled(bool enable)
{
    m_gbCollectTimer->cancel();
    m_garbageCollectorEnable=enable;
    if (enable)
    {
        m_gbCollectTimer->setPeriodUs(m_garbageCollectorPeriod*1000);
        m_gbCollectTimer->start(
            [this](TimerTypes::Status status)
            {
                if (status==TimerTypes::Timeout)
                {
                    garbageCollectorSync();
                }
                HATN_DEBUG_LVL(mempool,3,"garbageCollector timer handler done");
                return true;
            }
        );
    }
}

//---------------------------------------------------------------
template <typename SyncInvoker>
RawBlock* MultiBucketPoolTraits<SyncInvoker>::allocateRawBlock()
{
    BucketT* bucket=nullptr;
    void* chunk=allocateChunk(bucket);
    auto block=new(chunk) RawBlock();
    block->setBucket(bucket);
    return block;
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void MultiBucketPoolTraits<SyncInvoker>::deallocateRawBlock(RawBlock *rawBlock) noexcept
{
    auto bucket=static_cast<BucketT*>(rawBlock->bucket());
    rawBlock->~RawBlock();
    bucket->deallocate(rawBlock);
    if (bucket->isEmpty())
    {
        // no occupied chunks left in the bucket
        m_freedBucketSinceLastGbCollecting.store(true,std::memory_order_release);
    }
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void MultiBucketPoolTraits<SyncInvoker>::setGarbageCollectorPeriod(uint32_t milliseconds)
{
    m_gbCollectTimer->cancel();
    m_garbageCollectorPeriod=milliseconds;
    setGarbageCollectorEnabled(isGarbageCollectorEnabled());
}

//---------------------------------------------------------------
template <typename SyncInvoker>
size_t MultiBucketPoolTraits<SyncInvoker>::bucketsCount() const noexcept
{
    return m_bucketsCount.load(std::memory_order_relaxed)+m_scheduleDropCount.load(std::memory_order_relaxed);
}

//---------------------------------------------------------------
template <typename SyncInvoker>
size_t MultiBucketPoolTraits<SyncInvoker>::unusedBucketsCount() const noexcept
{
    return m_scheduleDropCount.load(std::memory_order_relaxed);
}

//---------------------------------------------------------------
template <typename SyncInvoker>
bool MultiBucketPoolTraits<SyncInvoker>::isEmpty() const noexcept
{
    bool empty=m_bucketsCount.load(std::memory_order_relaxed)==0;
    if (!empty)
    {
        auto each=[&empty](BucketT* bucket)
        {
            empty=bucket->isEmpty();
            return empty;
        };
        const_cast<MultiBucketPoolTraits*>(this)->iterateBuckets(each);
    }
    return empty;
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void MultiBucketPoolTraits<SyncInvoker>::getStats(Stats &stats) const noexcept
{
    stats.reset();

    auto each=[&stats](BucketT* bucket)
    {
        stats.allocatedChunkCount+=bucket->chunkCount();

        stats.maxBucketChunkCount=(std::max)(stats.maxBucketChunkCount,bucket->chunkCount());
        stats.minBucketChunkCount=(std::min)(stats.minBucketChunkCount,bucket->chunkCount());

        auto usedChunks=bucket->chunkCount()-bucket->freeCount();
        stats.usedChunkCount+=usedChunks;

        stats.maxBucketUsedChunkCount=(std::max)(stats.maxBucketUsedChunkCount,usedChunks);
        stats.minBucketUsedChunkCount=(std::min)(stats.minBucketUsedChunkCount,usedChunks);

        return true;
    };
    const_cast<MultiBucketPoolTraits*>(this)->iterateBuckets(each);

    if (stats.minBucketUsedChunkCount>stats.maxBucketUsedChunkCount)
    {
        stats.minBucketUsedChunkCount=stats.maxBucketUsedChunkCount;
    }
    if (stats.minBucketChunkCount>stats.maxBucketChunkCount)
    {
        stats.minBucketChunkCount=stats.maxBucketChunkCount;
    }
    stats.allocatedChunkCount+=m_scheduleDropChunkCount.load(std::memory_order_relaxed);
}

//---------------------------------------------------------------
template <typename SyncInvoker>
size_t MultiBucketPoolTraits<SyncInvoker>::allocatedChunkSize() const noexcept
{
    return PoolWithConfig::alignedChunkSize(m_pool->chunkSize())+
            sizeof(typename SyncInvoker::SyncT::template Atomic<void*>);
}

//---------------------------------------------------------------
template <typename SyncInvoker>
void MultiBucketPoolTraits<SyncInvoker>::runGarbageCollector()
{
    garbageCollectorSync(true);
}

//---------------------------------------------------------------
} // namespace memorypool

HATN_COMMON_NAMESPACE_END

#endif // HATNMULTIBUCKETPOOL_IPP
