#include <boost/test/unit_test.hpp>

#include <hatn/test/multithreadfixture.h>

#include <hatn/common/memorypool/multibucketpool.ipp>

#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/pmr/singlepoolmemoryresource.h>

#include <hatn/common/elapsedtimer.h>
#include <hatn/common/makeshared.h>

HATN_USING
HATN_COMMON_USING
using namespace HATN_COMMON_NAMESPACE::memorypool;
using namespace HATN_COMMON_NAMESPACE::pmr;

HATN_TEST_USING

namespace {

struct Env : public HATN_TEST_NAMESPACE::MultiThreadFixture
{
    Env()
    {
        std::vector<std::string> debugModule={"mempool;debug;1"};
        Logger::configureModules(debugModule);

        if (!::HATN_COMMON_NAMESPACE::Logger::isRunning())
        {
            auto handler=[](const ::HATN_COMMON_NAMESPACE::FmtAllocatedBufferChar &s)
            {
                std::cout<<::HATN_COMMON_NAMESPACE::lib::toStringView(s)<<std::endl;
            };

            ::HATN_COMMON_NAMESPACE::Logger::setDefaultVerbosity(LoggerVerbosity::INFO);
            ::HATN_COMMON_NAMESPACE::Logger::setFatalTracing(false);
            ::HATN_COMMON_NAMESPACE::Logger::setOutputHandler(handler);
            ::HATN_COMMON_NAMESPACE::Logger::setFatalLogHandler(handler);
            ::HATN_COMMON_NAMESPACE::Logger::start(true);
        }
    }

    ~Env()
    {
        ::HATN_COMMON_NAMESPACE::Logger::stop();
        Logger::resetModules();
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};

}

BOOST_AUTO_TEST_SUITE(TestMemoryPool)

static void checkStats(const Stats& sample, const Stats& actual)
{
    BOOST_CHECK_EQUAL(sample.allocatedChunkCount,actual.allocatedChunkCount);
    BOOST_CHECK_EQUAL(sample.usedChunkCount,actual.usedChunkCount);

    BOOST_CHECK_EQUAL(sample.maxBucketChunkCount,actual.maxBucketChunkCount);
    BOOST_CHECK_EQUAL(sample.minBucketChunkCount,actual.minBucketChunkCount);
    BOOST_CHECK_EQUAL(sample.maxBucketUsedChunkCount,actual.maxBucketUsedChunkCount);
    BOOST_CHECK_EQUAL(sample.minBucketUsedChunkCount,actual.minBucketUsedChunkCount);
}

BOOST_FIXTURE_TEST_CASE(CheckAllocateDeallocate,Env)
{
    PoolConfig::Parameters params(2048);
    UnsynchronizedPool pool(params);

    size_t allocatedChunkSize=PoolWithConfig::alignedChunkSize(params.chunkSize)+sizeof(void*);
    BOOST_CHECK_EQUAL(pool.allocatedChunkSize(),allocatedChunkSize);

    Stats stats;
    Stats statsSample;
    statsSample.allocatedChunkCount=0;
    statsSample.usedChunkCount=0;
    statsSample.maxBucketChunkCount=0;
    statsSample.minBucketChunkCount=0;
    statsSample.maxBucketUsedChunkCount=0;
    statsSample.minBucketUsedChunkCount=0;

    pool.getStats(stats);
    BOOST_TEST_CONTEXT("Empty pool")
    {
        checkStats(statsSample,stats);
    }

    BOOST_CHECK_EQUAL(pool.bucketsCount(),0u);
    BOOST_CHECK(pool.isEmpty());

    auto block=pool.allocateRawBlock();
    BOOST_CHECK(block!=nullptr);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),1);
    BOOST_CHECK(!pool.isEmpty());

    statsSample.allocatedChunkCount=params.chunkCount;
    statsSample.usedChunkCount=1;
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=1;
    statsSample.minBucketUsedChunkCount=1;

    pool.getStats(stats);
    BOOST_TEST_CONTEXT("Pool with one chunk")
    {
        checkStats(statsSample,stats);
    }

    pool.deallocateRawBlock(block);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),1);
    BOOST_CHECK(pool.isEmpty());

    statsSample.maxBucketUsedChunkCount=0;
    statsSample.minBucketUsedChunkCount=0;
    statsSample.usedChunkCount=0;
    pool.getStats(stats);
    BOOST_TEST_CONTEXT("Pool after chunk deallocation")
    {
        checkStats(statsSample,stats);
    }
}

BOOST_FIXTURE_TEST_CASE(CheckSequentialAllocateDeallocate,Env)
{
    constexpr const size_t chunkSize=32;
    PoolConfig::Parameters params(chunkSize,16);
    UnsynchronizedPool pool(params);
    pool.setDynamicBucketSizeEnabled(false);
    UnsynchronizedPool poolReset(params);

    Stats stats;
    Stats statsSample;

    constexpr const size_t count=1000;
    std::array<RawBlock*,count> blocks;
    std::array<std::array<char,chunkSize>,count> sampleBlocks;

    bool cmpOk=true;
    for (size_t i=0;i<count;i++)
    {
        auto block=pool.allocateRawBlock();
        blocks[i]=block;

        auto& sampleBlock=sampleBlocks[i];
        randBuf(sampleBlock);
        auto data=block->data();
        std::copy(sampleBlock.begin(),sampleBlock.end(),data);
        if (cmpOk)
        {
            for (size_t j=0;j<=i;j++)
            {
                auto sampleData=sampleBlocks[j].data();
                auto data=blocks[j]->data();
                cmpOk=memcmp(sampleData,data,chunkSize)==0;
                if (!cmpOk)
                {
                    BOOST_TEST_MESSAGE(fmt::format("Broken compare at {}/{}",j,i));
                    break;
                }
            }
        }

        std::ignore=poolReset.allocateRawBlock();
    }
    BOOST_CHECK(cmpOk);
    BOOST_CHECK(!pool.isEmpty());
    auto bucketsCount=count/params.chunkCount+(((count%params.chunkCount)!=0)?1:0);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    size_t allocatedChunkSize=PoolWithConfig::alignedChunkSize(params.chunkSize)+sizeof(void*);
    BOOST_CHECK_EQUAL(pool.allocatedChunkSize(),allocatedChunkSize);
    statsSample.allocatedChunkCount=bucketsCount*params.chunkCount;
    statsSample.usedChunkCount=count;
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=params.chunkCount-(statsSample.allocatedChunkCount-count);
    BOOST_TEST_CONTEXT("Pool after filling")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    cmpOk=true;
    for (size_t i=0;i<count;i++)
    {
        if (cmpOk)
        {
            auto sampleData=sampleBlocks[i].data();
            auto data=blocks[i]->data();
            cmpOk=memcmp(sampleData,data,chunkSize)==0;
        }
    }
    BOOST_CHECK(cmpOk);

    cmpOk=true;
    for (size_t i=0;i<count/2;i++)
    {
        if (cmpOk)
        {
            auto sampleData=sampleBlocks[i].data();
            auto data=blocks[i]->data();
            cmpOk=memcmp(sampleData,data,chunkSize)==0;
        }

        pool.deallocateRawBlock(blocks[i]);
    }
    BOOST_CHECK(cmpOk);
    BOOST_CHECK(!pool.isEmpty());
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    BOOST_CHECK_EQUAL(pool.allocatedChunkSize(),allocatedChunkSize);
    statsSample.allocatedChunkCount=bucketsCount*params.chunkCount;
    statsSample.usedChunkCount=count/2;
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=0;
    BOOST_TEST_CONTEXT("Pool after half deallocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    cmpOk=true;
    for (size_t i=count/2;i<count;i++)
    {
        if (cmpOk)
        {
            cmpOk=memcmp(sampleBlocks[i].data(),blocks[i]->data(),chunkSize)==0;
        }

        pool.deallocateRawBlock(blocks[i]);
    }
    BOOST_CHECK(cmpOk);
    BOOST_CHECK(pool.isEmpty());
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    BOOST_CHECK_EQUAL(pool.allocatedChunkSize(),allocatedChunkSize);
    statsSample.allocatedChunkCount=bucketsCount*params.chunkCount;
    statsSample.usedChunkCount=0;
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=0;
    statsSample.minBucketUsedChunkCount=0;
    BOOST_TEST_CONTEXT("Pool after deallocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    poolReset.clear();
    BOOST_TEST_CONTEXT("Pool after reset")
    {
        Stats resetStats;
        poolReset.getStats(stats);
        resetStats.clear();
        checkStats(resetStats,stats);
    }
}

BOOST_FIXTURE_TEST_CASE(CheckInterleavedAllocateDeallocate,Env)
{
    Stats stats;
    Stats statsSample;

    constexpr const size_t chunkSize=32;
    constexpr const size_t chunkCount=64;

    PoolConfig::Parameters params(chunkSize,chunkCount);
    UnsynchronizedPool pool(params);
    pool.setDynamicBucketSizeEnabled(false);

    constexpr const size_t count=1000;
    std::array<RawBlock*,count> blocks;
    std::array<std::array<char,chunkSize>,count> sampleBlocks;
    std::set<size_t> allocatedIndexes;

    for (size_t i=0;i<count;i++)
    {
        auto block=pool.allocateRawBlock();
        blocks[i]=block;

        auto& sampleBlock=sampleBlocks[i];
        randBuf(sampleBlock);
        auto data=block->data();
        std::copy(sampleBlock.begin(),sampleBlock.end(),data);
        allocatedIndexes.insert(i);

        if (i>16 && i%4==0)
        {
            pool.deallocateRawBlock(blocks[i-16]);
            allocatedIndexes.erase(i-16);
            auto it=allocatedIndexes.find(i-11);
            if (it!=allocatedIndexes.end())
            {
                pool.deallocateRawBlock(blocks[i-11]);
                allocatedIndexes.erase(i-11);
            }
        }
    }

    bool cmpOk=true;
    for (auto&& it:allocatedIndexes)
    {
        if (cmpOk)
        {
            cmpOk=memcmp(sampleBlocks[it].data(),blocks[it]->data(),chunkSize)==0;
        }
    }

    BOOST_CHECK(cmpOk);
    BOOST_CHECK(!pool.isEmpty());
    auto bucketsCount=(count/params.chunkCount+(((count%params.chunkCount)!=0)?1:0))/2;
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    size_t allocatedChunkSize=PoolWithConfig::alignedChunkSize(params.chunkSize)+sizeof(void*);
    BOOST_CHECK_EQUAL(pool.allocatedChunkSize(),allocatedChunkSize);
    statsSample.allocatedChunkCount=bucketsCount*chunkCount;
    statsSample.usedChunkCount=allocatedIndexes.size();
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=chunkCount-2;
    BOOST_TEST_CONTEXT("Pool after first allocate deallocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    size_t i=0;
    size_t deallocateCount=allocatedIndexes.size()/2;
    for (auto it=allocatedIndexes.begin();;)
    {
        pool.deallocateRawBlock(blocks[*it]);
        it=allocatedIndexes.erase(it);

        if (++i==deallocateCount)
        {
            break;
        }
    }

    cmpOk=true;
    for (auto&& it:allocatedIndexes)
    {
        if (cmpOk)
        {
            cmpOk=memcmp(sampleBlocks[it].data(),blocks[it]->data(),chunkSize)==0;
        }
    }
    BOOST_CHECK(cmpOk);
    BOOST_CHECK(!pool.isEmpty());
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    statsSample.usedChunkCount=allocatedIndexes.size();
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=0;
    BOOST_TEST_CONTEXT("Pool after half deallocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    for (size_t i=0;i<count;i++)
    {
        auto it=allocatedIndexes.find(i);
        if (it==allocatedIndexes.end())
        {
            auto block=pool.allocateRawBlock();
            blocks[i]=block;

            auto& sampleBlock=sampleBlocks[i];
            auto data=block->data();
            std::copy(sampleBlock.begin(),sampleBlock.end(),data);
            allocatedIndexes.insert(i);
        }
    }

    cmpOk=true;
    for (size_t i=0;i<count;i++)
    {
        if (cmpOk)
        {
            cmpOk=memcmp(sampleBlocks[i].data(),blocks[i]->data(),chunkSize)==0;
        }
    }
    BOOST_CHECK(cmpOk);
    BOOST_CHECK(!pool.isEmpty());
    bucketsCount=count/params.chunkCount+(((count%params.chunkCount)!=0)?1:0);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    statsSample.allocatedChunkCount=bucketsCount*params.chunkCount;
    statsSample.usedChunkCount=count;
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=params.chunkCount-(statsSample.allocatedChunkCount-count);
    BOOST_TEST_CONTEXT("Pool after full allocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    pool.clear();
    BOOST_TEST_CONTEXT("Pool after reset")
    {
        BOOST_CHECK(pool.isEmpty());
        BOOST_CHECK_EQUAL(pool.bucketsCount(),0u);

        Stats resetStats;
        pool.getStats(stats);
        resetStats.clear();
        checkStats(resetStats,stats);
    }
}

BOOST_FIXTURE_TEST_CASE(CheckGarbageCollector,Env)
{
    Stats stats;
    Stats statsSample;

    constexpr const size_t chunkSize=32;
    constexpr const size_t chunkCount=64;

    PoolConfig::Parameters params(chunkSize,chunkCount);
    UnsynchronizedPool pool(params);
    pool.setDynamicBucketSizeEnabled(false);
    pool.setDropBucketDelay(3);

    constexpr const size_t count=1000;
    std::array<RawBlock*,count> blocks;
    std::array<std::array<char,chunkSize>,count> sampleBlocks;
    std::set<size_t> allocatedIndexes;

    for (size_t i=0;i<count;i++)
    {
        auto block=pool.allocateRawBlock();
        blocks[i]=block;

        auto& sampleBlock=sampleBlocks[i];
        randBuf(sampleBlock);
        auto data=block->data();
        std::copy(sampleBlock.begin(),sampleBlock.end(),data);
        allocatedIndexes.insert(i);
    }

    bool cmpOk=true;
    for (auto&& it:allocatedIndexes)
    {
        if (cmpOk)
        {
            cmpOk=memcmp(sampleBlocks[it].data(),blocks[it]->data(),chunkSize)==0;
        }
    }

    BOOST_CHECK(cmpOk);
    BOOST_CHECK(!pool.isEmpty());
    auto bucketsCount=count/params.chunkCount+(((count%params.chunkCount)!=0)?1:0);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    size_t allocatedChunkSize=PoolWithConfig::alignedChunkSize(params.chunkSize)+sizeof(void*);
    BOOST_CHECK_EQUAL(pool.allocatedChunkSize(),allocatedChunkSize);
    statsSample.allocatedChunkCount=bucketsCount*chunkCount;
    statsSample.usedChunkCount=allocatedIndexes.size();
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    auto minBucketUsedChunkCount=params.chunkCount-(statsSample.allocatedChunkCount-count);
    statsSample.minBucketUsedChunkCount=minBucketUsedChunkCount;
    BOOST_TEST_CONTEXT("Pool after allocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    for (size_t i=0;i<chunkCount;i++)
    {
        pool.deallocateRawBlock(blocks[i]);
        allocatedIndexes.erase(i);
    }
    for (size_t i=3*chunkCount;i<5*chunkCount;i++)
    {
        pool.deallocateRawBlock(blocks[i]);
        allocatedIndexes.erase(i);
    }
    for (size_t i=count-chunkCount;i<count;i++)
    {
        pool.deallocateRawBlock(blocks[i]);
        allocatedIndexes.erase(i);
    }
    cmpOk=true;
    for (auto&& it:allocatedIndexes)
    {
        if (cmpOk)
        {
            cmpOk=memcmp(sampleBlocks[it].data(),blocks[it]->data(),chunkSize)==0;
        }
    }
    BOOST_CHECK(cmpOk);
    statsSample.allocatedChunkCount=bucketsCount*chunkCount;
    statsSample.usedChunkCount=allocatedIndexes.size();
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=0;
    BOOST_TEST_CONTEXT("Pool after partial deallocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    pool.setGarbageCollectorPeriod(1000);
    pool.setGarbageCollectorEnabled(true);

    BOOST_TEST_MESSAGE("Executing 3 seconds...");
    exec(3);

    cmpOk=true;
    for (auto&& it:allocatedIndexes)
    {
        if (cmpOk)
        {
            cmpOk=memcmp(sampleBlocks[it].data(),blocks[it]->data(),chunkSize)==0;
        }
    }
    BOOST_CHECK(cmpOk);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    BOOST_CHECK_EQUAL(pool.unusedBucketsCount(),4);
    statsSample.allocatedChunkCount=bucketsCount*chunkCount;
    statsSample.usedChunkCount=allocatedIndexes.size();
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=minBucketUsedChunkCount;
    BOOST_TEST_CONTEXT("Pool after partial deallocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    BOOST_TEST_MESSAGE("Executing 5 seconds...");
    exec(5);

    for (size_t i=0;i<count;i++)
    {
        auto it=allocatedIndexes.find(i);
        if (it==allocatedIndexes.end())
        {
            auto block=pool.allocateRawBlock();
            blocks[i]=block;

            auto& sampleBlock=sampleBlocks[i];
            auto data=block->data();
            std::copy(sampleBlock.begin(),sampleBlock.end(),data);
            allocatedIndexes.insert(i);
        }
    }

    cmpOk=true;
    for (size_t i=0;i<count;i++)
    {
        if (cmpOk)
        {
            cmpOk=memcmp(sampleBlocks[i].data(),blocks[i]->data(),chunkSize)==0;
        }
    }
    BOOST_CHECK(cmpOk);
    BOOST_CHECK(!pool.isEmpty());
    bucketsCount=count/params.chunkCount+(((count%params.chunkCount)!=0)?1:0);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    statsSample.allocatedChunkCount=bucketsCount*params.chunkCount;
    statsSample.usedChunkCount=count;
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=params.chunkCount-(statsSample.allocatedChunkCount-count);
    BOOST_TEST_CONTEXT("Pool after full allocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    BOOST_TEST_MESSAGE("Executing 5 seconds...");
    exec(5);

    for (size_t i=count-chunkCount;i<count;i++)
    {
        pool.deallocateRawBlock(blocks[i]);
        allocatedIndexes.erase(i);
    }
    bucketsCount=count/params.chunkCount+(((count%params.chunkCount)!=0)?1:0);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    statsSample.allocatedChunkCount=bucketsCount*params.chunkCount;
    statsSample.usedChunkCount=count-chunkCount;
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=params.chunkCount-(statsSample.allocatedChunkCount-count);

    BOOST_TEST_MESSAGE("Executing 2 seconds...");
    exec(2);
    cmpOk=true;
    for (auto&& it:allocatedIndexes)
    {
        if (cmpOk)
        {
            cmpOk=memcmp(sampleBlocks[it].data(),blocks[it]->data(),chunkSize)==0;
        }
    }
    BOOST_CHECK(cmpOk);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),bucketsCount);
    statsSample.allocatedChunkCount=bucketsCount*chunkCount;
    statsSample.usedChunkCount=allocatedIndexes.size();
    statsSample.maxBucketChunkCount=params.chunkCount;
    statsSample.minBucketChunkCount=params.chunkCount;
    statsSample.maxBucketUsedChunkCount=statsSample.maxBucketChunkCount;
    statsSample.minBucketUsedChunkCount=minBucketUsedChunkCount;
    BOOST_TEST_CONTEXT("Pool after final partial deallocate")
    {
        pool.getStats(stats);
        checkStats(statsSample,stats);
    }

    pool.clear();
    BOOST_TEST_CONTEXT("Pool after reset")
    {
        BOOST_CHECK(pool.isEmpty());
        BOOST_CHECK_EQUAL(pool.bucketsCount(),0u);

        Stats resetStats;
        pool.getStats(stats);
        resetStats.clear();
        checkStats(resetStats,stats);
    }
}

namespace {

struct ThreadContext
{
    constexpr static const size_t chunkSize=9;
#ifdef BUILD_VALGRIND
    constexpr static const size_t count=50000;
#else
    #if defined (BUILD_ANDROID) || defined(BUILD_DEBUG)
        constexpr static const size_t count=80000;
    #else
        constexpr static const size_t count=500000;
    #endif
#endif

    std::vector<RawBlock*> blocks;
    std::vector<std::array<char,chunkSize>> sampleBlocks;
    std::set<size_t> allocatedIndexes;

    ThreadContext()
    {
        blocks.resize(count*2);
        sampleBlocks.resize(count*2);
    }
};

}

template <typename PoolT>
static void checkParallelAllocateDeallocate(Env* env)
{
    const size_t chunkSize=ThreadContext::chunkSize;
    const size_t chunkCount=256;
    const size_t threadCount=4;
    const size_t count=ThreadContext::count;

    ElapsedTimer elapsed;
    std::array<ThreadContext,threadCount> contexts;
    env->createThreads(threadCount+1);

    PoolConfig::Parameters params(chunkSize,chunkCount);
    PoolT pool(env->thread(0).get(),params);
    pool.setDynamicBucketSizeEnabled(true);
    pool.setGarbageCollectorPeriod(1000);
    pool.setGarbageCollectorEnabled(true);

    for (size_t i=0;i<count*2;i++)
    {
        for (size_t j=0;j<threadCount;j++)
        {
            auto& ctx=contexts[j];
            auto& sampleBlock=ctx.sampleBlocks[i];
            env->randBuf(sampleBlock);
        }
    }

    std::atomic<size_t> doneCount(0);

#ifdef _MSC_VER
    auto handler=[&pool,&contexts,&doneCount,env,count,chunkCount,threadCount](size_t idx)
#else
    auto handler=[&pool,&contexts,&doneCount,env](size_t idx)
#endif
    {
        auto& ctx=contexts[idx];

        for (size_t k=0;k<1;k++)
        {
            size_t startI=k*count;
            if (idx%2==k)
            {
                for (size_t i=startI;i<startI+count;i++)
                {
                    auto block=pool.allocateRawBlock();
                    ctx.blocks[i]=block;

                    auto& sampleBlock=ctx.sampleBlocks[i];
                    auto data=block->data();
                    std::copy(sampleBlock.begin(),sampleBlock.end(),data);
                    ctx.allocatedIndexes.insert(i);

                    if (i>16 && i%4==0)
                    {
                        pool.deallocateRawBlock(ctx.blocks[i-16]);
                        ctx.allocatedIndexes.erase(i-16);
                        auto it=ctx.allocatedIndexes.find(i-11);
                        if (it!=ctx.allocatedIndexes.end())
                        {
                            pool.deallocateRawBlock(ctx.blocks[i-11]);
                            ctx.allocatedIndexes.erase(i-11);
                        }
                    }
                }
            }
            else
            {
                for (size_t i=startI;i<startI+count;i++)
                {
                    auto block=pool.allocateRawBlock();
                    ctx.blocks[i]=block;

                    auto& sampleBlock=ctx.sampleBlocks[i];
                    auto data=block->data();
                    std::copy(sampleBlock.begin(),sampleBlock.end(),data);
                    ctx.allocatedIndexes.insert(i);
                }

                for (size_t i=startI;i<startI+chunkCount;i++)
                {
                    pool.deallocateRawBlock(ctx.blocks[i]);
                    ctx.allocatedIndexes.erase(i);
                }
                for (size_t i=startI+3*chunkCount;i<startI+5*chunkCount;i++)
                {
                    pool.deallocateRawBlock(ctx.blocks[i]);
                    ctx.allocatedIndexes.erase(i);
                }
                for (size_t i=startI+count-chunkCount;i<startI+count;i++)
                {
                    pool.deallocateRawBlock(ctx.blocks[i]);
                    ctx.allocatedIndexes.erase(i);
                }
            }
        }

        HATN_TEST_MESSAGE_TS(fmt::format("Done handler for thread {}",idx));
        if (++doneCount==threadCount)
        {
            env->quit();
        }
    };

    BOOST_TEST_MESSAGE(fmt::format("Executing for count {}",count));

    env->thread(0)->start(false);
    for (int j=0;j<threadCount;j++)
    {
        env->thread(j+1)->execAsync(
                        [&handler,j]()
                        {
                            handler(j);
                        }
                    );
        env->thread(j+1)->start(false);
    }
    elapsed.reset();
    if (doneCount!=count)
    {
        env->exec(15);
    }
    auto elapsedStr=elapsed.toString();
    BOOST_TEST_MESSAGE(fmt::format("Elapsed {}",elapsedStr));

    BOOST_CHECK(!pool.isEmpty());

    BOOST_CHECK_GT(pool.bucketsCount(),0u);

    for (size_t k=0;k<threadCount;k++)
    {
        auto& ctx=contexts[k];
        BOOST_CHECK_GT(ctx.allocatedIndexes.size(),0u);
        bool cmpOk=true;
        for (auto&& it:ctx.allocatedIndexes)
        {
            cmpOk=memcmp(ctx.sampleBlocks[it].data(),ctx.blocks[it]->data(),chunkSize)==0;
            if (!cmpOk)
            {
                break;
            }
        }
        BOOST_CHECK(cmpOk);
    }

    Stats stats;
#if 0
    BOOST_TEST_CONTEXT("Pool after exec done")
    {
        Stats sampleStats;
        sampleStats.clear();
        pool.getStats(stats);
        checkStats(sampleStats,stats);
    }
#endif

    BOOST_TEST_MESSAGE("Clear pool");
    elapsed.reset();
    pool.clear();
    elapsedStr=elapsed.toString();
    BOOST_TEST_MESSAGE(fmt::format("Elapsed {}",elapsedStr));
    BOOST_TEST_CONTEXT("Pool after reset")
    {
        BOOST_CHECK(pool.isEmpty());
        BOOST_CHECK_EQUAL(pool.bucketsCount(),0u);

        Stats resetStats;
        pool.getStats(stats);
        resetStats.clear();
        checkStats(resetStats,stats);
    }

    env->thread(0)->stop();
    for (int j=0;j<threadCount;j++)
    {
        env->thread(j+1)->stop();
    }
}

BOOST_FIXTURE_TEST_CASE(CheckParallelAllocateDeallocate,Env)
{
    BOOST_TEST_CONTEXT("Use other thread")
    {
        checkParallelAllocateDeallocate<SynchronizedThreadPool>(this);
    }
}

BOOST_FIXTURE_TEST_CASE(CheckParallelAllocateDeallocateST,Env)
{
    BOOST_TEST_CONTEXT("Use the same thread")
    {
        checkParallelAllocateDeallocate<SynchronizedMutexPool>(this);
    }
}

template <typename PoolT>
static void checkParallelAllocateDeallocateGb(Env* env)
{
    constexpr const size_t chunkSize=ThreadContext::chunkSize;
    constexpr const size_t threadCount=4;
    constexpr const size_t count=ThreadContext::count;
    constexpr const size_t chunkCount=(count/chunkSize)/32;

    ElapsedTimer elapsed;
    std::array<ThreadContext,threadCount+1> contexts;
    env->createThreads(threadCount+1);
    env->thread(0)->start(false);

    PoolConfig::Parameters params(chunkSize,chunkCount);
    PoolT pool(env->thread(0).get(),params);
    pool.setDynamicBucketSizeEnabled(true);
    pool.setGarbageCollectorEnabled(false);
    pool.setDropBucketDelay(2);

    // fill sample blocks for thread contexts
    for (size_t i=0;i<count*2;i++)
    {
        for (size_t j=1;j<threadCount+1;j++)
        {
            auto& ctx=contexts[j];
            auto& sampleBlock=ctx.sampleBlocks[i];
            env->randBuf(sampleBlock);
        }
    }

    constexpr const size_t count1=ThreadContext::count;
    std::vector<RawBlock*> blocks(count1);
    std::vector<std::array<char,chunkSize>> sampleBlocks(count1);
    std::set<size_t> allocatedIndexes;

    // fill sample raw blocks and copy them to test raw blocks
    for (size_t i=0;i<count1;i++)
    {
        auto block=pool.allocateRawBlock();
        blocks[i]=block;

        auto& sampleBlock=sampleBlocks[i];
        env->randBuf(sampleBlock);
        auto data=block->data();
        std::copy(sampleBlock.begin(),sampleBlock.end(),data);
        allocatedIndexes.insert(i);

        // deallocate some blocks, e.g. each (i-16) and (i-11) when i is divisible by 4
        if (i>16 && i%4==0)
        {
            pool.deallocateRawBlock(blocks[i-16]);
            allocatedIndexes.erase(i-16);
            auto it=allocatedIndexes.find(i-11);
            if (it!=allocatedIndexes.end())
            {
                pool.deallocateRawBlock(blocks[i-11]);
                allocatedIndexes.erase(i-11);
            }
        }
    }

    // deallocate blocks up to 4*chunkCount before threads running
    if (4*chunkCount<=count1)
    {
        for (size_t i=0;i<4*chunkCount;i++)
        {
            auto it=allocatedIndexes.find(i);
            if (it!=allocatedIndexes.end())
            {
                pool.deallocateRawBlock(blocks[i]);
                allocatedIndexes.erase(i);
            }
        }
    }

    // deallocate blocks between 8*chunkCOunt and 256*chunkCount before threads running
    if (256*chunkCount<=count1)
    {
        for (size_t i=8*chunkCount;i<256*chunkCount;i++)
        {
            auto it=allocatedIndexes.find(i);
            if (it!=allocatedIndexes.end())
            {
                pool.deallocateRawBlock(blocks[i]);
                allocatedIndexes.erase(i);
            }
        }
        for (size_t i=count1-64*chunkCount;i<count1;i++)
        {
            auto it=allocatedIndexes.find(i);
            if (it!=allocatedIndexes.end())
            {
                pool.deallocateRawBlock(blocks[i]);
                allocatedIndexes.erase(i);
            }
        }
    }

    // add this point we have partially populated memory pool

    // init handler for periodic invokations of garbage collector
    std::atomic<bool> stopGb(false);
    auto gbTimer=makeShared<AsioHighResolutionTimer>(env->thread(0).get());
    gbTimer->setSingleShot(false);
    gbTimer->setPeriodUs(1000);
    gbTimer->start(
        [&pool,&stopGb](TimerTypes::Status status)
        {
            if (status==TimerTypes::Timeout)
            {
                for (size_t i=0;i<100;i++)
                {
                    if (stopGb.load(std::memory_order_acquire))
                    {
                        return false;
                    }
                    pool.runGarbageCollector();
                }
            }
            return true;
        }
    );

    // init test quick timer
    auto quitTimer=makeShared<AsioDeadlineTimer>(env->thread(0).get(),
        [env](TimerTypes::Status status)
        {
            if (status==TimerTypes::Timeout)
            {
                env->quit();
            }
            return true;
        }
    );
    quitTimer->setSingleShot(true);
    quitTimer->setPeriodUs(5000000);

    std::atomic<size_t> doneCount(0);

    // sample working handler that allocates/deallocates blocks in the pool copies data between blocks
#ifdef _MSC_VER
    auto handler=[&pool,&contexts,&doneCount,quitTimer,count,chunkCount,threadCount](size_t threadIdx)
#else
    auto handler=[&pool,&contexts,&doneCount,quitTimer](size_t threadIdx)
#endif
    {
        auto& ctx=contexts[threadIdx];

        for (size_t k=0;k<1;k++)
        {
            size_t startI=k*count;
            if (threadIdx%2==k)
            {
                for (size_t i=startI;i<startI+count;i++)
                {
                    auto block=pool.allocateRawBlock();
                    ctx.blocks[i]=block;                    

                    auto& sampleBlock=ctx.sampleBlocks[i];
                    auto data=block->data();
                    std::copy(sampleBlock.begin(),sampleBlock.end(),data);
                    ctx.allocatedIndexes.insert(i);

                    if (i>16 && i%4==0)
                    {
                        pool.deallocateRawBlock(ctx.blocks[i-16]);
                        ctx.allocatedIndexes.erase(i-16);
                        auto it=ctx.allocatedIndexes.find(i-11);
                        if (it!=ctx.allocatedIndexes.end())
                        {
                            pool.deallocateRawBlock(ctx.blocks[i-11]);
                            ctx.allocatedIndexes.erase(i-11);
                        }
                    }
                }
            }
            else
            {
                for (size_t i=startI;i<startI+count;i++)
                {
                    auto block=pool.allocateRawBlock();
                    ctx.blocks[i]=block;

                    auto& sampleBlock=ctx.sampleBlocks[i];
                    auto data=block->data();
                    std::copy(sampleBlock.begin(),sampleBlock.end(),data);
                    ctx.allocatedIndexes.insert(i);
                }

                for (size_t i=startI;i<startI+chunkCount;i++)
                {
                    auto it=ctx.allocatedIndexes.find(i);
                    if (it!=ctx.allocatedIndexes.end())
                    {
                        pool.deallocateRawBlock(ctx.blocks[i]);
                        ctx.allocatedIndexes.erase(i);
                    }
                }
                if (256*chunkCount<=count)
                {
                    for (size_t i=startI+8*chunkCount;i<startI+256*chunkCount;i++)
                    {
                        auto it=ctx.allocatedIndexes.find(i);
                        if (it!=ctx.allocatedIndexes.end())
                        {
                            pool.deallocateRawBlock(ctx.blocks[i]);
                            ctx.allocatedIndexes.erase(i);
                        }
                    }

                    for (size_t i=startI+count-256*chunkCount;i<startI+count;i++)
                    {
                        auto it=ctx.allocatedIndexes.find(i);
                        if (it!=ctx.allocatedIndexes.end())
                        {
                            pool.deallocateRawBlock(ctx.blocks[i]);
                            ctx.allocatedIndexes.erase(i);
                        }
                    }
                }
            }
        }

        if (512*chunkCount<=count)
        {
            for (size_t i=count-512*chunkCount;i<2*count;i++)
            {
                auto it=ctx.allocatedIndexes.find(i);
                if (it!=ctx.allocatedIndexes.end())
                {
                    pool.deallocateRawBlock(ctx.blocks[i]);
                    ctx.allocatedIndexes.erase(i);
                }
            }
        }

        HATN_TEST_MESSAGE_TS(fmt::format("Done handler for thread {}",threadIdx));
        if ((doneCount.fetch_add(1)+1)==threadCount)
        {
            quitTimer->start();
        }
    };

    // run handlers in threads
    BOOST_TEST_MESSAGE(fmt::format("Executing for count {}",count));    
    for (int j=1;j<threadCount+1;j++)
    {
        env->thread(j)->execAsync(
                        [&handler,j]()
                        {
                            handler(j);
                        }
                    );
    }
    for (int j=1;j<threadCount+1;j++)
    {
        env->thread(j)->start(false);
    }

    SpinLock gbLock;
    env->thread(0)->execAsync(
        [&pool,&stopGb, &gbLock]()
        {
            for (size_t i=0;i<100;i++)
            {
                SpinScopedLock l(gbLock);
                if (stopGb.load(std::memory_order_acquire))
                {
                    break;
                }
                pool.runGarbageCollector();
            }
        }
    );

    // run main thread
    elapsed.reset();
    if (doneCount!=count)
    {
        env->exec(60);
    }
    auto elapsedStr=elapsed.toString();
    BOOST_TEST_MESSAGE(fmt::format("Elapsed {}",elapsedStr));

    gbTimer->cancel();
    gbLock.lock();
    stopGb.store(true,std::memory_order_release);
    gbLock.unlock();

    BOOST_CHECK_EQUAL(doneCount.load(),threadCount);
    BOOST_CHECK(!pool.isEmpty());
    BOOST_CHECK_GT(pool.bucketsCount(),0u);

    // check contents of ordinary blocks that were allocated/deallocated before thread running
    bool cmpOk=true;
    for (auto&& it:allocatedIndexes)
    {
        cmpOk=memcmp(sampleBlocks[it].data(),blocks[it]->data(),chunkSize)==0;
        if (!cmpOk)
        {
            BOOST_TEST_MESSAGE(fmt::format("Data mismatch in sampleBlocks, index {} of {}",static_cast<size_t>(it),allocatedIndexes.size()));
            break;
        }
    }
    BOOST_CHECK_MESSAGE(cmpOk,"Data mismatch in ordinary blocks");

    // check contents of thread blocks
    for (size_t k=1;k<threadCount+1;k++)
    {
        auto& ctx=contexts[k];
        BOOST_CHECK_GT(ctx.allocatedIndexes.size(),0u);
        cmpOk=true;
        for (auto&& it:ctx.allocatedIndexes)
        {
            cmpOk=memcmp(ctx.sampleBlocks[it].data(),ctx.blocks[it]->data(),chunkSize)==0;
            if (!cmpOk)
            {
                BOOST_TEST_MESSAGE(fmt::format("Data mismatched in ctx.sampleBlocks, thread {}, index {} of {}",k,static_cast<size_t>(it),allocatedIndexes.size()));
                break;
            }
        }
        BOOST_CHECK_MESSAGE(cmpOk, fmt::format("Data mismatch in blocks of thread {}",k));
    }

    Stats stats;

    BOOST_TEST_MESSAGE("Clear pool");
    elapsed.reset();
    pool.clear();
    elapsedStr=elapsed.toString();
    BOOST_TEST_MESSAGE(fmt::format("Elapsed {}",elapsedStr));
    BOOST_TEST_CONTEXT("Pool after reset")
    {
        BOOST_CHECK(pool.isEmpty());
        BOOST_CHECK_EQUAL(pool.bucketsCount(),0u);

        Stats resetStats;
        pool.getStats(stats);
        resetStats.clear();
        checkStats(resetStats,stats);
    }

    env->thread(0)->stop();
    for (int j=0;j<threadCount;j++)
    {
        env->thread(j+1)->stop();
    }
}

BOOST_FIXTURE_TEST_CASE(CheckParallelAllocateDeallocateGb,Env)
{
    BOOST_TEST_CONTEXT("Use other thread")
    {
        checkParallelAllocateDeallocateGb<SynchronizedThreadPool>(this);
    }
}

BOOST_FIXTURE_TEST_CASE(CheckParallelAllocateDeallocateGbCt,Env)
{
    BOOST_TEST_CONTEXT("Use current thread")
    {
        checkParallelAllocateDeallocateGb<SynchronizedMutexPool>(this);
    }
}

namespace {

struct TestStruct
{
    size_t idx;
    char data[100];

    TestStruct(size_t idx):idx(idx)
    {}

    ~TestStruct()
    {
        idxCount+=idx;
    }
    TestStruct(const TestStruct&)=default;
    TestStruct(TestStruct&&) =default;
    TestStruct& operator=(const TestStruct&)=default;
    TestStruct& operator=(TestStruct&&) =default;

    static size_t idxCount;
};
size_t TestStruct::idxCount=0;

}

BOOST_FIXTURE_TEST_CASE(CheckObjectPool,Env)
{
    createThreads(1);
    PoolConfig::Parameters params(sizeof(TestStruct),32);
    ObjectPool<TestStruct,UnsynchronizedPool> pool(thread(0).get(),params);

    constexpr const size_t count=1000;
    std::vector<TestStruct*> objects(count);

    for (size_t i=0;i<count;i++)
    {
        objects[i]=pool.create(i+1);
    }
    for (size_t i=0;i<count;i++)
    {
        pool.destroy(objects[i]);
    }

    size_t sum=((1+count)*count)/2;
    BOOST_CHECK_EQUAL(TestStruct::idxCount,sum);
    BOOST_CHECK_EQUAL(pool.bucketsCount(),6);
    BOOST_CHECK_EQUAL(pool.allocatedChunkSize(),PoolWithConfig::alignedChunkSize(sizeof(TestStruct))+sizeof(void*));

    Stats stats;
    Stats statsSample;
    statsSample.allocatedChunkCount=2016;
    statsSample.usedChunkCount=0;
    statsSample.maxBucketChunkCount=1024;
    statsSample.minBucketChunkCount=32;
    statsSample.maxBucketUsedChunkCount=0;
    statsSample.minBucketUsedChunkCount=0;
    pool.getStats(stats);
    checkStats(statsSample,stats);
}

#define TEST_POOL_PERFORMANCE
#ifdef TEST_POOL_PERFORMANCE

template <typename PoolT>
static void performanceTest(Env* env)
{
    size_t cyclesCount=4000000;
    size_t chunkSize=200;
    size_t chunkCount=cyclesCount/2;
    size_t allocateDeallocateIters=4;
    size_t allocateDeallocateBunchSize=4000;

    ElapsedTimer elapsed;
    env->createThreads(2);

    PoolConfig::Parameters params(chunkSize,chunkCount);
    UnsynchronizedPool pool(env->thread(0).get(),params);
    pool.setDynamicBucketSizeEnabled(true);
    pool.setGarbageCollectorEnabled(true);
    pool.setMaxBucketSize(cyclesCount);

    UnsynchronizedPool pool1(env->thread(0).get(),params);
    pool1.setDynamicBucketSizeEnabled(true);
    pool1.setGarbageCollectorEnabled(true);
    pool1.setMaxBucketSize(cyclesCount);
    BOOST_TEST_MESSAGE(fmt::format("Test performance for {} blocks of size {} with chunk count {}",cyclesCount,chunkSize,chunkCount));

    auto poolMem=std::make_shared<PoolT>(env->thread(0).get(),params);
    poolMem->setDynamicBucketSizeEnabled(true);
    poolMem->setGarbageCollectorEnabled(true);
    poolMem->setMaxBucketSize(cyclesCount);
    auto resource1=std::make_shared<PoolMemoryResource<PoolT>>(std::move(poolMem));

    auto cacheGen=std::make_shared<PoolCacheGen<PoolT>>(chunkCount*chunkSize);
    auto creator=[env,cyclesCount](const memorypool::PoolConfig::Parameters& params)
    {
        auto pool=std::make_shared<PoolT>(env->thread(0).get(),params);
        pool->setDynamicBucketSizeEnabled(true);
        pool->setGarbageCollectorEnabled(true);
        pool->setMaxBucketSize(cyclesCount);
        return pool;
    };
    cacheGen->setPoolCreator(creator);
    auto resource2=std::make_shared<PoolMemoryResource<PoolT>>(
            typename PoolMemoryResource<PoolT>::Options(std::move(cacheGen),false)
        );

    auto pool3=std::make_shared<PoolT>(env->thread(0).get(),params);
    pool3->setDynamicBucketSizeEnabled(true);
    pool3->setGarbageCollectorEnabled(true);
    pool3->setMaxBucketSize(cyclesCount);
    auto resource3=std::make_shared<SinglePoolMemoryResource<PoolT>>(std::move(pool3));

    auto handler=[env,cyclesCount,&pool,&pool1,&elapsed,chunkSize,allocateDeallocateIters,allocateDeallocateBunchSize,
                  resource1,resource2,resource3]()
    {
        std::vector<RawBlock*> blocks(cyclesCount);
        std::string elapsedStr;

        elapsed.reset();
        for (size_t k=0;k<allocateDeallocateIters;k++)
        {
            for (size_t i=0;i<cyclesCount;)
            {
                for (size_t j=0;j<allocateDeallocateBunchSize;j++)
                {
                    blocks[i+j]=pool1.allocateRawBlock();
                }
                for (size_t j=0;j<allocateDeallocateBunchSize;j++)
                {
                    pool1.deallocateRawBlock(blocks[i++]);
                }
            }
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done pool allocation/deallocation, elapsed {}",elapsedStr));
        pool1.clear();

        elapsed.reset();
        for (size_t k=0;k<allocateDeallocateIters;k++)
        {
            for (size_t i=0;i<cyclesCount;)
            {
                for (size_t j=0;j<allocateDeallocateBunchSize;j++)
                {
                    blocks[i+j]=reinterpret_cast<RawBlock*>(new char[chunkSize]);
                }
                for (size_t j=0;j<allocateDeallocateBunchSize;j++)
                {
                    delete [] (reinterpret_cast<char*>(blocks[i++]));
                }
            }
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done new/delete, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=pool.allocateRawBlock();
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done pool allocation, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            pool.deallocateRawBlock(blocks[i]);
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done pool deallocation, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=pool.allocateRawBlock();
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done reuse pool allocation, elapsed {}",elapsedStr));

        elapsed.reset();
        pool.clear();
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done pool clear, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=pool.allocateRawBlock();
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done reuse pool allocation, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            pool.deallocateRawBlock(blocks[i]);
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done reuse pool deallocation, elapsed {}",elapsedStr));

        elapsed.reset();
        pool.runGarbageCollector();
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done pool garbage collector, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=reinterpret_cast<RawBlock*>(new char[chunkSize]);
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done new, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            delete [] (reinterpret_cast<char*>(blocks[i]));
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done delete, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=reinterpret_cast<RawBlock*>(resource1->allocate(chunkSize,alignof(RawBlock)));
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with embedded allocate, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            resource1->deallocate(blocks[i],chunkSize,alignof(RawBlock));
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with embedded deallocate, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=reinterpret_cast<RawBlock*>(resource1->allocate(chunkSize,alignof(RawBlock)));
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with embedded reused allocate, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=reinterpret_cast<RawBlock*>(resource1->allocate(chunkSize,alignof(RawBlock)));
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with embedded pool deallocate, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=reinterpret_cast<RawBlock*>(resource3->allocate(chunkSize,alignof(RawBlock)));
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with single pool allocate, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            resource3->deallocate(blocks[i],chunkSize,alignof(RawBlock));
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with single pool deallocate, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=reinterpret_cast<RawBlock*>(resource3->allocate(chunkSize,alignof(RawBlock)));
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with single pool reused allocate, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            resource3->deallocate(blocks[i],chunkSize,alignof(RawBlock));
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with single pool deallocate, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            blocks[i]=reinterpret_cast<RawBlock*>(resource2->allocate(chunkSize,alignof(RawBlock)));
            if (i%4==0)
            {
                blocks[i]=reinterpret_cast<RawBlock*>(resource2->allocate(32,alignof(RawBlock)));
            }
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with cache gen allocate, elapsed {}",elapsedStr));

        elapsed.reset();
        for (size_t i=0;i<cyclesCount;i++)
        {
            resource2->deallocate(blocks[i],chunkSize,alignof(RawBlock));
            if (i%4==0)
            {
                resource2->deallocate(blocks[i],32,alignof(RawBlock));
            }
        }
        elapsedStr=elapsed.toString();
        BOOST_TEST_MESSAGE(fmt::format("Done polymorphic memory resource with cache gen deallocate, elapsed {}",elapsedStr));

        env->quit();
    };

    env->thread(1)->execAsync(
        handler
    );
    env->thread(0)->start(false);
    env->thread(1)->start(false);
    env->exec(60);
    env->thread(0)->stop();
    env->thread(1)->stop();

    resource1.reset();
    resource2.reset();
}

BOOST_FIXTURE_TEST_CASE(CheckPoolPerformanceUnsynchronized,Env, * boost::unit_test::disabled())
{
    performanceTest<UnsynchronizedPool>(this);
}

BOOST_FIXTURE_TEST_CASE(CheckPoolPerformanceMutex,Env, * boost::unit_test::disabled())
{
    performanceTest<SynchronizedMutexPool>(this);
}

BOOST_FIXTURE_TEST_CASE(CheckPoolPerformanceThread,Env, * boost::unit_test::disabled())
{
    performanceTest<SynchronizedThreadPool>(this);
}

#endif
BOOST_AUTO_TEST_SUITE_END()
