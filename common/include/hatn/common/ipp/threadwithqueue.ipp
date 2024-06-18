/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/ipp/threadwithqueue.ipp
  *
  *     hatn thread with queue
  */

#ifndef HATNTHREADWITHQUEUE_IPP
#define HATNTHREADWITHQUEUE_IPP

#include <hatn/common/mutexqueue.h>

#include <hatn/common/threadqueueinterface.h>
#include <hatn/common/threadwithqueue.h>

#include <hatn/common/logger.h>

DECLARE_LOG_MODULE(thread)

HATN_COMMON_NAMESPACE_BEGIN

/********************** ThreadWithQueueTraits **************************/

//#define TEST_BOUNDARIES

template <typename TaskT>
class ThreadWithQueueTraits_p
{
    public:

        ThreadWithQueueTraits<TaskT> *traits;

        Thread* thread;
        std::unique_ptr<Queue<TaskT>> queue;
        TaskT* currentTask;
        QueueItem* currentItem;
        int maxHandlersPerLoop=0;
        std::atomic<int> lockForAdd;

        ThreadQueueInterface<TaskT,ThreadWithQueueTraits>* threadQueueInterface;

        ThreadWithQueueTraits_p(
                ThreadWithQueueTraits<TaskT> *traits,
                Queue<TaskT>* q
            ) : traits(traits),
                queue(q),
                currentTask(nullptr),
                currentItem(nullptr),
                lockForAdd(0),
                threadQueueInterface(nullptr)
        {
            if (queue==nullptr)
            {
                queue=std::move(std::make_unique<MutexQueue<TaskT>>());
            }
        }

        //! Loop processing tasks
        template <typename T=TaskT> void taskLoop()
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
template <typename TaskT>
ThreadWithQueueTraits<TaskT>::ThreadWithQueueTraits(
        Queue<TaskT>* queue
    ) : d(std::make_unique<ThreadWithQueueTraits_p<TaskT>>(this,queue))
{
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadWithQueueTraits<TaskT>::postImpl(
    TaskT&& task
    )
{
    d->queue->push(std::move(task));
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadWithQueueTraits<TaskT>::postImpl(
    TaskT* task
    )
{
    d->queue->pushItem(task->queueItem);
}

//---------------------------------------------------------------
template <typename TaskT>
template <typename Arg>
void ThreadWithQueueTraits<TaskT>::doPost(
    Arg&& task
    )
{
    d->queue->incPostingRefCount();
    bool scheduleAsync=d->queue->isEmpty();
    postImpl(std::forward<Arg>(task));
    d->queue->decPostingRefCount();
    if (scheduleAsync)
    {
        d->thread->execAsync(
            [this]()
            {
                this->d->taskLoop();
            }
        );
    }
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadWithQueueTraits<TaskT>::post(
        TaskT* task
    )
{
    doPost(task);
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadWithQueueTraits<TaskT>::postTask(
    TaskT task
    )
{
    doPost(std::move(task));
}

//---------------------------------------------------------------
template <typename TaskT>
TaskT* ThreadWithQueueTraits<TaskT>::prepare()
{
    auto* item=d->queue->prepare();
    auto* task=&item->m_val;
    task->queueItem=item;
    return task;
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadWithQueueTraits<TaskT>::setMaxTasksPerLoop(int count)
{
    d->maxHandlersPerLoop=count;
}

//---------------------------------------------------------------
template <typename TaskT>
int ThreadWithQueueTraits<TaskT>::maxTasksPerLoop() const
{
    return d->maxHandlersPerLoop;
}

//---------------------------------------------------------------

template <typename TaskT>
bool ThreadWithQueueTraits<TaskT>::containsCurrentThread() const
{
    return Thread::currentThread()==d->thread;
}

//---------------------------------------------------------------

template <typename TaskT>
void ThreadWithQueueTraits<TaskT>::setQueue(Queue<TaskT>* queue)
{
    d->queue.reset(queue);
}

/********************** ThreadWithQueue **************************/

//---------------------------------------------------------------
template <typename TaskT>
ThreadWithQueue<TaskT>::ThreadWithQueue(
        const FixedByteArrayThrow16& id,
        Queue<TaskT>* queue,
        bool newThread
    ) : Thread(id,newThread),
        ThreadQueueInterface<TaskT,ThreadWithQueueTraits>(queue)
{
    setThreadQueueInterface(this);
    this->traits().d->thread=this;
}

//---------------------------------------------------------------
template <typename TaskT>
ThreadWithQueue<TaskT>::~ThreadWithQueue()
{
    clearQueue();
}

//---------------------------------------------------------------
template <typename TaskT>
size_t ThreadWithQueue<TaskT>::queueDepth() const noexcept
{
    return this->traits().d->queue->size();
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadWithQueue<TaskT>::clearQueue()
{
    if (isStopped() || !isStarted())
    {
        this->traits().d->queue->clear();
    }
    else
    {
        if (
            execSync(
                [this]()
                {
                    this->traits().d->queue->clear();
                }
            )
        )
        {
            HATN_WARN(thread,"Timeout in clear queue");
        }
    }
}

//---------------------------------------------------------------
template <typename TaskT>
bool ThreadWithQueue<TaskT>::isQueueEmpty() const noexcept
{
    return this->traits().d->queue->isEmpty();
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadWithQueue<TaskT>::setQueue(Queue<TaskT>* queue)
{
    Assert(queue,"Queue can not be nullptr");
    this->traits().d->queue.reset(queue);
}

//---------------------------------------------------------------
template <typename TaskT>
Queue<TaskT>* ThreadWithQueue<TaskT>::queue() const noexcept
{
    return this->traits().d->queue.get();
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadWithQueue<TaskT>::beforeRun()
{
    this->setCurrent(this->traits().d->threadQueueInterface);
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadWithQueue<TaskT>::setThreadQueueInterface(ThreadQueueInterface<TaskT,ThreadWithQueueTraits>* interface) noexcept
{
    this->traits().d->threadQueueInterface=interface;
}

//---------------------------------------------------------------
template <typename TaskT>
ThreadQueueInterface<TaskT,ThreadWithQueueTraits>* ThreadWithQueue<TaskT>::threadQueueInterface() const noexcept
{
    return this->traits().d->threadQueueInterface;
}

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END

#endif // HATNTHREADWITHQUEUE_IPP
