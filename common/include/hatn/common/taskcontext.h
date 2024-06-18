/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/taskcontext.h
  *
  *     Task context.
  *
  */

/****************************************************************************/

#ifndef HATNTASKCONTEXT_H
#define HATNTASKCONTEXT_H

#include <boost/hana.hpp>

#include <hatn/common/common.h>
#include <hatn/common/managedobject.h>

HATN_COMMON_NAMESPACE_BEGIN

class HATN_COMMON_EXPORT TaskContext : public ManagedObject
{
    public:

        virtual void beforeThreadProcessing();

        virtual void afterThreadProcessing();
};

template <typename T>
class ThreadLocalContext
{
};

template <typename T, typename Traits>
class TaskContextContainer
{
    public:

        using type=T;

        T* taskContext() const noexcept
        {
            return Traits::context(this);
        }
};

template <typename ContextContainers, typename BaseTaskContextT=TaskContext>
class TaskContextT : public BaseTaskContextT
{
    public:

        BaseTaskContextT* baseTaskContext() noexcept
        {
            return static_cast<BaseTaskContextT*>(this);
        }

        void beforeThreadProcessing() override
        {
            BaseTaskContextT::beforeThreadProcessing();

            boost::hana::for_each(
                m_contextContainers,
                [](auto&& contextContainer)
                {
                    using type=typename std::decay_t<decltype(contextContainer)>::type;
                    ThreadLocalContext<type>::setValue(contextContainer.taskContext());
                }
            );
        }

        void afterThreadProcessing() override
        {
            BaseTaskContextT::afterThreadProcessing();

            boost::hana::for_each(
                m_contextContainers,
                [](auto&& contextContainer)
                {
                    using type=typename std::decay_t<decltype(contextContainer)>::type;
                    ThreadLocalContext<type>::setValue(nullptr);
                }
            );
        }

        template <typename Ts>
        void setContextContainers(Ts&& contextContainers)
        {
            m_contextContainers=std::forward<Ts>(contextContainers);
        }

    private:

        ContextContainers m_contextContainers;
};

HATN_COMMON_NAMESPACE_END

#define HATN_TASK_CONTEXT_DECLARE(Type,Export) \
    HATN_COMMON_NAMESPACE_BEGIN \
    template <> \
    class Export ThreadLocalContext<Type> \
    { \
        public: \
            static Type* value() noexcept; \
            static setValue(T* val) noexcept; \
    }; \
    HATN_COMMON_NAMESPACE_END

#define HATN_TASK_CONTEXT_DEFINE(Type,Export) \
    namespace { \
        thread_local static Type* ThreadLocalContextValue{nullptr}; \
    } \
    HATN_COMMON_NAMESPACE_BEGIN \
    T* ThreadLocalContext<Type>::value() noexcept \
    { \
        return ThreadLocalContextValue; \
    } \
    void ThreadLocalContext<Type>::setValue(T* val) noexcept \
    { \
        ThreadLocalContextValue=val; \
    } \
    template class Export ThreadLocalContext<Type>; \
    HATN_COMMON_NAMESPACE_END

#endif // HATNTASKCONTEXT_H
