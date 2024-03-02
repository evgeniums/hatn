/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/threadqueueinterface.h
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
    using HandlerT=std::function<void(const SharedPtr<ManagedObject>&)>;

    //! Handler to invoke
    HandlerT handler;
    //! Context
    WeakPtr<ManagedObject> context;
    //! Check if context exists before invoke
    bool checkContext;

    //! Ctor
    TaskWithContext(
        HandlerT handler=HandlerT(), //!< Handler to invoke
        WeakPtr<ManagedObject> context=WeakPtr<ManagedObject>(), //!< Context to invoke with
        bool checkContext=true //!< Check if context exists before invoke
    ) noexcept: handler(std::move(handler)),context(std::move(context)),checkContext(checkContext),queueItem(nullptr)
    {}

    //! Function operator
    inline void operator() ()
    {
        auto ctx=context.lock();
        if (!ctx.isNull()||!checkContext)
        {
            handler(ctx);
        }
    }

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
 * \attention Use it only by posting tasks with pointers to prepared tasks: first prepare() and then post(TaskTypetem*).
 * Do not post it by value with postTask(TaskType).
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

//! Interface of thread with queue
template <typename TaskType>
class ThreadQueueInterface
{
    public:

        //! Constructor
        ThreadQueueInterface() = default;

        virtual ~ThreadQueueInterface()=default;
        ThreadQueueInterface(const ThreadQueueInterface&)=default;
        ThreadQueueInterface(ThreadQueueInterface&&) =default;
        ThreadQueueInterface& operator=(const ThreadQueueInterface&)=default;
        ThreadQueueInterface& operator=(ThreadQueueInterface&&) =default;

        //! Post task
        void postTask(
            TaskType task
        )
        {
            Assert(TaskType::CopyConstructible,"Can not post task that is not copy constructible");
            doPostTask(std::move(task));
        }

        //! Post prepared task
        virtual void post(
            TaskType* task
        ) = 0;

        /**
         * @brief Set new queue
         * @param queue
         *
         * Not thread safe, call it only in setup routines before running thread
         */
        virtual void setQueue(Queue<TaskType>* queue) = 0;

        /**
         * @brief Prepare queue item and create task object
         * @return Prepared task object that can be filled in the caller and then pushed back to the queue
         */
        virtual TaskType* prepare() = 0;

        /**
         * @brief Set max number of tasks per sinle io_context loop (default 64)
         * @param count
         *
         * Set this parameter before starting thread, though it is not critical to set it in runtime but take into account that parameter isn't atomic.
         */
        virtual void setMaxTasksPerLoop(int count) =0;

        //! Get max number of tasks per single io_context loop
        virtual int maxTasksPerLoop() const=0;

        //! Check if interface includes current thread
        virtual bool containsCurrentThread() const=0;

        //! Get current thread interface
        static ThreadQueueInterface<TaskType>* currentThreadInterface() noexcept;

    protected:

        virtual void doPostTask(
            TaskType task
        ) =0;

        //! Set current thread interface with thread locality
        static void setCurrentThreadInterface(ThreadQueueInterface<TaskType>* interface);
};

using TaskThreadInterface=ThreadQueueInterface<Task>;
using TaskWithContextThreadInterface=ThreadQueueInterface<TaskWithContext>;

using TaskThreadCategoriesPool=ThreadCategoriesPool<TaskThreadInterface>;
using TaskWithContextThreadCategoriesPool=ThreadCategoriesPool<TaskWithContextThreadInterface>;

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNTHREADQUEUEINTERFACE_H
