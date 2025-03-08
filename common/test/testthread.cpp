#include <iostream>
#include <chrono>

#include <boost/test/unit_test.hpp>

#include <hatn/common/thread.h>
#include <hatn/common/threadwithqueue.h>
#include <hatn/common/threadpoolwithqueues.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/format.h>
#include <hatn/common/sharedlocker.h>

#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/memorypool/newdeletepool.h>

#include <hatn/common/mutexqueue.h>
#include <hatn/common/mpscqueue.h>
#include <hatn/common/asiotimer.h>

#include <hatn/test/multithreadfixture.h>

HATN_USING
HATN_COMMON_USING
HATN_TEST_USING

BOOST_AUTO_TEST_SUITE(TestThread)

namespace {

using MemoryResource=memorypool::NewDeletePoolResource;
using MemoryPool=memorypool::NewDeletePool;

struct MemResourceConfig
{
    MemResourceConfig(const std::shared_ptr<memorypool::PoolCacheGen<MemoryPool>>& poolCacheGen):opts(poolCacheGen)
    {
    }

    MemoryResource::Options opts;
};

std::unique_ptr<pmr::memory_resource> makeResource(const MemResourceConfig& config)
{
    return std::make_unique<MemoryResource>(config.opts);
}
}

struct TestStruct : public TaskContext
{
    uint32_t m_id;

    //! Ctor
    TestStruct(
            uint32_t id
        ) : m_id(id)
    {}
};

BOOST_AUTO_TEST_CASE(MainThread)
{
    auto mainThread=std::make_shared<Thread>("main",false);
    Thread::setMainThread(mainThread);

    BOOST_CHECK(Thread::mainThread().get()!=nullptr);
    Thread::releaseMainThread();
    BOOST_CHECK(Thread::mainThread().get()==nullptr);
    mainThread.reset();
}

BOOST_FIXTURE_TEST_CASE(ExecAsync,MultiThreadFixture)
{
    auto thread=std::make_shared<Thread>("test");
    thread->start();

    int counter=0;
    auto task=[&counter]()
    {
        ++counter;
    };

    thread->execAsync(task);
    thread->execAsync(task);
    thread->execAsync(task);

    exec(1);

    BOOST_CHECK_EQUAL(counter,3);
}

BOOST_FIXTURE_TEST_CASE(ExecSync,MultiThreadFixture)
{
    auto thread=std::make_shared<Thread>("test");
    thread->start();

    int counter=0;
    auto task=[&counter]()
    {
        return ++counter;
    };
    auto task1=[&counter]()
    {
        ++counter;
    };

    int ret=thread->execSync<int>(task);
    BOOST_CHECK_EQUAL(ret,1);
    ret=thread->execSync<int>(task);
    BOOST_CHECK_EQUAL(ret,2);
    ret=thread->execSync<int>(task);
    BOOST_CHECK_EQUAL(ret,3);

    if (thread->execSync(task1))
    {
        BOOST_FAIL("Timeout in execSync");
    }

    BOOST_CHECK_EQUAL(counter,4);
}

BOOST_FIXTURE_TEST_CASE(ExecFuture,MultiThreadFixture)
{
    auto thread=std::make_shared<Thread>("test");
    thread->start();

    int counter=0;
    auto task=[&counter]()
    {
        return ++counter;
    };

    auto f1=thread->execFuture<int>(task);
    auto f2=thread->execFuture<int>(task);
    auto f3=thread->execFuture<int>(task);

    BOOST_CHECK_EQUAL(f3.get(),3);
    BOOST_CHECK_EQUAL(f1.get(),1);
    BOOST_CHECK_EQUAL(f2.get(),2);

    BOOST_CHECK_EQUAL(counter,3);
}

void testSimpleQueue(TaskQueue* queue, MultiThreadFixture* testFxt)
{
    int counter=0;
    auto handler=[&counter]()
    {
        if (strcmp(Thread::currentThreadID(),"test")==0)
        {
            ++counter;
        }
    };

    auto thread=std::make_shared<TaskThread>("test",queue);

    BOOST_CHECK_EQUAL(counter,0);
    thread->start();

    Task task(handler);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);
    thread->postTask(task);

    testFxt->exec(1);

    BOOST_CHECK_EQUAL(counter,12);

    thread->stop();
    thread.reset();
}

BOOST_FIXTURE_TEST_CASE(SimpleMutexQueue,MultiThreadFixture)
{
    auto queue=new MutexQueue<Task>();
    testSimpleQueue(queue,this);
}

BOOST_FIXTURE_TEST_CASE(SimpleMpscQueue,MultiThreadFixture)
{
    auto queue=new MPSCQueue<Task>();
    testSimpleQueue(queue,this);
}

void testContextQueue(TaskWithContextQueue* queue, MultiThreadFixture* testFxt, const MemResourceConfig& config)
{
    int counter=0;

    auto resource=makeResource(config);
    pmr::polymorphic_allocator<TestStruct> allocator(resource.get());

    {
        auto ptr0=allocateShared<TestStruct>(allocator,20);

        auto handler=[&counter,&ptr0](const SharedPtr<TaskContext>& ptr)
        {
            if (strcmp(Thread::currentThreadID(),"test")==0)
            {
                auto ptr1=ptr.dynamicCast<TestStruct>();
                BOOST_REQUIRE(!ptr1.isNull());
                ++ptr1->m_id;
                ++counter;
                if (!ptr0.isNull())
                {
                    ptr0.reset();
                }
            }
        };

        auto ptr=allocateShared<TestStruct>(allocator,10);

        auto thread=std::make_shared<TaskWithContextThread>("test",queue);

        BOOST_CHECK_EQUAL(counter,0);
        thread->start();

        for (size_t i=0;i<8;i++)
        {
            auto* task=thread->prepare();
            task->setContext(ptr);
            if (i==0 || i>4)
            {
                task->setGuard(ptr0);
            }
            task->setHandler(handler);
            thread->post(task);
        }

        testFxt->exec(1);

        BOOST_CHECK_EQUAL(counter,5);
        BOOST_CHECK_EQUAL(ptr->m_id,static_cast<uint32_t>(15));

        thread->stop();
        thread.reset();
    }
}

BOOST_FIXTURE_TEST_CASE(ContextMutexQueue,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    auto queue=new MutexQueue<TaskWithContext>(resource.get());
    testContextQueue(queue,this,config);
}

BOOST_FIXTURE_TEST_CASE(ContextMpscQueue,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    auto queue=new MPSCQueue<TaskWithContext>(resource.get());
    testContextQueue(queue,this,config);
}

void testClearQueue(TaskQueue* queue,
                   int count)
{
    auto workerThread=std::make_shared<TaskThread>("worker",queue);

    BOOST_CHECK(workerThread->isQueueEmpty());
    BOOST_CHECK_EQUAL(workerThread->queueDepth(),0);

    for (int i=0;i<count;i++)
    {
        auto task=workerThread->prepare();
        task->handler=[]{};
        workerThread->post(task);
    }

    BOOST_CHECK(!workerThread->isQueueEmpty());
    BOOST_CHECK_EQUAL(workerThread->queueDepth(),count);

    workerThread->clearQueue();

    BOOST_CHECK(workerThread->isQueueEmpty());
    BOOST_CHECK_EQUAL(workerThread->queueDepth(),0);

    workerThread.reset();
}

BOOST_FIXTURE_TEST_CASE(ClearQueueMutex,MultiThreadFixture)
{
#ifdef BUILD_VALGRIND
    int count=10;
#else
    int count=1000;
#endif

    auto queue=new MutexQueue<Task>();
    testClearQueue(queue,count);
}

BOOST_FIXTURE_TEST_CASE(ClearQueueMPSC,MultiThreadFixture)
{
#ifdef BUILD_VALGRIND
    int count=10;
#else
    int count=1000;
#endif
    auto queue=new MPSCQueue<Task>();
    testClearQueue(queue,count);
}

void testQueueLoad(TaskQueue* queue,
                   int count,
                   int delay,
                   MultiThreadFixture* testFxt)
{
    auto workerThread=std::make_shared<TaskThread>("worker",queue);
    workerThread->start();

    std::chrono::time_point<std::chrono::system_clock> start;
    start = std::chrono::system_clock::now();    

    std::atomic<int> counter1(0);
    std::atomic<int> counter2(0);
    std::atomic<int> counter3(0);
    std::atomic<int> counter4(0);

    auto checkForDone=[&counter1,&counter2,&counter3,&counter4,&count,&testFxt]()
    {
        if (counter1==count && counter2==count && counter3==count && counter4==count)
        {
            testFxt->quit();
        }
    };

    auto handler1=[&counter1,&count,&start,&checkForDone]()
    {
        ++counter1;
        if (counter1==count)
        {
            auto end = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                                     (end-start).count();
            std::cout << "Counter1 "<<count<<" took " << elapsed << " ms" << std::endl;
            checkForDone();
        }
    };
    auto handler2=[&counter2,&count,&start,&checkForDone]()
    {
        ++counter2;
        if (counter2==count)
        {
            auto end = std::chrono::system_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                                     (end-start).count();
            std::cout << "Counter2 "<<count<<" took " << elapsed << " ms" << std::endl;
            checkForDone();
        }
    };
    auto handler3=[&counter3,&count,&start,&checkForDone]()
    {
        ++counter3;
        if (counter3==count)
        {
            auto end = std::chrono::system_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                                     (end-start).count();
            std::cout << "Counter3 "<<count<<" took " << elapsed << " ms" << std::endl;
            checkForDone();
        }
    };
    auto handler4=[&counter4,&count,&start,&checkForDone]()
    {
        ++counter4;
        if (counter4==count)
        {
            auto end = std::chrono::system_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                                     (end-start).count();
            std::cout << "Counter4 "<<count<<" took " << elapsed << " ms" << std::endl;
            checkForDone();
        }
    };

    auto thread1=std::make_shared<TaskThread>("1");
    thread1->start();
    auto taskHandler1=[&count,&workerThread,&handler1,&start]()
    {
        for (int i=0;i<count;i++)
        {
            auto task=workerThread->prepare();
            task->handler=handler1;
            workerThread->post(task);
        }
        auto end = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                                 (end-start).count();
        std::cout << "Done "<<count<<" task handler1 after " << elapsed << " ms" << std::endl;
    };
    thread1->execAsync(taskHandler1);

    auto thread2=std::make_shared<TaskThread>("2");
    thread2->start();
    auto taskHandler2=[&count,&workerThread,&handler2,&start]()
    {
        for (int i=0;i<count;i++)
        {
            auto task=workerThread->prepare();
            task->handler=handler2;
            workerThread->post(task);
        }
        auto end = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                                 (end-start).count();
        std::cout << "Done "<<count<<" task handler2 after " << elapsed << " ms" << std::endl;
    };
    thread2->execAsync(taskHandler2);

    auto thread3=std::make_shared<TaskThread>("3");
    thread3->start();
    auto taskHandler3=[&count,&workerThread,&handler3,&start]()
    {
        for (int i=0;i<count;i++)
        {
            auto task=workerThread->prepare();
            task->handler=handler3;
            workerThread->post(task);
        }
        auto end = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                                 (end-start).count();
        std::cout << "Done "<<count<<" task handler3 after " << elapsed << " ms" << std::endl;
    };
    thread3->execAsync(taskHandler3);

    auto thread4=std::make_shared<TaskThread>("4");
    thread4->start();
    auto taskHandler4=[&count,&workerThread,&handler4,&start]()
    {
        for (int i=0;i<count;i++)
        {
            auto task=workerThread->prepare();
            task->handler=handler4;
            workerThread->post(task);
        }
        auto end = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                                 (end-start).count();
        std::cout << "Done "<<count<<" task handler4 after " << elapsed << " ms" << std::endl;
    };
    thread4->execAsync(taskHandler4);

    testFxt->exec(delay);

    BOOST_CHECK_EQUAL(counter1,count);
    BOOST_CHECK_EQUAL(counter2,count);
    BOOST_CHECK_EQUAL(counter3,count);
    BOOST_CHECK_EQUAL(counter4,count);

    thread1->stop();
    thread2->stop();
    thread3->stop();
    thread4->stop();
    workerThread->stop();
    workerThread->clearQueue();
    workerThread.reset();
}

BOOST_FIXTURE_TEST_CASE(MutexQueueLoad,MultiThreadFixture)
{
#ifdef BUILD_VALGRIND
    int count=20;
#else
    int count=200000;
#endif
    auto queue=new MutexQueue<Task>();
    testQueueLoad(queue,count,180,this);
}
BOOST_FIXTURE_TEST_CASE(MpscQueueLoad,MultiThreadFixture)
{
    /* note that it is a memory consuming test
     * no check for available memory is performed, so it can crash or hang on 32-bit or low memory platforms
     * especially for debug builds
     */
#ifdef BUILD_VALGRIND
    int count=20;
#else
    #if defined (BUILD_ANDROID)
        int count=200000;
    #else
        #if defined(BUILD_DEBUG) || (defined(_WIN32) && !defined(_WIN64))
            int count=100000;
        #else
            int count=10000000;
        #endif
    #endif
#endif

    auto queue=new MPSCQueue<Task>();
    testQueueLoad(queue,count,180,this);
}

void testQueueBoundaries(TaskQueue* queue,
                   MultiThreadFixture* testFxt,
                   int countIn=100)
{
    auto workerThread=std::make_shared<TaskThread>("worker",queue);
    workerThread->start();

    int counter1=0;
    int counter2=0;
    int counter3=0;
    int counter4=0;

#ifdef BUILD_VALGRIND
    std::ignore=countIn;
    int count=2;
    size_t cycle=2;
    size_t repeats=4;
#else
    int count=countIn;
    size_t cycle=10;
    size_t repeats=100;
#endif
    auto result=count*cycle*repeats;

    auto checkForDone=[&counter1,&counter2,&counter3,&counter4,&testFxt,&result]()
    {
        if (counter1==result && counter2==result && counter3==result && counter4==result)
        {
            testFxt->quit();
        }
    };

    auto handler1=[&counter1,&checkForDone]()
    {
        ++counter1;        
        checkForDone();
    };
    auto handler2=[&counter2,&checkForDone]()
    {
        ++counter2;
        checkForDone();
    };
    auto handler3=[&counter3,&checkForDone]()
    {
        ++counter3;
        checkForDone();
    };
    auto handler4=[&counter4,&checkForDone]()
    {
        ++counter4;
        checkForDone();
    };

    auto thread1=std::make_shared<TaskThread>("1");
    thread1->start();
    auto taskHandler1=[&count,&workerThread,&handler1,cycle,repeats]()
    {
        auto k=repeats;
        while (k--!=0)
        {
            auto j=cycle;
            while (j--!=0)
            {
                do
                {
                    bool pushBecauseDepth=workerThread->queueDepth()==j;
                    if (workerThread->isQueueEmpty() || pushBecauseDepth)
                    {
                        for (int i=0;i<count;i++)
                        {
                            auto task=workerThread->prepare();
                            task->handler=handler1;
                            workerThread->post(task);
                        }
                        break;
                    }
#ifdef BUILD_VALGRIND
                std::this_thread::sleep_for(std::chrono::milliseconds(12));
#endif
                } while (true);
            }
        }
    };
    thread1->execAsync(taskHandler1);

    auto thread2=std::make_shared<TaskThread>("2");
    thread2->start();
    auto taskHandler2=[&count,&workerThread,&handler2,cycle,repeats]()
    {
        auto k=repeats;
        while (k--!=0)
        {
            auto j=cycle;
            while (j--!=0)
            {
                do
                {
                    bool pushBecauseDepth=workerThread->queueDepth()==j;
                    if (workerThread->isQueueEmpty() || pushBecauseDepth)
                    {
                        for (int i=0;i<count;i++)
                        {
                            auto task=workerThread->prepare();
                            task->handler=handler2;
                            workerThread->post(task);
                        }
                        break;
                    }
#ifdef BUILD_VALGRIND
                std::this_thread::sleep_for(std::chrono::milliseconds(7));
#endif
                } while (true);
            }
        }
    };
    thread2->execAsync(taskHandler2);

    auto thread3=std::make_shared<TaskThread>("3");
    thread3->start();
    auto taskHandler3=[&count,&workerThread,&handler3,cycle,repeats]()
    {
        auto k=repeats;
        while (k--!=0)
        {
            auto j=cycle;
            while (j--!=0)
            {
                do
                {
                    bool pushBecauseDepth=workerThread->queueDepth()==j;
                    if (workerThread->isQueueEmpty() || pushBecauseDepth)
                    {
                        for (int i=0;i<count;i++)
                        {
                            auto task=workerThread->prepare();
                            task->handler=handler3;
                            workerThread->post(task);
                        }
                        break;
                    }
#ifdef BUILD_VALGRIND
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
                } while (true);
            }
        }
    };
    thread3->execAsync(taskHandler3);

    auto thread4=std::make_shared<TaskThread>("4");
    thread4->start();
    auto taskHandler4=[&count,&workerThread,&handler4,cycle,repeats]()
    {
        auto k=repeats;
        while (k--!=0)
        {
            auto j=cycle;
            while (j--!=0)
            {
                do
                {
                    bool pushBecauseDepth=workerThread->queueDepth()==j;
                    if (workerThread->isQueueEmpty() || pushBecauseDepth)
                    {
                        for (int i=0;i<count;i++)
                        {
                            auto task=workerThread->prepare();
                            task->handler=handler4;
                            workerThread->post(task);
                        }
#ifdef BUILD_VALGRIND
                std::this_thread::sleep_for(std::chrono::milliseconds(9));
#endif
                        break;
                    }
                } while (true);
            }
        }
    };
    thread4->execAsync(taskHandler4);

    testFxt->exec(180);

    BOOST_CHECK_EQUAL(counter1,result);
    BOOST_CHECK_EQUAL(counter2,result);
    BOOST_CHECK_EQUAL(counter3,result);
    BOOST_CHECK_EQUAL(counter4,result);

    thread1->stop();
    thread2->stop();
    thread3->stop();
    thread4->stop();

    workerThread->stop();
    workerThread.reset();
}

BOOST_FIXTURE_TEST_CASE(MutexQueueBoundaries,MultiThreadFixture)
{
    auto queue=new MutexQueue<Task>();
    testQueueBoundaries(queue,this,70);
}
BOOST_FIXTURE_TEST_CASE(MpscQueueBoundaries,MultiThreadFixture)
{
    auto queue=new MPSCQueue<Task>();
    testQueueBoundaries(queue,this,70);
}

BOOST_FIXTURE_TEST_CASE(ThreadPool,MultiThreadFixture)
{
    auto queue=new MPSCQueue<Task>();
    auto pool=new ThreadPoolWithQueues<Task>(4,"threadpool",queue);

    int counter1=0;
    int counter2=0;
    int counter3=0;
    int counter4=0;
    std::atomic<int> counter(0);
	volatile int jjj = 0;

    auto handler=[&counter1,&counter2,&counter3,&counter4,&counter,&jjj]()
    {
        counter.fetch_add(1,std::memory_order_relaxed);
        if (strcmp(Thread::currentThreadID(),"threadpool1")==0)
        {
            ++counter1;
        }
        else if (strcmp(Thread::currentThreadID(),"threadpool2")==0)
        {
            ++counter2;
        }
        else if (strcmp(Thread::currentThreadID(),"threadpool3")==0)
        {
            ++counter3;
        }
        else if (strcmp(Thread::currentThreadID(),"threadpool4")==0)
        {
            ++counter4;
        }        
#ifdef BUILD_VALGRIND
    int delayCount=20;
#else
    int delayCount=10000;
#endif
        for (int i=0;i<delayCount;i++)
        {
            jjj++;
        }
    };

    pool->start();

#ifdef BUILD_VALGRIND
    int repeats=5;
#else
    int repeats=100;
#endif
    for (int i=0;i<repeats;i++)
    {
        pool->postTask(Task(handler));

        auto task1=pool->prepare();
        task1->handler=handler;
        pool->post(task1);
    }

    exec(5);

    pool->stop();

    std::cout<<"Calculated data in pool: counter1="<<counter1<<", counter2="<<counter2<<", counter3="<<counter3<<", counter4="<<counter4<<std::endl;

    int sum=counter1+counter2+counter3+counter4;
    int result=repeats*2;
    BOOST_CHECK_EQUAL(sum,result);
    BOOST_CHECK_EQUAL(counter.load(std::memory_order_relaxed),result);

    delete pool;
}

#ifndef HATN_THREAD_TIMER_TEST_DISABLE
BOOST_FIXTURE_TEST_CASE(ThreadTimers,MultiThreadFixture)
#else
BOOST_FIXTURE_TEST_CASE(ThreadTimers,MultiThreadFixture, *boost::unit_test::disabled())
#endif
{
    int counter1=0;
    auto task1=[&counter1]()
    {
        ++counter1;
        return true;
    };
    int counter2=0;
    auto task2=[&counter2]()
    {
        ++counter2;
        return true;
    };
    int counter3=0;
    auto task3=[&counter3]()
    {
        ++counter3;
        return true;
    };
    int counter4=0;
    auto task4=[&counter4]()
    {
        ++counter4;
        return true;
    };
    int counter5=0;
    auto task5=[&counter5]()
    {
        ++counter5;
        return false;
    };
    int counter6=0;
    auto task6=[&counter6]()
    {
        ++counter6;
        return false;
    };

    createThreads(1);

    auto worker1=thread(0);

    worker1->installTimer(10,std::move(task1),false,true);
    worker1->installTimer(1000,std::move(task2),false,false);
    worker1->installTimer(10,std::move(task3),true,true);
    worker1->installTimer(1000,std::move(task4),true,false);
    worker1->installTimer(10,std::move(task5),false,true);
    worker1->installTimer(1000,std::move(task6),false,false);

    worker1->start();

    exec(5);

    worker1->stop();

    BOOST_TEST_MESSAGE("Checking if worker is stopped");
    BOOST_TEST_REQUIRE(worker1->isStopped());
    BOOST_TEST_MESSAGE("Worker is stopped, destroying worker...");
    destroyThreads();
    BOOST_TEST_MESSAGE(fmt::format("Thread destroyed, use count={}",worker1.use_count()));
    worker1.reset();
    BOOST_TEST_MESSAGE(fmt::format("Worker destroyed, use count={}",worker1.use_count()));

    BOOST_TEST_MESSAGE(fmt::format("Counter1 {}",counter1));
    BOOST_TEST_MESSAGE(fmt::format("Counter2 {}",counter2));
    BOOST_TEST_MESSAGE(fmt::format("Counter3 {}",counter3));
    BOOST_TEST_MESSAGE(fmt::format("Counter4 {}",counter4));
    BOOST_TEST_MESSAGE(fmt::format("Counter5 {}",counter5));
    BOOST_TEST_MESSAGE(fmt::format("Counter6 {}",counter6));

#ifdef BUILD_VALGRIND
    BOOST_CHECK_GT(counter1,5);
    BOOST_CHECK_GT(counter2,5);
#else
    BOOST_CHECK_GT(counter1,100);
    BOOST_CHECK_GT(counter2,100);
#endif

    BOOST_CHECK_EQUAL(counter3,1);
    BOOST_CHECK_EQUAL(counter4,1);
    BOOST_CHECK_EQUAL(counter5,1);
    BOOST_CHECK_EQUAL(counter6,1);

    BOOST_TEST_MESSAGE("End of test");
}

#ifndef HATN_THREAD_TIMER_TEST_DISABLE
BOOST_FIXTURE_TEST_CASE(TestAsioTimer,MultiThreadFixture)
#else
BOOST_FIXTURE_TEST_CASE(TestAsioTimer,MultiThreadFixture, *boost::unit_test::disabled())
#endif
{
    int counter1=0;
    auto task1=[&counter1](TimerTypes::Status status)
    {
        if (status==TimerTypes::Status::Timeout)
        {
            ++counter1;
        }
    };
    int counter2=0;
    auto task2=[&counter2](TimerTypes::Status status)
    {
        if (status==TimerTypes::Status::Timeout)
        {
            ++counter2;
        }
    };
    int counter3=0;
    auto task3=[&counter3](TimerTypes::Status status)
    {
        if (status==TimerTypes::Status::Timeout)
        {
            ++counter3;
        }
    };
    int counter4=0;
    auto task4=[&counter4](TimerTypes::Status status)
    {
        if (status==TimerTypes::Status::Timeout)
        {
            ++counter4;
        }
    };

    createThreads(4);

    auto worker1=thread(0);
    auto timer1=makeShared<AsioHighResolutionTimer>(worker1.get(),task1);
    timer1->setPeriodUs(1);
    timer1->setSingleShot(false);
    timer1->start();

    auto worker2=thread(1);
    auto timer2=makeShared<AsioDeadlineTimer>(worker2.get(),task2);
    timer2->setPeriodUs(1000);
    timer2->setSingleShot(false);
    timer2->start();

    auto worker3=thread(2);
    auto timer3=makeShared<AsioHighResolutionTimer>(worker3.get(),task3);
    timer3->setPeriodUs(1);
    timer3->start();

    auto worker4=thread(3);
    auto timer4=makeShared<AsioDeadlineTimer>(worker4.get(),task4);
    timer4->setPeriodUs(1000);
    timer4->start();

    worker1->start();
    worker2->start();
    worker3->start();
    worker4->start();

    exec(5);

    timer1->cancel();
    timer2->cancel();
    timer3->cancel();
    timer4->cancel();

    worker1->stop();
    worker2->stop();
    worker3->stop();
    worker4->stop();

    timer1->reset();
    timer2->reset();
    timer3->reset();
    timer4->reset();

    BOOST_TEST_MESSAGE(fmt::format("Counter1 {}",counter1));
    BOOST_TEST_MESSAGE(fmt::format("Counter2 {}",counter2));
    BOOST_TEST_MESSAGE(fmt::format("Counter3 {}",counter3));
    BOOST_TEST_MESSAGE(fmt::format("Counter4 {}",counter4));

#ifdef BUILD_VALGRIND
    BOOST_CHECK_GT(counter1,5);
    BOOST_CHECK_GT(counter2,5);
#else
    BOOST_CHECK_GT(counter1,100);
    BOOST_CHECK_GT(counter2,100);
#endif

    BOOST_CHECK_EQUAL(counter3,1);
    BOOST_CHECK_EQUAL(counter4,1);
}

BOOST_FIXTURE_TEST_CASE(TestSharedLocker,MultiThreadFixture)
{
    size_t count=10000;
    size_t pauseCount=100;

    size_t threadCount=4;
    createThreads(static_cast<int>(threadCount));

    SharedLocker locker;
    std::atomic<int> sharedVal(0);

    std::atomic<size_t> doneCount(0);

    auto sharedHandler=[&sharedVal,count,threadCount,&doneCount,&locker,pauseCount,this]()
    {
        for (size_t i=0;i<count;i++)
        {
            SharedLocker::SharedScope l(locker);

            auto val1=sharedVal.load();
            size_t sum=0;
            for (size_t j=0;j<pauseCount;j++)
            {
                ++sum;
            }
            if (sum!=pauseCount)
            {
                HATN_CHECK_EQUAL_TS(sum,pauseCount);
                quit();
                return;
            }

            auto val2=sharedVal.load();
            if (val1!=val2)
            {
                HATN_CHECK_EQUAL_TS(val1,val2);
                quit();
                return;
            }
        }
        if (++doneCount==threadCount)
        {
            quit();
        }
    };

    auto exclusiveHandler=[&sharedVal,count,threadCount,&doneCount,&locker,pauseCount,this]()
    {
        for (size_t i=0;i<count;i++)
        {
            SharedLocker::ExclusiveScope l(locker);

            auto val1=sharedVal.fetch_add(1)+1;
            size_t sum=0;
            for (size_t j=0;j<pauseCount;j++)
            {
                ++sum;
            }
            if (sum!=pauseCount)
            {
                HATN_CHECK_EQUAL_TS(sum,pauseCount);
                quit();
                return;
            }

            auto val2=sharedVal.load();
            if (val1!=val2)
            {
                HATN_CHECK_EQUAL_TS(val1,val2);
                quit();
                return;
            }
        }
        if (++doneCount==threadCount)
        {
            quit();
        }
    };

    for (int i=2;i<static_cast<int>(threadCount);i++)
    {
        thread(i)->execAsync(sharedHandler);
        thread(i)->start();
    }
    for (int i=0;i<2;i++)
    {
        thread(i)->execAsync(exclusiveHandler);
        thread(i)->start();
    }

    if (doneCount!=threadCount)
    {
        exec(15);
    }

    for (int i=0;i<static_cast<int>(threadCount);i++)
    {
        thread(i)->stop();
    }

    BOOST_CHECK_EQUAL(sharedVal.load(),count*2);
}

BOOST_FIXTURE_TEST_CASE(TestAsyncWithCallback,MultiThreadFixture)
{
    size_t threadCount=4;
    std::vector<TaskWithContextThread*> threads;
    threads.resize(threadCount);
    createThreads(static_cast<int>(threadCount));

    for (int i=0;i<static_cast<int>(threadCount);i++)
    {
        thread(i)->start();
        auto threadQ=dynamic_cast<TaskWithContextThread*>(thread(i).get());
        BOOST_REQUIRE(threadQ!=nullptr);
        threads[i]=threadQ;
    }

    auto originThread=threads[0];
    auto targetThread=threads[1];

    auto async1=[&targetThread,&originThread]()
    {
        HATN_TEST_MESSAGE_TS(fmt::format("Origin thread {}",Thread::currentThreadID()));
        BOOST_REQUIRE(originThread->id()==Thread::currentThreadID());

        auto cb=[&originThread](SharedPtr<TaskContext>, int val)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("Callback thread {}, val {}",Thread::currentThreadID(),val));
            BOOST_REQUIRE(originThread->id()==Thread::currentThreadID());
            BOOST_CHECK_EQUAL(val,100);
        };

        auto ctx=makeShared<TaskContext>();
        auto handler=[&targetThread](SharedPtr<TaskContext> ctx, auto cbArg)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("Target thread {}",Thread::currentThreadID()));
            BOOST_REQUIRE(targetThread->id()==Thread::currentThreadID());
            cbArg(std::move(ctx),100);
        };

        postAsyncTask(targetThread,ctx,handler,cb);
    };

    originThread->execAsync(async1);

    exec(1);
}

BOOST_AUTO_TEST_SUITE_END()
