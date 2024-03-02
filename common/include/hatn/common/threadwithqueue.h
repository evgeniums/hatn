/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/threadwithqueue.h
  *
  *     Dracosha thread with queue.
  *
  */

/****************************************************************************/

#ifndef HATNTHREADWITHQUEUE_H
#define HATNTHREADWITHQUEUE_H

#include <hatn/common/common.h>
#include <hatn/common/threadqueueinterface.h>
#include <hatn/common/thread.h>
#include <hatn/common/queue.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename TaskType> class ThreadWithQueue_p;

//! Dracosha thread with queue tasks
/**
 * Note that queue will be processing in almost infinite loop, so that direct execAsync and execSync might be delayed or never invoked.
 * Still, you can use execAsync or execSync before populating the queue with post() or when the queue is empty.
 */
template <typename TaskType> class ThreadWithQueue : public Thread, public ThreadQueueInterface<TaskType>
{
    public:

        //! Constructor
        ThreadWithQueue(
            const FixedByteArrayThrow16& id, //!< Thread's ID
            Queue<TaskType>* queue=nullptr, //!< Queue object, if null then default queue with mutex is constructed
            bool newThread=true //!< If false then no actual thread will be started, only asioContext will run
        );

        //! Destructor
        virtual ~ThreadWithQueue();

        ThreadWithQueue(const ThreadWithQueue&)=delete;
        ThreadWithQueue(ThreadWithQueue&&) =default;
        ThreadWithQueue& operator=(const ThreadWithQueue&)=delete;
        ThreadWithQueue& operator=(ThreadWithQueue&&) =default;

        //! Post prepared task
        virtual void post(
            TaskType* task
        ) override;

        /**
         * @brief Set new queue
         * @param queue
         *
         * Not thread safe, call it only in setup routines before running thread
         */
        virtual void setQueue(Queue<TaskType>* queue) override;

        //! Get queue
        Queue<TaskType>* queue() const noexcept;

        //! Get queue depth
        size_t queueDepth() const noexcept;

        //! Is queue empty
        bool isQueueEmpty() const noexcept;

        //! Clear the queue
        void clearQueue();

        /**
         * @brief Prepare queue item and create task object
         * @return Prepared task object that can be filled in the caller and then pushed back to the queue
         */
        virtual TaskType* prepare() override;

        /**
         * @brief Set max number of tasks per sinle io_context loop (default 64)
         * @param count
         *
         * Set this parameter before starting thread, though it is not critical to set it in runtime but take into account that parameter isn't atomic.
         */
        virtual void setMaxTasksPerLoop(int count) override;

        //! Get max number of tasks per single io_context loop
        virtual int maxTasksPerLoop() const override;

        //! Check if interface includes current thread
        virtual bool containsCurrentThread() const override;

        //! Set thread queue interface if this thread is a part of thread pool
        void setThreadQueueInterface(ThreadQueueInterface<TaskType>* interface) noexcept;

        //! Get thread queue interface if this thread is a part of thread pool
        ThreadQueueInterface<TaskType>* threadQueueInterface() const noexcept;

    protected:

        virtual void beforeRun() override;

        virtual void doPostTask(
            TaskType task
        ) override;

    private:

       std::unique_ptr<ThreadWithQueue_p<TaskType>> d1;

       //! Post task
       void postImpl(
           TaskType&& task
       );

       //! Post prepared task
       void postImpl(
           TaskType* task
       );

       template <typename Arg> void doPost(Arg&& arg);
};

using TaskQueue=Queue<Task>;
using TaskWithContextQueue=Queue<TaskWithContext>;
using TaskThread=ThreadWithQueue<Task>;
using TaskWithContextThread=ThreadWithQueue<TaskWithContext>;

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNTHREADWITHQUEUE_H
