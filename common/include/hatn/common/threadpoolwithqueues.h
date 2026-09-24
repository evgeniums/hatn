/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/threadpoolwithqueues.h
  *
  *     hatn pool of threads with queues.
  *
  */

/****************************************************************************/

#ifndef HATNTHREADPOOLWITHQUEUES_H
#define HATNTHREADPOOLWITHQUEUES_H

#include <hatn/common/common.h>

#include <hatn/common/threadwithqueue.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename TaskT>
class ThreadPoolWithQueuesTraits_p;

template <typename TaskT>
class ThreadPoolWithQueuesTraits
{
    public:

        ThreadPoolWithQueuesTraits();

        //! Post task
        void postTask(TaskT task);

        //! Post prepared task
        void post(TaskT* task);

        /**
                 * @brief Set new queue
                 * @param queue
                 *
                 * Not thread safe, call it only in setup routines before running thread
                 */
        void setQueue(Queue<TaskT>* queue);

        /**
                 * @brief Prepare queue item and create task object
                 * @return Prepared task object that can be filled in the caller and then pushed back to the queue
                 */
        TaskT* prepare();

        /**
                 * @brief Set max number of tasks per sinle io_context loop (default 64)
                 * @param count
                 *
                 * Set this parameter before starting thread, though it is not critical to set it in runtime but take into account that parameter isn't atomic.
                 */
        void setMaxTasksPerLoop(int count);

        //! Get max number of tasks per single io_context loop
        int maxTasksPerLoop() const;

        //! Check if interface includes current thread
        bool containsCurrentThread() const;

    private:

        std::unique_ptr<ThreadPoolWithQueuesTraits_p<TaskT>> d;

        template <typename T> friend
            class ThreadPoolWithQueues;
};

//! hatn pool of threads with queues
template <typename TaskT>
class ThreadPoolWithQueues : public ThreadQ<TaskT,ThreadPoolWithQueuesTraits>
{
    public:

        //! Constructor
        ThreadPoolWithQueues(
            size_t threadCount, //!< Number of threads in the pool
            const FixedByteArrayThrow16& id, //!< Base id for threads in pool
            Queue<TaskT>* queue=nullptr //!< Master queue object, if null then default queue with mutex is constructed
        );

        //! Start threads
        void start();

        //! Stop threads
        void stop();

        //! Destructor
        ~ThreadPoolWithQueues();

        ThreadPoolWithQueues(const ThreadPoolWithQueues&)=delete;
        ThreadPoolWithQueues(ThreadPoolWithQueues&&) noexcept;
        ThreadPoolWithQueues& operator=(const ThreadPoolWithQueues&)=delete;
        ThreadPoolWithQueues& operator=(ThreadPoolWithQueues&&) noexcept;

        //! Get thread count
        size_t threadCount() const;

        //! Get thread
        ThreadWithQueue<TaskT>* thread(size_t num);

        //! Get shared pointer to thread, e.g. so an owner outside this pool (App) can co-own
        //! the pool's own threads for its own bookkeeping (thread list, tagging, log context).
        std::shared_ptr<ThreadWithQueue<TaskT>> threadShared(size_t num);

        //! Post a plain handler to the pool's least-loaded thread.
        /**
         * Mirrors Thread::execAsync() for callers that only need "run this on some pool
         * thread" and don't need a TaskContext. Unlike Thread::execAsync(), the handler goes
         * through the target thread's task queue rather than boost::asio::post() directly -
         * that is what lets the pool balance load at all (see
         * ThreadPoolWithQueuesTraits_p::selectThread()).
         *
         * Posts with a null task context, deliberately reproducing Thread::execAsync()'s
         * semantics exactly (no beforeThreadProcessing()/afterThreadProcessing() around the
         * handler). If a caller needs those, use postTask()/prepare()+post() with an explicit
         * context instead.
         *
         * Defined inline (not in threadpoolwithqueues.cpp) because HandlerT is a distinct
         * closure type per call site - it cannot be covered by the class's explicit
         * instantiations the way the TaskT-only methods above are.
         */
        template <typename HandlerT>
        void execAsync(HandlerT handler)
        {
            if constexpr (std::is_same<TaskT,TaskWithContext>::value)
            {
                this->postTask(TaskWithContext{
                    [h{std::move(handler)}](const SharedPtr<TaskContext>&) mutable
                    {
                        h();
                    }
                });
            }
            else
            {
                this->postTask(TaskT{std::move(handler)});
            }
        }
};

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END

#endif // HATNTHREADPOOLWITHQUEUES_H
