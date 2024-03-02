/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/threadpoolwithqueues.h
  *
  *     Dracosha pool of threads with queues.
  *
  */

/****************************************************************************/

#ifndef HATNTHREADPOOLWITHQUEUES_H
#define HATNTHREADPOOLWITHQUEUES_H

#include <hatn/common/common.h>

#include <hatn/common/threadwithqueue.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename TaskType> class ThreadPoolWithQueues_p;

//! Dracosha pool of threads with queues
template <typename TaskType> class ThreadPoolWithQueues : public ThreadQueueInterface<TaskType>
{
    public:

        //! Constructor
        ThreadPoolWithQueues(
            size_t threadCount, //!< Number of threads in the pool
            const FixedByteArrayThrow16& id, //!< Base id for threads in pool
            Queue<TaskType>* queue=nullptr //!< Master queue object, if null then default queue with mutex is constructed
        );

        //! Start threads
        void start();

        //! Stop threads
        void stop();

        //! Destructor
        virtual ~ThreadPoolWithQueues();

        ThreadPoolWithQueues(const ThreadPoolWithQueues&)=delete;
        ThreadPoolWithQueues(ThreadPoolWithQueues&&) noexcept;
        ThreadPoolWithQueues& operator=(const ThreadPoolWithQueues&)=delete;
        ThreadPoolWithQueues& operator=(ThreadPoolWithQueues&&) noexcept;

        //! Post prepared task
        void post(
            TaskType* task
        ) override;

        /**
         * @brief Set new queue
         * @param queue
         *
         * Not thread safe, call it only in setup routines before running threads
         */
        void setQueue(Queue<TaskType>* queue) override;

        /**
         * @brief Prepare queue item and create task object
         * @return Prepared task object that can be filled in the caller and then pushed back to the queue
         */
        TaskType* prepare() override;

        /**
         * @brief Set max number of tasks per sinle io_context loop (default 64)
         * @param count
         *
         * Set this parameter before starting thread, though it is not critical to set it in runtime but take into account that parameter isn't atomic.
         */
        void setMaxTasksPerLoop(int count) override;

        //! Get max number of tasks per single io_context loop
        int maxTasksPerLoop() const override;

        //! Check if interface includes current thread
        bool containsCurrentThread() const override;

        //! Get thread count
        size_t threadCount() const;

        //! Get thread
        ThreadWithQueue<TaskType>* thread(size_t num);

    private:

        virtual void doPostTask(
            TaskType task
        ) override;

        std::unique_ptr<ThreadPoolWithQueues_p<TaskType>> d;
};

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNTHREADPOOLWITHQUEUES_H
