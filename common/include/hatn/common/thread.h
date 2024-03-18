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

HATN_COMMON_NAMESPACE_BEGIN

class Thread_p;

//! hatn thread
class HATN_COMMON_EXPORT Thread : public std::enable_shared_from_this<Thread>
{
    public:

        typedef std::thread::id NativeID;

        //! Constructor
        Thread(
            FixedByteArrayThrow16 id="unknown", //!< Thread's ID
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
            std::function<void()> handler
        );

        /**
         * @brief Exec sync function in thread without returning any result
         * @param handler Handler to invoke
         * @param timeoutMs Period to wait for, default is 3 minutes
         * @return Execution status
         */
        Error execSync(
            std::function<void()> handler,
            size_t timeoutMs=180000
        );

        /**
         * @brief Exec sync function in thread returning some result
         * @param handler Handler to invoke
         * @param timeoutMs Period to wait the result for, default is 3 minutes
         * @return T result
         *
         * @throws ErrorException on timeout
         */
        template <typename T>
        T execSync(
                std::function<T ()> handler,
                size_t timeoutMs=180000
            )
        {
            std::packaged_task<T ()> task(std::move(handler));
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
         * @param thread Thread to set as main. \attention This thread must be created with newThread=false.
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
        const FixedByteArrayThrow16& id() const noexcept;

        //! Get number of pending handlers
        int pendingHandlersCount() const noexcept;

        //! Get ID of current thread
        static const char* currentThreadID() noexcept;

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
