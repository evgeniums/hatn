/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/threadwithqueue.h
  *
  *     hatn thread with queue.
  *
  */

/****************************************************************************/

#ifndef HATNTHREADWITHQUEUE_H
#define HATNTHREADWITHQUEUE_H

#include <hatn/common/common.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/threadq.h>
#include <hatn/common/thread.h>
#include <hatn/common/queue.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename TaskT>
class ThreadWithQueueTraits_p;

template <typename TaskT>
class ThreadWithQueueTraits
{
    public:

        ThreadWithQueueTraits(Queue<TaskT>* queue);

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

        std::unique_ptr<ThreadWithQueueTraits_p<TaskT>> d;

        //! Post task
        void postImpl(
            TaskT&& task
            );

        //! Post prepared task
        void postImpl(
            TaskT* task
            );

        template <typename Arg> void doPost(Arg&& arg);

        template <typename T> friend
        class ThreadWithQueue;
};

//! Thread with queue tasks
/**
 * Note that queue will be processing in almost infinite loop, so that direct execAsync and execSync might be delayed or never invoked.
 * Still, you can use execAsync or execSync before populating the queue with post() or when the queue is empty.
 */
template <typename TaskT>
class ThreadWithQueue : public Thread, public ThreadQ<TaskT,ThreadWithQueueTraits>
{
    public:

        //! Constructor
        ThreadWithQueue(
            lib::string_view id, //!< Thread's ID
            Queue<TaskT>* queue=nullptr, //!< Queue object, if null then default queue with mutex is constructed
            bool newThread=true //!< If false then no actual thread will be started, only asioContext will run
        );

        //! Destructor
        virtual ~ThreadWithQueue();

        ThreadWithQueue(const ThreadWithQueue&)=delete;
        ThreadWithQueue(ThreadWithQueue&&) =default;
        ThreadWithQueue& operator=(const ThreadWithQueue&)=delete;
        ThreadWithQueue& operator=(ThreadWithQueue&&) =default;

        //! Set queue
        void setQueue(Queue<TaskT>* queue);

        //! Get queue
        Queue<TaskT>* queue() const noexcept;

        //! Get queue depth
        size_t queueDepth() const noexcept;

        //! Is queue empty
        bool isQueueEmpty() const noexcept;

        //! Clear the queue
        void clearQueue();

        //! Set thread queue interface if this thread is a part of thread pool
        void setThreadQ(ThreadQ<TaskT,ThreadWithQueueTraits>* iface) noexcept;

        //! Get thread queue interface if this thread is a part of thread pool
        ThreadQ<TaskT,ThreadWithQueueTraits>* threadQueueInterface() const noexcept;

    protected:

        virtual void beforeRun() override;
};

using TaskQueue=Queue<Task>;
using TaskWithContextQueue=Queue<TaskWithContext>;
using TaskThread=ThreadWithQueue<Task>;
using TaskWithContextThread=ThreadWithQueue<TaskWithContext>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNTHREADWITHQUEUE_H
