/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/thread.сpp
  *
  *     hatn thread
  *
  */

#include <mutex>
#include <map>
#include <future>
#include <condition_variable>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/high_resolution_timer.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

#include <hatn/common/types.h>
#include <hatn/common/utils.h>
#include <hatn/common/queue.h>
#include <hatn/common/logger.h>
#include <hatn/common/makeshared.h>

#include <hatn/common/thread.h>

#include <hatn/common/threadcategoriespoolimpl.h>
#include <hatn/common/loggermoduleimp.h>

DECLARE_LOG_MODULE(thread)

HATN_COMMON_NAMESPACE_BEGIN

/********************** Thread **************************/

thread_local static char ThisThreadId[17] = {'m','a','i','n','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
thread_local static Thread* ThisThread=nullptr;

namespace {
struct MainThreadWrapper
{
    std::shared_ptr<Thread> mainThread;
};

static MainThreadWrapper& MainThread()
{
    static MainThreadWrapper mainThreadWrapper;
    return mainThreadWrapper;
}

struct Timer final
{
    uint64_t periodUs;
    std::function<bool()> handler;
    bool runOnce;
    std::function<void()> uninstall;

    std::unique_ptr<boost::asio::deadline_timer> normalTimer;
    std::unique_ptr<boost::asio::high_resolution_timer> highResTimer;

    std::atomic<bool> stopped;

    std::shared_ptr<int> refCount;

    void stop()
    {
        if (highResTimer!=nullptr)
        {
            highResTimer->cancel();
        }
        else
        {
            normalTimer->cancel();
        }
    }

    void timeout(const boost::system::error_code& ec)
    {
        if (ec != boost::asio::error::operation_aborted)
        {
            if (!handler() || runOnce)
            {
                if (uninstall)
                {
                    uninstall();
                }
            }
            else
            {
                auto count=refCount;
                if (highResTimer!=nullptr)
                {
                    highResTimer->expires_from_now(std::chrono::microseconds(periodUs));
                    highResTimer->async_wait([this,count](const boost::system::error_code& ec){this->timeout(ec);});
                }
                else
                {
                    normalTimer->expires_from_now(boost::posix_time::microseconds(periodUs));
                    normalTimer->async_wait([this,count](const boost::system::error_code& ec){this->timeout(ec);});
                }
            }
        }
        else
        {
            std::cerr<<"Timer aborted, ref count="<<refCount.use_count()<<std::endl;
            stopped.store(true,std::memory_order_relaxed);
        }
    }

    Timer(
            uint64_t timeoutPeriodUs,
            const std::function<bool ()>& handler,
            bool runOnce,
            bool highResolution,
            boost::asio::io_context& asioContext,
            const std::function<void ()>& uninstall
    ) : periodUs(timeoutPeriodUs),
        handler(handler),
        runOnce(runOnce),
        uninstall(uninstall),
        stopped(false)
    {
        std::cerr<<"Timer created, ref count="<<refCount.use_count()<<std::endl;

        refCount=std::make_shared<int>(0);
        auto count=refCount;
        if (highResolution)
        {
            highResTimer=std::make_unique<boost::asio::high_resolution_timer>(asioContext);
            highResTimer->expires_from_now(std::chrono::microseconds(periodUs));
            highResTimer->async_wait([this,count](const boost::system::error_code& ec){this->timeout(ec);});
        }
        else
        {
            normalTimer=std::make_unique<boost::asio::deadline_timer>(asioContext);
            normalTimer->expires_from_now(boost::posix_time::microseconds(periodUs));
            normalTimer->async_wait([this,count](const boost::system::error_code& ec){this->timeout(ec);});
        }
    }

    ~Timer() {
        refCount.reset();
        std::cerr<<"Timer destroy begin, ref count="<<refCount.use_count()<<std::endl;
        stop();
        // if (normalTimer) {
        //     normalTimer.reset();
        // } else if (highResTimer) {
        //     highResTimer.reset();
        // }
        std::cerr<<"Timer destroy end, ref count="<<refCount.use_count()<<std::endl;
    }

    Timer(const Timer&)=delete;
    Timer(const Timer&&)=delete;
    Timer& operator=(const Timer&)=delete;
    Timer& operator=(Timer&&)=delete;
};

}

class Thread_p
{
    public:

        std::shared_ptr<boost::asio::io_context> asioContext;
        bool newThread=false;

        std::atomic<bool> running;
        std::atomic<bool> stopped;
        std::atomic<bool> firstRun;

        std::condition_variable startedCondition;
        std::mutex mutex;
        std::thread thread;
        std::unique_ptr<boost::asio::io_context::work> dummyWork;
        std::thread::id nativeID;
        FixedByteArrayThrow16 id;

        std::atomic<int> handlersPending;

        std::map<uint32_t,std::shared_ptr<Timer>> timers;
        uint32_t timerIncId=0;

        void init(bool newT)
        {
            stopped.store(false);
            newThread=newT;
            asioContext=std::make_shared<boost::asio::io_context>(1);
            nativeID=std::this_thread::get_id();
            handlersPending.store(0,std::memory_order_release);
            firstRun.store(true);
            running.store(false);

            // required for the io service to continue working for ever
            dummyWork=std::make_unique<boost::asio::io_context::work>(boost::asio::io_context::work(*asioContext));
        }
};

//---------------------------------------------------------------
Thread::Thread(
        const FixedByteArrayThrow16& id,
        bool newThread
    ) : d(std::make_unique<Thread_p>())
{
    d->init(newThread);
    d->id=id;
    d->timerIncId=0;
}

//---------------------------------------------------------------
Thread::~Thread()
{
    std::cerr << "Thread::~Thread() " << id().c_str() << std::endl;

    stop();
}

Thread::Thread(Thread&&) noexcept=default;
Thread& Thread::operator=(Thread&&) noexcept=default;

//---------------------------------------------------------------
std::shared_ptr<boost::asio::io_context> Thread::asioContext() const noexcept
{
    return d->asioContext;
}

//---------------------------------------------------------------
boost::asio::io_context& Thread::asioContextRef() noexcept
{
    return *d->asioContext;
}

//---------------------------------------------------------------
void Thread::start(bool waitForStarted)
{
    if (d->running.load())
    {
        return;
    }
    d->stopped.store(false);

    if (d->newThread)
    {
        d->thread = std::thread(
            [this]()
            {
                run();
            }
        );
        if (waitForStarted)
        {
            std::unique_lock<decltype(d->mutex)> l(d->mutex);
            if (!d->running.load() && !d->stopped.load())
            {
                d->startedCondition.wait(l);
            }
        }
    }
    else
    {
        run();
    }
}

//---------------------------------------------------------------
void Thread::stop()
{
    std::cerr << "Thread::stop() " << id().c_str() << std::endl;

    d->asioContext->post(
        [this]()
        {
            d->asioContext->stop();
        }
    );

    if (d->thread.joinable())
    {
        std::cerr << "Waiting for thread join in thread " << id().c_str()<<"..." << std::endl;
        d->thread.join();
        std::cerr << "Thread joined  in thread " << id().c_str() << std::endl;
    } else {
        std::cerr << "Thread join not needed in thread " << id().c_str() << std::endl;
    }
}

//---------------------------------------------------------------
void Thread::run()
{
    try
    {
        beforeRun();
    }
    catch(std::exception &e)
    {
        HATN_FATAL(thread,"Uncaught exception in beforeRun() in thread " << id().c_str() << ": " << e.what());
        throw;
    }
    d->nativeID=std::this_thread::get_id();

    ThisThread=this;
    memset(ThisThreadId,'\0',sizeof(ThisThreadId));
    if (!d->id.isEmpty())
    {
        memcpy(&ThisThreadId[0],d->id.constData(),d->id.size());
    }

    try
    {
        if (!d->firstRun.load())
        {
            d->asioContext->restart();
        }
        d->firstRun.store(false);

        d->mutex.lock();
        d->running.store(true);
        d->startedCondition.notify_one();
        d->mutex.unlock();

        d->asioContext->run();

        std::cerr<<"Locking mutex after run in thread " << id().c_str() <<"..." << std::endl;
        d->mutex.lock();
        d->running.store(false);
        d->stopped.store(true);
        d->mutex.unlock();
        std::cerr<<"Unlocked mutex after run in thread " << id().c_str() << std::endl;
    }
    catch(std::exception &e)
    {
        HATN_FATAL(thread,"Uncaught exception in thread " << id().c_str() << ": " << e.what());
        throw;
    }

    try
    {
        afterRun();
    }
    catch(std::exception &e)
    {
        HATN_FATAL(thread,"Uncaught exception in afterRun() in thread " << id().c_str() << ": " << e.what());
        throw;
    }
    ThisThread=nullptr;
}

//---------------------------------------------------------------
void Thread::execAsync(
        std::function<void ()> handler
    )
{
    d->handlersPending.fetch_add(1,std::memory_order_acquire);
    d->asioContext->post(
        [this,handler{std::move(handler)}]()
        {
            d->handlersPending.fetch_sub(1,std::memory_order_acquire);
            handler();
        }
    );
}

//---------------------------------------------------------------
int Thread::pendingHandlersCount() const noexcept
{
   return d->handlersPending.load(std::memory_order_relaxed);
}

//---------------------------------------------------------------
Error Thread::execSync(
        std::function<void ()> handler,
        size_t timeoutMs
    )
{
    std::packaged_task<void ()> task(std::move(handler));
    auto future=task.get_future();
    auto taskPtr=&task;
    execAsync(
        [taskPtr]()
        {
            (*taskPtr)();
        }
    );
    if (timeoutMs==0)
    {
        future.wait();
    }
    else if (future.wait_for(std::chrono::milliseconds(timeoutMs))==std::future_status::timeout)
    {
        HATN_DEBUG(thread,"Timeout in execSync()");
        return Error(CommonError::TIMEOUT);
    }
    return Error(CommonError::OK);
}

//---------------------------------------------------------------
Thread::NativeID Thread::nativeID() const noexcept
{
    std::unique_lock<std::mutex> lock(d->mutex);
    return d->nativeID;
}

//---------------------------------------------------------------
Thread* Thread::currentThread() noexcept
{
    return ThisThread;
}

//---------------------------------------------------------------
Thread* Thread::currentThreadOrMain() noexcept
{
    return (ThisThread==nullptr)?MainThread().mainThread.get():ThisThread;
}

//---------------------------------------------------------------
const char* Thread::currentThreadID() noexcept
{
    return &ThisThreadId[0];
}

//---------------------------------------------------------------
void Thread::releaseMainThread() noexcept
{
    MainThread().mainThread.reset();
    ThisThread=nullptr;
}

//---------------------------------------------------------------
bool Thread::isStopped() const noexcept
{
    return !d->running.load() && !d->firstRun.load();
}

//---------------------------------------------------------------
bool Thread::isStarted() const noexcept
{
    return d->running.load();
}

//---------------------------------------------------------------
std::shared_ptr<Thread> Thread::setMainThread(std::shared_ptr<Thread> thread)
{
    Assert(thread!=nullptr,"Main thread can not be null!");
    Assert(!thread->d->newThread,"Main thread must be instantiated with newThread=false!");
    Assert(static_cast<bool>(MainThread().mainThread)==false,"Main thread already exists!");
    MainThread().mainThread=std::move(thread);
    return MainThread().mainThread;
}

//---------------------------------------------------------------
std::shared_ptr<Thread> Thread::mainThread() noexcept
{
    return MainThread().mainThread;
}

//---------------------------------------------------------------
const FixedByteArrayThrow16& Thread::id() const noexcept
{
    return d->id;
}

//---------------------------------------------------------------
uint32_t Thread::installTimer(
            uint64_t timeoutPeriodUs,
            // codechecker_false_positive [performance-unnecessary-value-param]
            std::function<bool ()> handler,
            bool runOnce,
            bool highResolution
    )
{
    std::unique_lock<std::mutex> lock(d->mutex);
    auto id=++d->timerIncId;
    auto timer=std::make_shared<Timer>(timeoutPeriodUs,std::move(handler),runOnce,highResolution,asioContextRef(),[id,this](){this->uninstallTimer(id);});
    d->timers[d->timerIncId]=timer;
    return id;
}

//---------------------------------------------------------------
void Thread::uninstallTimer(uint32_t id)
{
    std::unique_lock<std::mutex> lock(d->mutex);
    d->timers.erase(id);
}

//---------------------------------------------------------------
void Thread::uninstallTimerWait(uint32_t id)
{
    d->mutex.lock();
    auto it=d->timers.find(id);
    if (it!=d->timers.end())
    {
        auto timer=it->second;
        d->mutex.unlock();

        if (!timer->stopped.load(std::memory_order_relaxed))
        {
            timer->stop();
            while (!timer->stopped.load(std::memory_order_relaxed))
            {}
        }

        uninstallTimer(id);
    }
    else
    {
        d->mutex.unlock();
    }
}

template class HATN_COMMON_EXPORT ThreadCategoriesPool<Thread>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
