/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/threadq.h
  *
  *     Interface of threads queue.
  *
  */

/****************************************************************************/

#ifndef HATNTHREADQUEUEINTERFACE_H
#define HATNTHREADQUEUEINTERFACE_H

#include <functional>
#include <array>

#include <hatn/common/common.h>
#include <hatn/common/weakptr.h>
#include <hatn/common/managedobject.h>
#include <hatn/common/queue.h>
#include <hatn/common/threadcategoriespool.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/objecttraits.h>
#include <hatn/common/taskcontext.h>

HATN_COMMON_NAMESPACE_BEGIN

struct QueueItem;

//! Thread task
struct Task final
{
    constexpr static const bool CopyConstructible=true;

    //! Handler to invoke in thread
    std::function<void()> handler;

    //! Ctor
    Task(
        std::function<void()> handler=std::function<void()>() //!< Handler to invoke
    ) noexcept : handler(std::move(handler)),queueItem(nullptr)
    {}

    //! Function operator
    inline void operator() () noexcept
    {
        handler();
    }

    QueueItem* queueItem;
};

//! Thread task with context
/**
 * @brief Thread can check if context exists and invoke task only when it exists
 */
struct TaskWithContext final
{
    constexpr static const bool CopyConstructible=true;
    using HandlerT=std::function<void(const SharedPtr<TaskContext>&)>;

    //! Ctor
    TaskWithContext(
        HandlerT handler=HandlerT{}, //!< Handler to invoke
        SharedPtr<TaskContext> context=SharedPtr<TaskContext>{} //!< Context to invoke with
    ) noexcept: handler(std::move(handler)),
                context(std::move(context)),
                checkGuard(false),
                queueItem(nullptr)
    {}

    inline void setGuard(SharedPtr<ManagedObject> g, bool alwaysCheck=true) noexcept
    {
        guard=g;
        checkGuard=alwaysCheck;
    }

    inline void setHandler(HandlerT h) noexcept
    {
        handler=std::move(h);
    }

    inline void setContext(SharedPtr<TaskContext> ctx) noexcept
    {
        context=std::move(ctx);
    }

    //! Function operator
    inline void operator() ()
    {
        if (checkGuard)
        {
            auto g=guard.lock();
            if (g.isNull())
            {
                return;
            }
        }

        if (!context.isNull())
        {
            context->beforeThreadProcessing();
            handler(context);
            context->afterThreadProcessing();
        }
        else
        {
            handler(context);
        }
    }

    private:

        //! Handler to invoke
        HandlerT handler;
        //! Context
        SharedPtr<TaskContext> context;
        //! Task guard
        WeakPtr<ManagedObject> guard;
        //! Check if guard exists before invoke
        bool checkGuard;

    public:

        QueueItem* queueItem;
};

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

/**
 *  @brief Thread task with type erasured context to use for contexts that are not copy-constructible.
 *
 * This is a workaround to avoid prolifiration of thread classes (with/without copy-constructible contexts).
 *
 * @attention Use it only by posting tasks with pointers to prepared tasks: first prepare() and then post(TaskTtem*).
 * Do not post it by value with postTask(TaskT).
 */
template <typename T>
struct TaskInlineContext final
{
    constexpr static const bool CopyConstructible=false;
    using HandlerT=std::function<void(const T&)>;

    //! Handler to invoke
    HandlerT handler;

    //! Ctor
    TaskInlineContext() noexcept : queueItem(nullptr),hasContext(false)
    {}

    //! Dtor
    ~TaskInlineContext()
    {
        if (hasContext)
        {
            lib::destroyAt(obj());
        }
    }

    // constructors and assignment operators below are intentionally not deleted
    // CopyConstructible must be used to detect that object is not copy constructible

    TaskInlineContext(const TaskInlineContext&)=default;
    TaskInlineContext(TaskInlineContext&&) =default;
    TaskInlineContext& operator=(const TaskInlineContext&)=default;
    TaskInlineContext& operator=(TaskInlineContext&&) =default;

    //! Context object
    inline T* obj() noexcept
    {
        return reinterpret_cast<T*>(&context);
    }

    //! Create object
    template <typename ...Args> inline
    T* createObj(Args&&... args)
    {
        ::new (&context) T(std::forward<Args>(args)...);
        hasContext=true;
        return obj();
    }

    //! Function operator
    inline void operator() ()
    {
        handler(*(obj()));
    }

    QueueItem* queueItem;

    private:

        std::aligned_storage_t<sizeof(T),alignof(T)> context;
        bool hasContext=false;
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

template <typename TaskT>
class ThreadQueueTraitsSample
{
    //! Post task
    void postTask(
            TaskT task
        )
    {
        std::ignore=task;
    }

    //! Post prepared task
    void post(
            TaskT* task
        )
    {
        std::ignore=task;
    }

    /**
         * @brief Set new queue
         * @param queue
         *
         * Not thread safe, call it only in setup routines before running thread
         */
    void setQueue(Queue<TaskT>* queue)
    {
        std::ignore=queue;
    }

    /**
         * @brief Prepare queue item and create task object
         * @return Prepared task object that can be filled in the caller and then pushed back to the queue
         */
    TaskT* prepare()
    {
        return nullptr;
    }

    /**
         * @brief Set max number of tasks per sinle io_context loop (default 64)
         * @param count
         *
         * Set this parameter before starting thread, though it is not critical to set it in runtime but take into account that parameter isn't atomic.
         */
    void setMaxTasksPerLoop(int count)
    {
        std::ignore=count;
    }

    //! Get max number of tasks per single io_context loop
    int maxTasksPerLoop() const
    {
        return 1;
    }

    //! Check if interface includes current thread
    bool containsCurrentThread() const
    {
        return false;
    }
};

//! Interface of thread with queue
template <typename TaskT, template <typename> class Traits>
class ThreadQ : public WithTraits<Traits<TaskT>>
{
    public:

        using base=WithTraits<Traits<TaskT>>;
        using selfType=ThreadQ<TaskT,Traits>;

        //! Ctor
        template <typename ... Args>
        ThreadQ(Args&& ...traitsArgs) noexcept : base(std::forward<Args>(traitsArgs)...)
        {}

        //! Post task
        template <typename T>
        void postTask(
            T&& task
        )
        {
            static_assert(std::decay_t<TaskT>::CopyConstructible,"Can not post task that is not copy constructible");
            this->traits().postTask(std::forward<T>(task));
        }

        //! Post prepared task
        void post(
            TaskT* task
        )
        {
            this->traits().post(task);
        }

        /**
         * @brief Set new queue
         * @param queue
         *
         * Not thread safe, call it only in setup routines before running thread
         */
        void setQueue(Queue<TaskT>* queue)
        {
            this->traits().setQueue(queue);
        }

        /**
         * @brief Prepare queue item and create task object
         * @return Prepared task object that can be filled in the caller and then pushed back to the queue
         */
        TaskT* prepare()
        {
            return this->traits().prepare();
        }

        /**
         * @brief Set max number of tasks per sinle io_context loop (default 64)
         * @param count
         *
         * Set this parameter before starting thread, though it is not critical to set it in runtime but take into account that parameter isn't atomic.
         */
        void setMaxTasksPerLoop(int count)
        {
            this->traits().setMaxTasksPerLoop(count);
        }

        //! Get max number of tasks per single io_context loop
        int maxTasksPerLoop() const
        {
            return this->traits().maxTasksPerLoop();
        }

        //! Check if interface includes current thread
        bool containsCurrentThread() const
        {
            return this->traits().containsCurrentThread();
        }

        //! Get current thread interface
        static selfType* current() noexcept;

    protected:

        //! Set current thread interface with thread locality
        static void setCurrent(selfType* iface);
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNTHREADQUEUEINTERFACE_H
