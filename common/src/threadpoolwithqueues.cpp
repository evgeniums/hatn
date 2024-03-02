/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/threadpoolwithqueues.сpp
  *
  *     hatn pool of threads with queues.
  */

#include <set>

#include <hatn/common/types.h>

#include <hatn/common/threadcategoriespoolimpl.h>
#include <hatn/common/threadpoolwithqueues.h>

HATN_COMMON_NAMESPACE_BEGIN

/********************** ThreadPoolWithQueues **************************/

template <typename TaskType> class ThreadPoolWithQueues_p
{
    public:

        std::set<ThreadWithQueue<TaskType>*> threads;
        int maxHandlersPerLoop=0;
        std::vector<std::shared_ptr<ThreadWithQueue<TaskType>>> threadStorage;

        ThreadWithQueue<TaskType>* selectThread()
        {
            ThreadWithQueue<TaskType>* thread=*threads.begin();
            size_t minDepth=std::numeric_limits<size_t>::max();
            for (auto&& it:threads)
            {
                auto depth=it->queueDepth();
                if (depth==0)
                {
                    return it;
                }
                if (depth<minDepth)
                {
                    minDepth=depth;
                    thread=it;
                }
            }
            return thread;
        }
};

//---------------------------------------------------------------
template <typename TaskType> ThreadPoolWithQueues<TaskType>::ThreadPoolWithQueues(
        size_t threadCount,
        const FixedByteArrayThrow16& id,
        Queue<TaskType>* queue
    ) : d(std::make_unique<ThreadPoolWithQueues_p<TaskType>>())
{
    Assert(threadCount>0,"Thread count must be positive number");
    auto i=threadCount;
    while(i>0)
    {
        Queue<TaskType>* threadQueue=nullptr;
        if (i==1)
        {
            threadQueue=queue;
        }
        else if (queue!=nullptr)
        {
            threadQueue=queue->buildQueue();
        }
        std::string name(id.c_str());
        std::string num=std::to_string(i);
        auto nameLength=name.length()+num.length();
        if (nameLength>16)
        {
            name=name.substr(0,16-num.length());
        }
        name+=num;
        auto thread=std::make_shared<ThreadWithQueue<TaskType>>(name.c_str(),threadQueue);
        thread->setThreadQueueInterface(this);
        d->threadStorage.push_back(thread);
        d->threads.insert(thread.get());
        --i;
    }
}

//---------------------------------------------------------------
template <typename TaskType> ThreadPoolWithQueues<TaskType>::~ThreadPoolWithQueues()
{
    stop();
}

template <typename TaskType> ThreadPoolWithQueues<TaskType>::ThreadPoolWithQueues(ThreadPoolWithQueues<TaskType>&&) noexcept=default;
template <typename TaskType> ThreadPoolWithQueues<TaskType>& ThreadPoolWithQueues<TaskType>::operator=(ThreadPoolWithQueues<TaskType>&&) noexcept=default;

//---------------------------------------------------------------
template <typename TaskType> void ThreadPoolWithQueues<TaskType>::doPostTask(
        TaskType task
    )
{
    d->selectThread()->postTask(std::move(task));
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadPoolWithQueues<TaskType>::post(
        TaskType* task
    )
{
    if (task->queueItem->m_data!=nullptr)
    {
        auto thread=reinterpret_cast<ThreadWithQueue<TaskType>*>(task->queueItem->m_data);
        thread->post(task);
    }
}

//---------------------------------------------------------------
template <typename TaskType> TaskType* ThreadPoolWithQueues<TaskType>::prepare()
{
    auto thread=d->selectThread();
    auto task=thread->prepare();
    task->queueItem->m_data=thread;
    return task;
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadPoolWithQueues<TaskType>::setMaxTasksPerLoop(int count)
{
    d->maxHandlersPerLoop=count;
    for (auto&& it:d->threads)
    {
        it->setMaxTasksPerLoop(count);
    }
}

//---------------------------------------------------------------
template <typename TaskType> int ThreadPoolWithQueues<TaskType>::maxTasksPerLoop() const
{
    return d->maxHandlersPerLoop;
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadPoolWithQueues<TaskType>::setQueue(Queue<TaskType>* queue)
{
    Assert(queue,"Queue can not be nullptr");
    auto i=d->threads.size();
    for (auto&& it:d->threads)
    {
        if (--i==0)
        {
            it->setQueue(queue);
        }
        else
        {
            it->setQueue(queue->buildQueue());
        }
    }
}

//---------------------------------------------------------------
template <typename TaskType> bool ThreadPoolWithQueues<TaskType>::containsCurrentThread() const
{
    auto current=Thread::currentThread();
    for (auto&& it:d->threads)
    {
        if (current==it)
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadPoolWithQueues<TaskType>::start()
{
    for (auto&& it:d->threads)
    {
        it->start();
    }
}

//---------------------------------------------------------------
template <typename TaskType> void ThreadPoolWithQueues<TaskType>::stop()
{
    for (auto&& it:d->threads)
    {
        it->stop();
    }
}

//---------------------------------------------------------------
template <typename TaskType> size_t ThreadPoolWithQueues<TaskType>::threadCount() const
{
    return d->threadStorage.size();
}

//---------------------------------------------------------------
template <typename TaskType> ThreadWithQueue<TaskType>* ThreadPoolWithQueues<TaskType>::thread(size_t num)
{
    if (num>d->threadStorage.size())
    {
        return nullptr;
    }
    return d->threadStorage[num].get();
}

//---------------------------------------------------------------
template class HATN_COMMON_EXPORT ThreadPoolWithQueues<Task>;
template class HATN_COMMON_EXPORT ThreadPoolWithQueues<TaskWithContext>;

template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadPoolWithQueues<Task>>;
template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadPoolWithQueues<TaskWithContext>>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
