#include <boost/test/unit_test.hpp>

#include <hatn/test/multithreadfixture.h>

#include <hatn/common/memorypool/newdeletepool.h>
#include <hatn/common/memorypool/multibucketpool.ipp>
#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/memorypool/poolcachegen.h>

#include <hatn/common/elapsedtimer.h>

HATN_USING
HATN_COMMON_USING
using namespace HATN_COMMON_NAMESPACE::memorypool;
HATN_TEST_USING
using namespace HATN_COMMON_NAMESPACE::pmr;

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

BOOST_AUTO_TEST_SUITE(TestPoolMemoryResource)

BOOST_FIXTURE_TEST_CASE(CheckPoolCacheGenSizes,Env)
{
    PoolCacheGen<UnsynchronizedPool> cacheGen;

    auto sizes=cacheGen.sizes().sizes();
    std::vector<KeyRange> checkSizes={
        {1,32},{33,64},{65,128},{129,224},{225,352},{353,544},
        {545,832},{833,1280},{1281,1600},{1601,2432},{2433,3680},
        {3681,4160},{4161,6272},{6273,9440},{9441,14176},{14177,16512},
        {16513,24800},{24801,33024},{33025,49568},{49569,65536}
    };
    BOOST_CHECK_EQUAL(checkSizes.size(),sizes.size());
    size_t i=0;
    for (auto&& it:sizes)
    {
        if (i<checkSizes.size())
        {
            BOOST_CHECK_EQUAL(it.from,checkSizes[i].from);
            BOOST_CHECK_EQUAL(it.to,checkSizes[i].to);
        }
        ++i;
    }
}

BOOST_FIXTURE_TEST_CASE(CheckPoolCacheGen,Env)
{
    createThreads(1);
    auto poolThread=thread(0).get();
    size_t chunkSize=80;
    size_t chunkCount=100;
    auto creator=[poolThread](const memorypool::PoolConfig::Parameters& params)
    {
        return std::make_shared<UnsynchronizedPool>(poolThread,params);
    };
    PoolCacheGen<UnsynchronizedPool> cacheGen(chunkSize*chunkCount);
    cacheGen.setPoolCreator(std::move(creator));
    BOOST_CHECK_EQUAL(cacheGen.bucketUsefulSize(),chunkSize*chunkCount);

    auto genPool1=cacheGen.pool(chunkSize,true);
    BOOST_REQUIRE(genPool1);
    BOOST_CHECK_EQUAL(genPool1->chunkSize(),chunkSize);
    BOOST_CHECK_EQUAL(genPool1->initialChunkCountPerBucket(),chunkCount);

    auto pool1=dynamic_cast<UnsynchronizedPool*>(genPool1.get());
    BOOST_REQUIRE(pool1);
    BOOST_CHECK(poolThread==pool1->thread());

    auto pool2=cacheGen.pool(chunkSize,true);
    BOOST_REQUIRE(pool2);
    BOOST_CHECK(pool2.get()==genPool1.get());

    auto pool3=cacheGen.pool(chunkSize);
    BOOST_REQUIRE(pool3);
    BOOST_CHECK(pool3.get()==genPool1.get());

    auto pool4=cacheGen.pool(76);
    BOOST_REQUIRE(pool4);
    BOOST_CHECK(pool4.get()!=genPool1.get());
    BOOST_CHECK_EQUAL(pool4->chunkSize(),128);
    BOOST_CHECK_EQUAL(pool4->initialChunkCountPerBucket(),cacheGen.bucketUsefulSize()/128);

    auto pool5=cacheGen.pool(84);
    BOOST_REQUIRE(pool5);
    BOOST_CHECK(pool4.get()==pool5.get());

    auto pool6=cacheGen.pool(200);
    BOOST_REQUIRE(pool6);
    BOOST_CHECK(pool6.get()!=pool4.get());
    BOOST_CHECK(pool6.get()!=genPool1.get());
    BOOST_CHECK_EQUAL(pool6->chunkSize(),224);
    BOOST_CHECK_EQUAL(pool6->initialChunkCountPerBucket(),cacheGen.bucketUsefulSize()/224);

    auto pool7=cacheGen.pool(224,true);
    BOOST_REQUIRE(pool7);
    BOOST_CHECK(pool7.get()==pool6.get());

    BOOST_CHECK_EQUAL(cacheGen.pools().size(),2);
    BOOST_CHECK_EQUAL(cacheGen.exactPools().size(),1);
}

namespace
{
struct TestStruct1
{
    size_t idx=0;
    char data[99];

    TestStruct1(size_t idx):idx(idx)
    {}
    ~TestStruct1()
    {
        IdxCount.fetch_add(idx,std::memory_order_relaxed);
    }
    TestStruct1(const TestStruct1&)=default;
    TestStruct1(TestStruct1&&) =default;
    TestStruct1& operator=(const TestStruct1&)=default;
    TestStruct1& operator=(TestStruct1&&) =default;

    static std::atomic<size_t> IdxCount;
};
std::atomic<size_t> TestStruct1::IdxCount(0);

}

BOOST_FIXTURE_TEST_CASE(CheckMemoryResource,Env)
{
    for (auto i=0;i<3;i++)
    {
        TestStruct1::IdxCount=0;

        std::shared_ptr<PoolMemoryResource<UnsynchronizedPool>> resource;
        std::string ctx;

        if (i==0)
        {
            auto pool=std::make_shared<UnsynchronizedPool>(sizeof(TestStruct1));
            resource=std::make_shared<PoolMemoryResource<UnsynchronizedPool>>(std::move(pool));
            ctx="Preset embedded pool";
        }
        else if (i==1)
        {
            auto cacheGen=std::make_shared<PoolCacheGen<UnsynchronizedPool>>();
            resource=std::make_shared<PoolMemoryResource<UnsynchronizedPool>>(std::move(cacheGen));
            ctx="Auto create embedded pool and use cache gen";
        }
        else
        {
            auto cacheGen=std::make_shared<PoolCacheGen<UnsynchronizedPool>>();
            resource=std::make_shared<PoolMemoryResource<UnsynchronizedPool>>(
                    PoolMemoryResource<UnsynchronizedPool>::Options(std::move(cacheGen),false)
                );
            ctx="Cache gen without embedded pool";
        }

        BOOST_TEST_CHECKPOINT(ctx.c_str());
        BOOST_TEST_CONTEXT(ctx.c_str())
        {
            polymorphic_allocator<TestStruct1> alloc(resource.get());

            auto obj1Buf=alloc.allocate(1);
            BOOST_REQUIRE(obj1Buf);
            auto obj1=new(obj1Buf) TestStruct1(1);
            BOOST_CHECK_EQUAL(obj1->idx,1);

            auto obj2Buf=alloc.allocate(1);
            BOOST_REQUIRE(obj2Buf);
            BOOST_CHECK(obj1Buf!=obj2Buf);
            auto obj2=new(obj2Buf) TestStruct1(2);
            BOOST_CHECK_EQUAL(obj2->idx,2);

            destroyDeallocate(obj1,alloc);
            BOOST_CHECK_EQUAL(TestStruct1::IdxCount,1);
            destroyDeallocate(obj2,alloc);
            BOOST_CHECK_EQUAL(TestStruct1::IdxCount,3);

            if (i==1 || i==2)
            {
                auto objArrBuf1=alloc.allocate(3);
                BOOST_REQUIRE(objArrBuf1);

                auto objArrBuf1c=reinterpret_cast<char*>(objArrBuf1);
                auto objArr3=new(objArrBuf1c) TestStruct1(3);
                auto objArr4=new(objArrBuf1c+sizeof(TestStruct1)) TestStruct1(4);
                auto objArr5=new(objArrBuf1c+2*sizeof(TestStruct1)) TestStruct1(5);

                auto objArrBuf2=alloc.allocate(2);
                if (!objArrBuf2)
                {
                    HATN_REQUIRE_TS(objArrBuf2);
                }

                auto objArrBuf2c=reinterpret_cast<char*>(objArrBuf2);
                auto objArr6=new(objArrBuf2c) TestStruct1(6);
                auto objArr7=new(objArrBuf2c+sizeof(TestStruct1)) TestStruct1(7);

                destroyAt(objArr3,alloc);
                BOOST_CHECK_EQUAL(TestStruct1::IdxCount,6);
                destroyAt(objArr4,alloc);
                BOOST_CHECK_EQUAL(TestStruct1::IdxCount,10);
                destroyAt(objArr5,alloc);
                BOOST_CHECK_EQUAL(TestStruct1::IdxCount,15);
                alloc.deallocate(objArrBuf1,3);

                destroyAt(objArr6,alloc);
                BOOST_CHECK_EQUAL(TestStruct1::IdxCount,21);
                destroyAt(objArr7,alloc);
                BOOST_CHECK_EQUAL(TestStruct1::IdxCount,28);
                alloc.deallocate(objArrBuf2,2);
            }
        }
    }
}

namespace {
struct TmpSum
{
    size_t& count;

    TmpSum(size_t& count) : count(count)
    {}
    ~TmpSum()
    {
        ++count;
    }
    TmpSum(const TmpSum&)=default;
    TmpSum(TmpSum&&) =default;
    TmpSum& operator=(const TmpSum&)=delete;
    TmpSum& operator=(TmpSum&&) =delete;
};
}

BOOST_FIXTURE_TEST_CASE(CheckMemoryResourceMt,Env)
{
#ifdef BUILD_VALGRIND
    size_t runCount=30;
    size_t delayCount=100000;
#else
    #if defined (ANDROID) || defined (BUILD_DEBUG)
        size_t runCount=200;
        size_t delayCount=1000000;
    #else
        size_t runCount=2000;
        size_t delayCount=5000000;
    #endif
#endif

    int threadCount=4;

    createThreads(threadCount);
    auto creator=[](const memorypool::PoolConfig::Parameters& params)
    {
        auto pool=new NewDeletePool(params);
        return std::shared_ptr<NewDeletePool>(pool);
    };

    for (auto i=0;i<3;i++)
    {
        TestStruct1::IdxCount=0;

        std::shared_ptr<PoolMemoryResource<NewDeletePool>> resource;
        std::string ctx;

        if (i==0)
        {
            auto pool=creator(memorypool::PoolConfig::Parameters(sizeof(TestStruct1)));
            resource=std::make_shared<PoolMemoryResource<NewDeletePool>>(std::move(pool));
            ctx="Preset embedded pool";
        }
        else if (i==1)
        {
            auto cacheGen=std::make_shared<PoolCacheGen<NewDeletePool>>();
            cacheGen->setPoolCreator(creator);
            resource=std::make_shared<PoolMemoryResource<NewDeletePool>>(std::move(cacheGen));
            ctx="Auto create embedded pool and use cache gen";
        }
        else
        {
            auto cacheGen=std::make_shared<PoolCacheGen<NewDeletePool>>();
            cacheGen->setPoolCreator(creator);
            resource=std::make_shared<PoolMemoryResource<NewDeletePool>>(
                    PoolMemoryResource<NewDeletePool>::Options(std::move(cacheGen),false)
                );
            ctx="Cache gen without embedded pool";
        }
        BOOST_TEST_MESSAGE(ctx.c_str());
        {
            std::atomic<size_t> doneCount(0);

            polymorphic_allocator<TestStruct1> alloc(resource.get());
            auto handler=[&alloc,runCount,&doneCount,delayCount,threadCount,i,this]()
            {
                size_t tmpCount=0;
                for (size_t k=0;k<delayCount;k++)
                {
                    TmpSum tmpSum(tmpCount);
                }
                if (tmpCount!=delayCount)
                {
                    HATN_CHECK_EQUAL_TS(tmpCount,delayCount);
                }

                bool ok=true;
                for (size_t j=0;j<runCount;j++)
                {
                    auto obj1Buf=alloc.allocate(1);
                    if (!obj1Buf)
                    {
                        HATN_REQUIRE_TS(obj1Buf);
                    }
                    auto obj1=new(obj1Buf) TestStruct1(1);
                    if (obj1->idx!=1)
                    {
                        HATN_CHECK_EQUAL_TS(obj1->idx,1);
                        ok=false;
                    }

                    auto obj2Buf=alloc.allocate(1);
                    if (!obj2Buf)
                    {
                        HATN_REQUIRE_TS(obj2Buf);
                    }
                    if (obj1Buf==obj2Buf)
                    {
                        HATN_CHECK_TS(obj1Buf!=obj2Buf);
                        ok=false;
                    }
                    auto obj2=new(obj2Buf) TestStruct1(2);
                    if (obj2->idx!=2)
                    {
                        HATN_CHECK_EQUAL_TS(obj2->idx,2);
                        ok=false;
                    }
                    destroyDeallocate(obj1,alloc);
                    destroyDeallocate(obj2,alloc);

                    if (i==1 || i==2)
                    {
                        auto objArrBuf1=alloc.allocate(3);
                        if (!objArrBuf1)
                        {
                            HATN_REQUIRE_TS(objArrBuf1);
                        }

                        auto objArrBuf1c=reinterpret_cast<char*>(objArrBuf1);
                        auto objArr3=new(objArrBuf1c) TestStruct1(3);
                        auto objArr4=new(objArrBuf1c+sizeof(TestStruct1)) TestStruct1(4);
                        auto objArr5=new(objArrBuf1c+2*sizeof(TestStruct1)) TestStruct1(5);

                        auto objArrBuf2=alloc.allocate(2);
                        if (!objArrBuf2)
                        {
                            HATN_REQUIRE_TS(objArrBuf2);
                        }

                        auto objArrBuf2c=reinterpret_cast<char*>(objArrBuf2);
                        auto objArr6=new(objArrBuf2c) TestStruct1(6);
                        auto objArr7=new(objArrBuf2c+sizeof(TestStruct1)) TestStruct1(7);

                        destroyAt(objArr3,alloc);
                        destroyAt(objArr4,alloc);
                        destroyAt(objArr5,alloc);
                        alloc.deallocate(objArrBuf1,3);

                        destroyAt(objArr6,alloc);
                        destroyAt(objArr7,alloc);
                        alloc.deallocate(objArrBuf2,2);

                        if (!ok)
                        {
                            break;
                        }
                    }
                }

                HATN_TEST_MESSAGE_TS(fmt::format("Handler done {}/{}",doneCount.load(),threadCount));
                if (++doneCount==threadCount)
                {
                    HATN_TEST_MESSAGE_TS("Call quit");
                    this->quit();
                }
            };

            BOOST_TEST_MESSAGE("Running exec()...");
            for (int j=0;j<threadCount;j++)
            {
                thread(j)->execAsync(handler);
                thread(j)->start(false);
            }
            if (doneCount!=threadCount)
            {
                exec(15);
            }
            BOOST_TEST_MESSAGE("Done exec()...");

            BOOST_CHECK_EQUAL(doneCount,threadCount);
            if (i==1 || i==2)
            {
                BOOST_CHECK_EQUAL(TestStruct1::IdxCount,28*runCount*threadCount);
            }
            else
            {
                BOOST_CHECK_EQUAL(TestStruct1::IdxCount,3*runCount*threadCount);
            }
            for (int j=0;j<threadCount;j++)
            {
                thread(j)->stop();
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
