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
                stopped.store(true,std::memory_order_relaxed);
                uninstall();
            }
            else
            {
                if (highResTimer!=nullptr)
                {
                    highResTimer->expires_from_now(std::chrono::microseconds(periodUs));
                    highResTimer->async_wait([this](const boost::system::error_code& ec){this->timeout(ec);});
                }
                else
                {
                    normalTimer->expires_from_now(boost::posix_time::microseconds(periodUs));
                    normalTimer->async_wait([this](const boost::system::error_code& ec){this->timeout(ec);});
                }
            }
        }
        else
        {
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
        if (highResolution)
        {
            highResTimer=std::make_unique<boost::asio::high_resolution_timer>(asioContext);
            highResTimer->expires_from_now(std::chrono::microseconds(periodUs));
            highResTimer->async_wait([this](const boost::system::error_code& ec){this->timeout(ec);});
        }
        else
        {
            normalTimer=std::make_unique<boost::asio::deadline_timer>(asioContext);
            normalTimer->expires_from_now(boost::posix_time::microseconds(periodUs));
            normalTimer->async_wait([this](const boost::system::error_code& ec){this->timeout(ec);});
        }
    }

    ~Timer()=default;
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

        bool running=false;
        bool stopped=false;
        bool firstRun=true;
        Thread* startThread=nullptr;

        std::condition_variable condition;
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
            stopped=false;
            startThread=nullptr;
            newThread=newT;
            asioContext=std::make_shared<boost::asio::io_context>(1);
            nativeID=std::this_thread::get_id();
            handlersPending.store(0,std::memory_order_release);
            firstRun=true;
            running=false;

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
    d->mutex.lock();
    if (d->running)
    {
        d->mutex.unlock();
        return;
    }
    d->startThread=Thread::currentThreadOrMain();
    d->stopped=false;
    d->mutex.unlock();

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
            if (!d->running && !d->stopped)
            {
                d->condition.wait(l);
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
    d->mutex.lock();
    if (d->running)
    {
        d->mutex.unlock();

        d->asioContext->post(
            [this]()
            {
                d->asioContext->stop();
            }
        );
        if (d->newThread && Thread::currentThreadOrMain()==d->startThread)
        {
            d->thread.join();
        }

        return;
    }
    d->mutex.unlock();
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
        if (!d->firstRun)
        {
            d->asioContext->restart();
        }
        d->firstRun=false;

        d->mutex.lock();
        d->running=true;
        if (d->newThread)
        {
            d->condition.notify_one();
        }
        d->mutex.unlock();

        d->asioContext->run();

        d->mutex.lock();
        d->running=false;
        d->stopped=true;
        d->mutex.unlock();
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
    return !d->running && !d->firstRun;
}

//---------------------------------------------------------------
bool Thread::isStarted() const noexcept
{
    return d->running;
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
