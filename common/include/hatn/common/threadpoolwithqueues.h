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
};

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END

#endif // HATNTHREADPOOLWITHQUEUES_H
