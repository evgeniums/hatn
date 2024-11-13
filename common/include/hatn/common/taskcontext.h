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

#if __cplusplus >= 201703L
#include <charconv>
#endif

#include <chrono>

#include <boost/hana.hpp>

namespace hana=boost::hana;

#include <hatn/common/common.h>
#include <hatn/common/managedobject.h>
#include <hatn/common/meta/decaytuple.h>
#include <hatn/common/meta/tupletypec.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/fixedbytearray.h>
#include <hatn/common/databuf.h>
#include <hatn/common/datetime.h>

HATN_COMMON_NAMESPACE_BEGIN

struct TaskContextTag{};
using TaskContextId=FixedByteArray20;
using TaskContextName=FixedByteArray32;

class HATN_COMMON_EXPORT TaskContext : public ManagedObject
{
    public:

        using Clock = std::chrono::system_clock;
        using SteadyClock = std::chrono::steady_clock;

        TaskContext(int8_t tz=DateTime::defaultTz())
                              : m_steadyStarted(nowSteady()),
                                m_tz(tz)
        {
            m_started=generateId(m_id);
            adjustTz(m_started);
        }

        TaskContext(TaskContextId id, int8_t tz=DateTime::defaultTz())
                                : m_id(std::move(id)),
                                  m_steadyStarted(nowSteady()),
                                  m_tz(tz)
        {
            auto msSinceEpoch=extractStarted(m_id);
            if (!msSinceEpoch)
            {
                m_started=msSinceEpoch.value();
            }
            else
            {
                m_started=nowUtc();
            }
            adjustTz(m_started);
        }

        std::chrono::time_point<Clock> adjustTz(const std::chrono::time_point<Clock>& tp) const
        {
            if (m_tz!=0)
            {
                return tp+std::chrono::hours(m_tz);
            }
            return tp;
        }

        void setTz(int8_t tz) noexcept
        {
            m_tz=tz;
        }

        int8_t tz() const noexcept
        {
            return m_tz;
        }

        static Result<std::chrono::time_point<Clock>> extractStarted(const TaskContextId& id);

        using hana_tag=TaskContextTag;

        virtual void beforeThreadProcessing();

        virtual void afterThreadProcessing();

        const TaskContextId& id() const noexcept
        {
            return m_id;
        }

        void setId(TaskContextId id) noexcept
        {
            m_id=std::move(id);
        }

        static std::chrono::time_point<Clock> generateId(TaskContextId& id);

        bool isValid() const noexcept
        {
            return !m_id.isEmpty();
        }

        static std::chrono::time_point<Clock> nowUtc() noexcept
        {
            return Clock::now();
        }

        std::chrono::time_point<Clock> now() const noexcept
        {
            return adjustTz(nowUtc());
        }

        static std::chrono::time_point<SteadyClock> nowSteady() noexcept
        {
            return SteadyClock::now();
        }

        uint64_t elapsedMicroseconds() const noexcept
        {
            auto d=nowSteady()-m_steadyStarted;
            return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
        }

        uint64_t finishMicroseconds() const noexcept
        {
            if (m_steadyFinished.time_since_epoch().count()==0)
            {
                auto self=const_cast<TaskContext*>(this);
                self->finish();
            }
            auto d=m_steadyFinished-m_steadyStarted;
            return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
        }

        void finish() noexcept
        {
            m_steadyFinished=nowSteady();
        }

        std::chrono::time_point<Clock> startedAt() const noexcept
        {
            return m_started;
        }

        void setName(const lib::string_view& name)
        {
            m_name=name;
        }

        const TaskContextName& name() const
        {
            return m_name;
        }

    private:

        TaskContextName m_name;
        TaskContextId m_id;

        std::chrono::time_point<Clock> m_started;

        std::chrono::time_point<SteadyClock> m_steadyStarted;
        std::chrono::time_point<SteadyClock> m_steadyFinished;

        int8_t m_tz;
};

template <typename T>
class TaskContextWrapper
{
    public:

        using type=T;

        TaskContextWrapper(TaskContext* taskContext) : m_value(taskContext)
        {
        }

        const type* value() const noexcept
        {
            return &m_value;
        }

        type* value() noexcept
        {
            return &m_value;
        }

    private:

        type m_value;
};

class TaskContextValue
{
    public:

        TaskContextValue(TaskContext* taskContext) : m_taskCtx(taskContext)
        {}

        const TaskContext* taskCtx() const noexcept
        {
            return m_taskCtx;
        }

        TaskContext* taskCtx() noexcept
        {
            return m_taskCtx;
        }

    private:

        TaskContext* m_taskCtx;
};

template <typename T>
class ThreadLocalContext
{
    // Implement in class specialization.

    // static T* value() noexcept;
    // static void setValue(T* val) noexcept;
    // static void reset() noexcept;
};

namespace detail {

template <typename ContextWrappersT>
struct ContextWrapperRefs
{
    constexpr static auto toRefs()
    {
        tupleToTupleCType<ContextWrappersT> tc;
        auto rtc=hana::transform(
            tc,
            [](auto&& v)
            {
                using type=typename std::decay_t<decltype(v)>::type;
                return hana::type_c<std::reference_wrapper<const type>>;
            }
        );
        return hana::unpack(rtc,hana::template_<hana::tuple>);
    }

    using typeC=decltype(toRefs());
    using type=typename typeC::type;
};

}

/**
 * @brief The TaskContextT class.
 *
 * Currently can use only default-constructed context wrappers.
 */
template <typename ContextWrappersT, typename BaseTaskContextT=TaskContext>
class TaskContextT : public BaseTaskContextT
{
    public:

        using selfT=TaskContextT<ContextWrappersT,BaseTaskContextT>;

        static auto replicateThis(TaskContext* self)
        {
            return hana::replicate<hana::tuple_tag>(self,hana::size(tupleToTupleCType<ContextWrappersT>{}));
        }

        template <typename ...Args>
        TaskContextT(Args&& ...args):
            BaseTaskContextT(std::forward<Args>(args)...),
            m_wrappers(replicateThis(this)),
            m_refs(wrapperRefs())
        {
        }

        BaseTaskContextT* baseTaskContext() noexcept
        {
            return static_cast<BaseTaskContextT*>(this);
        }

        void beforeThreadProcessing() override
        {
            BaseTaskContextT::beforeThreadProcessing();

            boost::hana::for_each(
                m_wrappers,
                [](auto&& ContextWrapper)
                {
                    using type=typename std::decay_t<decltype(ContextWrapper)>::type;
                    ThreadLocalContext<type>::setValue(ContextWrapper.value());
                }
            );
        }

        void afterThreadProcessing() override
        {
            BaseTaskContextT::afterThreadProcessing();

            boost::hana::for_each(
                m_wrappers,
                [](auto&& ContextWrapper)
                {
                    using type=typename std::decay_t<decltype(ContextWrapper)>::type;
                    ThreadLocalContext<type>::setValue(nullptr);
                }
            );
        }

        const ContextWrappersT& wrappers() const noexcept
        {
            return m_wrappers;
        }

        ContextWrappersT& wrappers() noexcept
        {
            return m_wrappers;
        }

        template <typename T>
        const T& get() const
        {
            auto wrapper=hana::find_if(
                m_refs,
                [](auto&& v)
                {
                    using crefT=std::decay_t<decltype(v)>;
                    using type=std::decay_t<typename crefT::type>;
                    return std::is_same<T,type>{};
                }
            );

            auto t=hana::type_c<T>;
            auto b=hana::type_c<BaseTaskContextT>;

            return hana::eval_if(
                hana::equal(wrapper,hana::nothing),
                [&](auto _)
                {
                    using type=typename std::decay_t<decltype(_(t))>::type;
                    static_assert(!std::is_same<type,TaskContext>::value,"Unknown context wrapper type");

                    using base=typename std::decay_t<decltype(_(b))>::type;
                    return base::template get<type>();
                },
                [&](auto _)
                {
                    return _(wrapper).value();
                }
            );
        }

        template <typename T>
        T& get()
        {
            auto constSelf=const_cast<const selfT*>(this);
            return const_cast<T&>(constSelf->template get<T>());
        }

    private:

        auto wrapperRefs() const
        {
            return hana::transform(
                m_wrappers,
                [](const auto& v)
                {
                    return std::cref(v);
                }
            );
        }

        ContextWrappersT m_wrappers;
        typename detail::ContextWrapperRefs<ContextWrappersT>::type m_refs;
};

namespace detail {

template <typename ...Types>
struct TaskContextTraits
{
    constexpr static auto typeFn()
    {
        auto xs=hana::tuple_t<Types...>;
        auto first=hana::front(xs);
        using firstT=typename std::decay_t<decltype(first)>::type;
        auto tc=hana::eval_if(
            hana::is_a<TaskContextTag,firstT>,
            [&](auto _)
            {
                return hana::make_pair(hana::drop_front(_(xs)),_(first));
            },
            [&](auto _)
            {
                return hana::make_pair(_(xs),hana::type_c<TaskContext>);
            }
        );
        auto wrappersC=hana::first(tc);
        using wrappersT=common::tupleCToTupleType<std::decay_t<decltype(wrappersC)>>; // std::decay_t<decltype(tupleCToTuple(wrappersC))>;
        auto base=hana::second(tc);
        return hana::template_<TaskContextT>(hana::type_c<wrappersT>,base);
    }

    using tupleC=decltype(typeFn());
    using type=typename tupleC::type;
};

}

template <typename ...Types>
struct makeTaskContextT
{
    template <typename ...Args>
    auto operator()(Args&&... args) const
    {
        using type=typename detail::TaskContextTraits<Types...>::type;
        return makeShared<type>(std::forward<Args>(args)...);
    }
};
template <typename ...Types>
constexpr makeTaskContextT<Types...> makeTaskContext{};

template <typename ...Types>
struct allocateTaskContextT
{
    template <typename ...Args>
    auto operator()(const pmr::polymorphic_allocator<typename detail::TaskContextTraits<Types...>::type>& allocator, Args&&... args) const
    {
        using type=typename detail::TaskContextTraits<Types...>::type;
        return allocateShared<type>(allocator,std::forward<Args>(args)...);
    }
};
template <typename ...Types>
constexpr allocateTaskContextT<Types...> allocateTaskContext{};

HATN_COMMON_NAMESPACE_END

#define HATN_TASK_CONTEXT_DECLARE(Type,Export) \
    HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_BEGIN \
    HATN_COMMON_NAMESPACE_BEGIN \
    template <> \
    class Export ThreadLocalContext<Type> \
    { \
        public: \
            static Type* value() noexcept; \
            static void setValue(Type* val) noexcept; \
            static void reset() noexcept; \
    }; \
    HATN_COMMON_NAMESPACE_END \
    HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_END

#define HATN_TASK_CONTEXT_DEFINE(Type) \
    HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_BEGIN \
    namespace { \
        thread_local static Type* ThreadLocalContextValue{nullptr}; \
    } \
    HATN_COMMON_NAMESPACE_BEGIN \
    Type* ThreadLocalContext<Type>::value() noexcept \
    { \
            return ThreadLocalContextValue; \
    } \
    void ThreadLocalContext<Type>::setValue(Type* val) noexcept \
    { \
            ThreadLocalContextValue=val; \
    } \
    void ThreadLocalContext<Type>::reset() noexcept \
    { \
            ThreadLocalContextValue=nullptr; \
    } \
    template class ThreadLocalContext<Type>; \
    HATN_COMMON_NAMESPACE_END \
    HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_END

#endif // HATNTASKCONTEXT_H
