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

#include <boost/thread/thread.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/executor_work_guard.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

#include <hatn/common/types.h>
#include <hatn/common/utils.h>
#include <hatn/common/logger.h>
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
    boost::asio::io_context& asioContext;
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
                stopped.store(true,std::memory_order_release);
                if (uninstall) {
                    asioContext.post(uninstall);
                }
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
            stopped.store(true,std::memory_order_release);
        }
    }

    Timer(
            uint64_t timeoutPeriodUs,
            const std::function<bool ()>& handler,
            bool runOnce,
            bool highResolution,
            boost::asio::io_context& asioContext,
            const std::function<void ()>& uninstall
        ) : asioContext(asioContext),
            periodUs(timeoutPeriodUs),
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

    ~Timer() {
        stop();
        stopped.store(true,std::memory_order_release);
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

        FixedByteArrayThrow16 id;
        bool newThread=false;

        std::shared_ptr<boost::asio::io_context> asioContext;

        std::atomic<bool> running;
        std::atomic<bool> stopped;
        std::atomic<bool> firstRun;

        std::condition_variable startedCondition;

        std::mutex* mx;
        std::thread* th;

        std::mutex& mutex;
        std::thread& thread;

        std::thread::id nativeID;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard;

        std::atomic<int> handlersPending;

        std::map<uint32_t,std::shared_ptr<Timer>> timers;
        uint32_t timerIncId;

        Thread_p(
                FixedByteArrayThrow16 id,
                bool newThread
            ) : id(std::move(id)),
                newThread(newThread),
                asioContext(std::make_shared<boost::asio::io_context>(1)),
                mx(new std::mutex),
                th(new std::thread),
                mutex(*mx),
                thread(*th),
                nativeID(std::this_thread::get_id()),
                workGuard(boost::asio::make_work_guard(*asioContext)),
                timerIncId(0)
        {
            stopped.store(false);
            handlersPending.store(0);
            firstRun.store(true);
            running.store(false);
        }
};

//---------------------------------------------------------------
Thread::Thread(
        FixedByteArrayThrow16 id,
        bool newThread
    ) : d(std::make_unique<Thread_p>(std::move(id),newThread))
{
}

//---------------------------------------------------------------
Thread::~Thread()
{
    d->workGuard.reset();
    stop();

    std::cerr<<"Resetting asion context in thread " << id().c_str() << std::endl;
    d->asioContext.reset();

    std::cerr<<"Deleting mutex in thread " << id().c_str() << std::endl;
    delete d->mx;

    std::cerr<<"Deleting thread in thread " << id().c_str() << std::endl;
    delete d->th;

    std::cerr<<"Clearing timers in thread " << id().c_str() << std::endl;
    d->timers.clear();

    std::cerr<<"Deleting d in thread " << id().c_str() << std::endl;
    auto ii=id();
    d.reset();

    std::cerr<<"End of thread destructor in thread " << ii.c_str() << std::endl;
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
    if (d->stopped.load())
    {
        d->asioContext->restart();
        d->stopped.store(false);
    }

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
            std::unique_lock<std::mutex> l(d->mutex);
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
    d->asioContext->stop();
    if (d->thread.joinable())
    {
        d->thread.join();
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

        {
            std::unique_lock<std::mutex> l(d->mutex);
            d->firstRun.store(false,std::memory_order_release);
            d->running.store(true,std::memory_order_release);
            d->startedCondition.notify_one();
        }

        d->asioContext->run();

        {
            std::unique_lock<std::mutex> l(d->mutex);
            for (auto&& it : d->timers)
            {
                it.second->stopped.store(true,std::memory_order_release);
            }
            d->running.store(false,std::memory_order_release);
            d->stopped.store(true,std::memory_order_release);
        }
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
void Thread::uninstallTimer(uint32_t id, bool wait)
{
    std::shared_ptr<Timer> timer;
    {
        std::unique_lock<std::mutex> lock(d->mutex);
        auto it=d->timers.find(id);
        if (it!=d->timers.end())
        {
            timer=it->second;
            d->timers.erase(id);
        }
    }

    if (timer) {
        timer->stop();
        if (wait) {
            while (!timer->stopped.load(std::memory_order_acquire))
            {}
        }
    }
}

template class HATN_COMMON_EXPORT ThreadCategoriesPool<Thread>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
