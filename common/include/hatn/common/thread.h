/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/thread.h
  *
  *     hatn thread.
  *
  */

/****************************************************************************/

#ifndef HATNTHREAD_H
#define HATNTHREAD_H

#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include <future>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

#endif

#include <boost/asio/io_context.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

#include <hatn/common/common.h>
#include <hatn/common/error.h>
#include <hatn/common/fixedbytearray.h>

// Default timeout for execSync() calls. The prior hardcoded 180000ms (3 min) default is far too
// long for a client app to hold as a fallback — observed real closes take at most a few seconds,
// and on iOS the ENTIRE background-execution budget is only ~30s, so a wedged execSync() call
// could alone blow through the whole window. Platform presets below; override at configure time
// via -DHATN_EXECSYNC_TIMEOUT_MS=<ms> or the HATN_EXECSYNC_TIMEOUT_MS env var (see CMakeLists.txt).
#ifndef HATN_EXECSYNC_TIMEOUT_MS
    #if defined(BUILD_ANDROID) || defined(BUILD_IOS)
        #define HATN_EXECSYNC_TIMEOUT_MS 10000
    #else
        #define HATN_EXECSYNC_TIMEOUT_MS 30000
    #endif
#endif

HATN_COMMON_NAMESPACE_BEGIN

using ThreadId=FixedByteArrayThrow16;

class Thread_p;

//! hatn thread
class HATN_COMMON_EXPORT Thread : public std::enable_shared_from_this<Thread>
{
    public:

        typedef std::thread::id NativeID;

        //! Constructor
        explicit Thread(
            lib::string_view id="unknown", //!< Thread's ID
            bool newThread=true //!< If false then no actual thread will be started, only asioContext will run
        );

        virtual ~Thread();
        Thread(const Thread&) =delete;
        Thread(Thread&&) noexcept;
        Thread& operator=(const Thread&) =delete;
        Thread& operator=(Thread&&) noexcept;

        //! Start thread
        void start(bool waitForStarted=true);

        //! Stop thread
        void stop();

        //! Get thread's io service
        std::shared_ptr<boost::asio::io_context> asioContext() const noexcept;

        //! Get thread's io service
        boost::asio::io_context& asioContextRef() noexcept;

        //! Exec async function in thread
        void execAsync(
            lib::move_only_function<void()> handler
        );

        /**
         * @brief Exec sync function in thread without returning any result
         * @param handler Handler to invoke
         * @param timeoutMs Period to wait for, default is HATN_EXECSYNC_TIMEOUT_MS
         * @return Execution status
         */
        Error execSync(
            std::function<void()> handler,
            size_t timeoutMs=HATN_EXECSYNC_TIMEOUT_MS
        );

        /**
         * @brief Exec sync function in thread returning some result
         * @param handler Handler to invoke
         * @param timeoutMs Period to wait the result for, default is HATN_EXECSYNC_TIMEOUT_MS
         * @return T result
         *
         * @throws ErrorException on timeout
         */
        template <typename T>
        T execSync(
                std::function<T ()> handler,
                size_t timeoutMs=HATN_EXECSYNC_TIMEOUT_MS
            )
        {
            auto currentThread=currentThreadOrMain();
            Assert(currentThread,"Current thread or main must be initialized");
            if (id()==currentThread->id())
            {
                return handler();
            }

            // Task is heap-allocated and kept alive via shared_ptr (not a stack-local pointed to
            // by a raw `taskPtr`): if this call times out below, the caller may return/unwind
            // while the task is still queued on the target thread. A raw pointer to a stack
            // object would then be a use-after-free when the queued lambda eventually runs;
            // the shared_ptr keeps the task valid regardless of how long it takes to drain.
            // Mirrors the existing execFuture() pattern below.
            auto task=std::make_shared<std::packaged_task<T ()>>(std::move(handler));
            auto future=task->get_future();
            execAsync(
                [task]()
                {
                    (*task)();
                }
            );
            if (timeoutMs==0)
            {
                future.wait();
            }
            else if (future.wait_for(std::chrono::milliseconds(timeoutMs))==std::future_status::timeout)
            {
                throw ErrorException(commonError(CommonError::TIMEOUT));
            }
            return future.get();
        }

        //! Exec async function in thread and return future object
        template <typename T> std::future<T> execFuture(
                std::function<T ()> handler
            )
        {
            auto task=std::make_shared<std::packaged_task<T ()>>(std::move(handler));
            auto future=task->get_future();
            execAsync(
                [task]()
                {
                    (*task)();
                }
            );
            return future;
        }

        /**
         * @brief Install timer
         * @param timeoutPeriod Timer period in microseconds
         * @param handler Handler to invoke, return false to disable and uninstall timer after invokation
         * @param runOnce If true then run once and delete timer after invokation
         * @param highResolution Use high resolution timer or not
         * @return Timer ID
         *
         */
        uint32_t installTimer(
            uint64_t timeoutPeriodUs,
            std::function<bool()> handler,
            bool runOnce=false,
            bool highResolution=false
        );

        /**
         * @brief Uninstall timer
         * @param id Timer ID
         */
        void uninstallTimer(uint32_t id, bool wait=true);

        //! Get native thread id
        Thread::NativeID nativeID() const noexcept;

        //! Get current thread
        static Thread* currentThread() noexcept;

        //! Get current or main thread
        static Thread* currentThreadOrMain() noexcept;

        /**
         * @brief Set the main thread
         * @param thread Thread to set as main. @attention This thread must be created with newThread=false.
         * @return STL shared_ptr to main thread
         */
        static std::shared_ptr<Thread> setMainThread(std::shared_ptr<Thread> thread);

        //! Get main thread
        static std::shared_ptr<Thread> mainThread() noexcept;

        //! Release main thread
        static void releaseMainThread() noexcept;

        //! Comparation operator
        inline bool operator==(const Thread& other) const noexcept
        {
            bool ok=(nativeID()==other.nativeID());
            return ok;
        }

        //! Check if thread is stopped
        bool isStopped() const noexcept;

        //! Check if thread is started
        bool isStarted() const noexcept;

        //! Get ID
        const ThreadId& id() const noexcept;

        //! Get number of pending handlers
        int pendingHandlersCount() const noexcept;

        //! Get ID of current thread
        static const char* currentThreadID() noexcept;

        void setTag(std::string tag);
        void unsetTag(const std::string& tag);
        bool hasTag(lib::string_view tag) const;

    protected:

        //! Routine called in thread before running io service
        virtual void beforeRun(){}

        //! Routine called in thread after exiting io service
        virtual void afterRun(){}

    private:

        //! Run thread
        void run();

        std::unique_ptr<Thread_p> d;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNTHREAD_H
