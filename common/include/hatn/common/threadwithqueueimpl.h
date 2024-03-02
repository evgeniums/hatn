/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/threadwithqueueimpl.h
  *
  *     Dracosha thread with queue
  */

#ifndef HATNTHREADWITHQUEUEIMPL_H
#define HATNTHREADWITHQUEUEIMPL_H

#include <hatn/common/mutexqueue.h>

#include <hatn/common/threadqueueinterface.h>
#include <hatn/common/threadwithqueue.h>

#include <hatn/common/logger.h>

DECLARE_LOG_MODULE(thread)

HATN_COMMON_NAMESPACE_BEGIN

/********************** ThreadWithQueue **************************/

//#define TEST_BOUNDARIES

template <typename TaskType> class ThreadWithQueue_p
{
    public:

        Thread* thread;
        std::unique_ptr<Queue<TaskType>> queue;
        TaskType* currentTask;
        QueueItem* currentItem;
        int maxHandlersPerLoop=0;
        std::atomic<int> lockForAdd;

        ThreadQueueInterface<TaskType>* threadQueueInterface;

        ThreadWithQueue_p(
                Thread* thread,
                Queue<TaskType>* q
            ) : thread(thread),
                queue(q),
                currentTask(nullptr),
                currentItem(nullptr),
                lockForAdd(0),
                threadQueueInterface(nullptr)
        {
            if (queue==nullptr)
            {
                queue=std::move(std::make_unique<MutexQueue<TaskType>>());
            }
        }

        //! Loop processing tasks
        template <typename T=TaskType> void taskLoop()
        {
#ifdef TEST_BOUNDARIES
            static int lockedCount=0;
            bool locked=false;
#endif
#if 0
            std::cout<<"Start queue loop, size="<<queue->size()<<std::endl;
#endif
            int processedCount=0;
            while (!thread->isStopped())
            {
                bool dequeued=queue->popValAndItem(currentTask,currentItem);
                if (dequeued)
                {
                    (*currentTask)();
                    queue->freeItem(currentItem);
                    if (maxHandlersPerLoop!=0 && ++processedCount>=maxHandlersPerLoop)
                    {
                        thread->execAsync(
                            [this]()
                            {
                                taskLoop();
                            }
                        );
                        break;
                    }
                }
                bool breakLoop=!dequeued && queue->postingRefCount()==0;
                if (breakLoop && queue->isEmpty())
                {
#ifdef TEST_BOUNDARIES
                    std::cout<<"Breaking queue loop, queue size="<<queue->size()<<std::endl;
#endif
                    break;
                }
#ifdef TEST_BOUNDARIES
                else
                {
                    if (!locked)
                    {
                        locked=true;
                        std::cout<<"Spinning in locked state, size="<<queue->size()<<", count="<<++lockedCount<<std::endl;
                    }
                }
#endif
            }
        }
};

//---------------------------------------------------------------
template <typename TaskType> ThreadWithQueue<TaskType>::ThreadWithQueue(
        const FixedByteArrayThrow16& id,
        Queue<TaskType>* queue,
        bool newThread
    ) : Thread(id,newThread),
        d1(std::make_unique<ThreadWithQueue_p<TaskType>>(this,queue))
{
    setThreadQueueInterface(this);
}

//---------------------------------------------------------------
template <typename TaskType> ThreadWithQueue<TaskType>::~ThreadWithQueue()
{
    clearQueue();
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadWithQueue<TaskType>::postImpl(
        TaskType&& task
    )
{
    d1->queue->push(std::move(task));
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadWithQueue<TaskType>::postImpl(
        TaskType* task
    )
{
    d1->queue->pushItem(task->queueItem);
}

//---------------------------------------------------------------
template <typename TaskType>
template <typename Arg>
void ThreadWithQueue<TaskType>::doPost(
        Arg&& task
    )
{
    d1->queue->incPostingRefCount();
    bool scheduleAsync=d1->queue->isEmpty();
    postImpl(std::forward<Arg>(task));
    d1->queue->decPostingRefCount();
    if (scheduleAsync)
    {
        execAsync(
            [this]()
            {
                this->d1->taskLoop();
            }
        );
    }
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadWithQueue<TaskType>::doPostTask(
        TaskType task
    )
{
    doPost(std::move(task));
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadWithQueue<TaskType>::post(
        TaskType* task
    )
{
    doPost(task);
}

//---------------------------------------------------------------
template <typename TaskType> size_t ThreadWithQueue<TaskType>::queueDepth() const noexcept
{
    return d1->queue->size();
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadWithQueue<TaskType>::clearQueue()
{
    if (isStopped() || !isStarted())
    {
        d1->queue->clear();
    }
    else
    {
        if (
            execSync(
                [this]()
                {
                    this->d1->queue->clear();
                }
            )
        )
        {
            HATN_WARN(thread,"Timeout in clear queue");
        }
    }
}

//---------------------------------------------------------------
template <typename TaskType> bool ThreadWithQueue<TaskType>::isQueueEmpty() const noexcept
{
    return d1->queue->isEmpty();
}

//---------------------------------------------------------------
template <typename TaskType> TaskType* ThreadWithQueue<TaskType>::prepare()
{
    auto* item=d1->queue->prepare();
    auto* task=&item->m_val;
    task->queueItem=item;
    return task;
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadWithQueue<TaskType>::setMaxTasksPerLoop(int count)
{
    d1->maxHandlersPerLoop=count;
}

//---------------------------------------------------------------
template <typename TaskType> int ThreadWithQueue<TaskType>::maxTasksPerLoop() const
{
    return d1->maxHandlersPerLoop;
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadWithQueue<TaskType>::setQueue(Queue<TaskType>* queue)
{
    Assert(queue,"Queue can not be nullptr");
    d1->queue.reset(queue);
}

//---------------------------------------------------------------
template <typename TaskType> Queue<TaskType>* ThreadWithQueue<TaskType>::queue() const noexcept
{
    return d1->queue.get();
}

//---------------------------------------------------------------
template <typename TaskType> bool ThreadWithQueue<TaskType>::containsCurrentThread() const
{
    return Thread::currentThread()==this;
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadWithQueue<TaskType>::beforeRun()
{
    this->setCurrentThreadInterface(d1->threadQueueInterface);
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadWithQueue<TaskType>::setThreadQueueInterface(ThreadQueueInterface<TaskType>* interface) noexcept
{
    d1->threadQueueInterface=interface;
}

//---------------------------------------------------------------
template <typename TaskType> ThreadQueueInterface<TaskType>* ThreadWithQueue<TaskType>::threadQueueInterface() const noexcept
{
    return d1->threadQueueInterface;
}

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNTHREADWITHQUEUEIMPL_H
