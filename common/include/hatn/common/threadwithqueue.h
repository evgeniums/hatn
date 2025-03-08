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
#include <hatn/common/random.h>

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

#ifdef __MINGW32__

template class HATN_COMMON_EXPORT Queue<Task>;
template class HATN_COMMON_EXPORT Queue<TaskWithContext>;

template class HATN_COMMON_EXPORT ThreadWithQueueTraits<Task>;
template class HATN_COMMON_EXPORT ThreadWithQueueTraits<TaskWithContext>;

template class HATN_COMMON_EXPORT ThreadQ<Task,ThreadWithQueueTraits>;
template class HATN_COMMON_EXPORT ThreadQ<TaskWithContext,ThreadWithQueueTraits>;

template class HATN_COMMON_EXPORT ThreadWithQueue<Task>;
template class HATN_COMMON_EXPORT ThreadWithQueue<TaskWithContext>;

template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadWithQueue<Task>>;
template class HATN_COMMON_EXPORT ThreadCategoriesPool<ThreadWithQueue<TaskWithContext>>;

#endif

using TaskQueue=Queue<Task>;
using TaskWithContextQueue=Queue<TaskWithContext>;
using TaskThread=ThreadWithQueue<Task>;
using TaskWithContextThread=ThreadWithQueue<TaskWithContext>;
using ThreadQWithTaskContext=std::pointer_traits<decltype(TaskWithContextThread::current())>::element_type;

struct postAsyncTaskT
{
    template <typename HandlerT>
    void operator ()(ThreadQWithTaskContext* thread,
                    SharedPtr<TaskContext> ctx,
                    HandlerT handler
                    ) const
    {
        auto originThreadQ=TaskWithContextThread::current();

        auto threadHandler=[originThreadQ,handler{std::move(handler)}](const SharedPtr<TaskContext>& ctx)
        {
            auto cb=handler(ctx);
            auto cbTask=originThreadQ->prepare();
            cbTask->setHandler(std::move(cb));
            cbTask->setContext(ctx);
            originThreadQ->post(cbTask);
        };

        auto task=thread->prepare();
        task->setHandler(std::move(threadHandler));
        task->setContext(std::move(ctx));
        thread->post(task);
    }

    //! @todo Implement operator with guard
};
constexpr postAsyncTaskT postAsyncTask{};

template <typename CallbackT>
struct postAsyncCallback
{
    template <typename ContextT, typename ...Args>
    void operator()(SharedPtr<ContextT> ctx, Args&&... args) const
    {
        auto ts=hana::make_tuple(ctx,std::forward<Args>(args)...);
        postAsyncTask(
            thread,
            ctx,
            [ts{std::move(ts)},cb{callback}](const common::SharedPtr<common::TaskContext>&)
            {
                hana::unpack(std::move(ts),std::move(cb));
            }
        );
    }

    postAsyncCallback(
            CallbackT callback,
            ThreadQWithTaskContext* thread=ThreadQWithTaskContext::current()
        ) : callback(std::move(callback)),thread(thread)
    {}

    CallbackT callback;
    ThreadQWithTaskContext* thread;
};

struct makePostAsynCallbackT
{
    template <typename CallbackT>
    auto operator()(
            CallbackT callback,
            ThreadQWithTaskContext* thread=ThreadQWithTaskContext::current()
        ) const
    {
        return postAsyncCallback<CallbackT>{std::move(callback),thread};
    }
};
constexpr makePostAsynCallbackT makePostAsynCallback{};

enum class MappedThreadMode : uint8_t
{
    Caller,
    Mapped,
    Default
};

class MappedThreadQWithTaskContext
{
    public:

        MappedThreadQWithTaskContext(
                MappedThreadMode mode=MappedThreadMode::Caller,
                ThreadQWithTaskContext* defaultThread=ThreadQWithTaskContext::current()
            )
            :   m_threadMode(mode),
                m_defaultThread(defaultThread)
        {}

        void setThreadMode(MappedThreadMode threadMode) noexcept
        {
            m_threadMode=threadMode;
        }

        MappedThreadMode threadMode() const noexcept
        {
            return m_threadMode;
        }

        void setMappedThreads(std::vector<ThreadQWithTaskContext*> threads) noexcept
        {
            m_threads=std::move(threads);
        }

        auto mappedThreads() const
        {
            return m_threads;
        }

        void setDefaultThread(ThreadQWithTaskContext* defaultThread) noexcept
        {
            m_defaultThread=defaultThread;
        }

        ThreadQWithTaskContext* defaultThread() const noexcept
        {
            if (m_defaultThread==nullptr)
            {
                if (!m_threads.empty())
                {
                    return m_threads[0];
                }
                return callerThread();
            }
            return m_defaultThread;
        }

        static ThreadQWithTaskContext* callerThread()
        {
            return ThreadQWithTaskContext::current();
        }

        template <typename T>
        ThreadQWithTaskContext* mappedThread(const T& key) const noexcept
        {
            if (m_threads.empty())
            {
                return defaultThread();
            }

            auto hash=std::hash<T>(key);
            auto idx=hash%m_threads.size();
            return m_threads.at(idx);
        }

        ThreadQWithTaskContext* mappedThread(size_t idx) const noexcept
        {
            if (m_threads.empty())
            {
                return defaultThread();
            }
            idx=idx%m_threads.size();
            return m_threads.at(idx);
        }

        ThreadQWithTaskContext* randomThread() const noexcept
        {
            if (m_threads.empty())
            {
                return defaultThread();
            }
            size_t idx=Random::generate(m_threads.size());
            return mappedThread(idx);
        }

        template <typename T>
        auto thread(const T& key) const noexcept
        {
            switch (m_threadMode)
            {
                case(MappedThreadMode::Caller): return callerThread(); break;
                case(MappedThreadMode::Default): return defaultThread(); break;
                case(MappedThreadMode::Mapped): return mappedThread(key); break;
            }
            return m_defaultThread;
        }

        auto thread() const noexcept
        {
            switch (m_threadMode)
            {
                case(MappedThreadMode::Caller): return callerThread(); break;
                case(MappedThreadMode::Default): return defaultThread(); break;
                case(MappedThreadMode::Mapped):
                    if (!m_threads.empty())
                    {
                        return m_threads[0];
                    }
                    return defaultThread();
                break;
            }
            return m_defaultThread;
        }

    private:

        MappedThreadMode m_threadMode;
        ThreadQWithTaskContext* m_defaultThread;
        std::vector<ThreadQWithTaskContext*> m_threads;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNTHREADWITHQUEUE_H
