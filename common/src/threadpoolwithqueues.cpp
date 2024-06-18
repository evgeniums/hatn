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

#include <hatn/common/threadpoolwithqueues.h>

#include <hatn/common/ipp/threadcategoriespool.ipp>

HATN_COMMON_NAMESPACE_BEGIN

/********************** ThreadPoolWithQueuesTraits **************************/

template <typename TaskT>
class ThreadPoolWithQueuesTraits_p
{
    public:

        //! @todo Use flatmap set
        std::set<ThreadWithQueue<TaskT>*> threads;
        int maxHandlersPerLoop=0;
        std::vector<std::shared_ptr<ThreadWithQueue<TaskT>>> threadStorage;

        ThreadWithQueue<TaskT>* selectThread()
        {
            ThreadWithQueue<TaskT>* thread=*threads.begin();
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

template <typename TaskT>
ThreadPoolWithQueuesTraits<TaskT>::ThreadPoolWithQueuesTraits(
    ) : d(std::make_unique<ThreadPoolWithQueuesTraits_p<TaskT>>())
{}

//---------------------------------------------------------------

template <typename TaskT>
void ThreadPoolWithQueuesTraits<TaskT>::postTask(
    TaskT task
    )
{
    d->selectThread()->postTask(std::move(task));
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadPoolWithQueuesTraits<TaskT>::post(
    TaskT* task
    )
{
    if (task->queueItem->m_data!=nullptr)
    {
        auto thread=reinterpret_cast<ThreadWithQueue<TaskT>*>(task->queueItem->m_data);
        thread->post(task);
    }
}

//---------------------------------------------------------------
template <typename TaskT>
TaskT* ThreadPoolWithQueuesTraits<TaskT>::prepare()
{
    auto thread=d->selectThread();
    auto task=thread->prepare();
    task->queueItem->m_data=thread;
    return task;
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadPoolWithQueuesTraits<TaskT>::setMaxTasksPerLoop(int count)
{
    d->maxHandlersPerLoop=count;
    for (auto&& it:d->threads)
    {
        it->setMaxTasksPerLoop(count);
    }
}

//---------------------------------------------------------------
template <typename TaskT>
int ThreadPoolWithQueuesTraits<TaskT>::maxTasksPerLoop() const
{
    return d->maxHandlersPerLoop;
}

//---------------------------------------------------------------
template <typename TaskT>
void ThreadPoolWithQueuesTraits<TaskT>::setQueue(Queue<TaskT>* queue)
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
template <typename TaskT>
bool ThreadPoolWithQueuesTraits<TaskT>::containsCurrentThread() const
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

/********************** ThreadPoolWithQueues **************************/

//---------------------------------------------------------------

template <typename TaskT>
ThreadPoolWithQueues<TaskT>::ThreadPoolWithQueues(
        size_t threadCount,
        const FixedByteArrayThrow16& id,
        Queue<TaskT>* queue
    )
{
    Assert(threadCount>0,"Thread count must be positive number");
    auto i=threadCount;
    while(i>0)
    {
        Queue<TaskT>* threadQueue=nullptr;
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
        auto thread=std::make_shared<ThreadWithQueue<TaskT>>(name.c_str(),threadQueue);

        //! @todo Is it needed?
        // thread->setThreadQ(this);

        this->traits().d->threadStorage.push_back(thread);
        this->traits().d->threads.insert(thread.get());
        --i;
    }
}

//---------------------------------------------------------------

template <typename TaskT>
ThreadPoolWithQueues<TaskT>::~ThreadPoolWithQueues()
{
    stop();
}

template <typename TaskT>
ThreadPoolWithQueues<TaskT>::ThreadPoolWithQueues(ThreadPoolWithQueues<TaskT>&&) noexcept=default;

template <typename TaskT>
ThreadPoolWithQueues<TaskT>& ThreadPoolWithQueues<TaskT>::operator=(ThreadPoolWithQueues<TaskT>&&) noexcept=default;

//---------------------------------------------------------------

template <typename TaskT>
void ThreadPoolWithQueues<TaskT>::start()
{
    for (auto&& it:this->traits().d->threads)
    {
        it->start();
    }
}

//---------------------------------------------------------------

template <typename TaskT>
void ThreadPoolWithQueues<TaskT>::stop()
{
    for (auto&& it:this->traits().d->threads)
    {
        it->stop();
    }
}

//---------------------------------------------------------------

template <typename TaskT>
size_t ThreadPoolWithQueues<TaskT>::threadCount() const
{
    return this->traits().d->threadStorage.size();
}

//---------------------------------------------------------------

template <typename TaskT>
ThreadWithQueue<TaskT>* ThreadPoolWithQueues<TaskT>::thread(size_t num)
{
    if (num>this->traits().d->threadStorage.size())
    {
        return nullptr;
    }
    return this->traits().d->threadStorage[num].get();
}

//---------------------------------------------------------------

template class HATN_COMMON_EXPORT ThreadPoolWithQueuesTraits<Task>;
template class HATN_COMMON_EXPORT ThreadPoolWithQueuesTraits<TaskWithContext>;
template class HATN_COMMON_EXPORT ThreadPoolWithQueues<Task>;
template class HATN_COMMON_EXPORT ThreadPoolWithQueues<TaskWithContext>;

template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadPoolWithQueues<Task>>;
template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadPoolWithQueues<TaskWithContext>>;

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
