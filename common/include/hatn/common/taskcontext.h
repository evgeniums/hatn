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

struct TaskContexTag{};
using TaskContextId=FixedByteArray20;
using TaskContextName=FixedByteArray32;

/**
 * @brief Main class for task contexts.
 *
 * A task context is a holder of task's state. A task context can be forwarded from one thread to another
 * and it lives until the task is finished.
 */
class HATN_COMMON_EXPORT TaskContext : public ManagedObject
{
    public:

        using hana_tag=TaskContexTag;

        using Clock = std::chrono::system_clock;
        using SteadyClock = std::chrono::steady_clock;

        /**
         * @brief Constructor.
         * @param tz Timezone to use for measuring task's times.
         */
        TaskContext(int8_t tz=DateTime::defaultTz())
                              : m_steadyStarted(nowSteady()),
                                m_tz(tz)
        {
            m_started=generateId(m_id);
            adjustTz(m_started);
        }

        /**
         * @brief Constructor.
         * @param id ID of task.
         * @param tz imezone to use for measuring task's times.
         */
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

        /**
         * @brief Adjust timezone of a timepoint.
         * @param tp Timepoint.
         * @return Timepoint with adjusted timezone.
         */
        std::chrono::time_point<Clock> adjustTz(const std::chrono::time_point<Clock>& tp) const
        {
            if (m_tz!=0)
            {
                return tp+std::chrono::hours(m_tz);
            }
            return tp;
        }

        /**
         * @brief Set task's timezone.
         * @param tz Timezone.
         */
        void setTz(int8_t tz) noexcept
        {
            m_tz=tz;
        }

        /**
         * @brief Get task's timezone.
         * @return Timezone.
         */
        int8_t tz() const noexcept
        {
            return m_tz;
        }

        /**
         * @brief Extract start time of a task from its ID.
         * @param id Task ID.
         * @return Start time of a task.
         */
        static Result<std::chrono::time_point<Clock>> extractStarted(const TaskContextId& id);

        /**
         * @brief Method to invoke just after the task enters a thread.
         */
        virtual void beforeThreadProcessing();

        /**
         * @brief Method to invoke just before the task leaves a thread.
         */
        virtual void afterThreadProcessing();

        /**
         * @brief Get ID of the task.
         * @return ID of the task.
         */
        const TaskContextId& id() const noexcept
        {
            return m_id;
        }

        /**
         * @brief Set ID of the task.
         * @param id New ID.
         */
        void setId(TaskContextId id) noexcept
        {
            m_id=std::move(id);
        }

        /**
         * @brief Generate  task ID.
         * @param id Where to put result.
         * @return Timepoint of generated ID.
         */
        static std::chrono::time_point<Clock> generateId(TaskContextId& id);

        /**
         * @brief Check if this task valid.
         * @return true if ID not empty.
         */
        bool isValid() const noexcept
        {
            return !m_id.isEmpty();
        }

        /**
         * @brief Get current timepoint in UTC.
         * @return Result.
         */
        static std::chrono::time_point<Clock> nowUtc() noexcept
        {
            return Clock::now();
        }

        /**
         * @brief Get current timepoint in timezone of the task.
         * @return Result.
         */
        std::chrono::time_point<Clock> now() const noexcept
        {
            return adjustTz(nowUtc());
        }

        /**
         * @brief Get current steady timepoint.
         * @return Result.
         */
        static std::chrono::time_point<SteadyClock> nowSteady() noexcept
        {
            return SteadyClock::now();
        }

        /**
         * @brief Get microseconds elapsed from the start of the task.
         * @return Result.
         */
        uint64_t elapsedMicroseconds() const noexcept
        {
            auto d=nowSteady()-m_steadyStarted;
            return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
        }

        /**
         * @brief Finish measuring time and return microseconds elapsed from the start of the task.
         * @return Result.
         */
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

        /**
         * @brief Finish measuring task's timing.
         */
        void finish() noexcept
        {
            m_steadyFinished=nowSteady();
        }

        /**
         * @brief Get time of task's start.
         * @return Result.
         */
        std::chrono::time_point<Clock> startedAt() const noexcept
        {
            return m_started;
        }

        /**
         * @brief Set task's name.
         * @param name New name of the task.
         */
        void setName(const lib::string_view& name)
        {
            m_name=name;
        }

        /**
         * @brief Get task's name.
         * @return Result.
         */
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

/**
 * @brief Base class for task subcontexts.
 *
 * Actual task contexts can consist of independent subcontexts where each subcontext is responsible
 * for some specific data or state or feature. A TaskSubcontext must be used as a base class for each
 * such subcontext.
 */
class TaskSubcontext
{
    public:

        /**
         * @brief Constructor.
         * @param taskContext Main task context.
         */
        TaskSubcontext(TaskContext* mainContext) : m_mainCtx(*mainContext)
        {}

        /**
         * @brief Get main task context.
         * @return Result.
         */
        const TaskContext& mainCtx() const noexcept
        {
            return m_mainCtx;
        }

        /**
         * @brief Get main task context.
         * @return Result.
         */
        TaskContext& mainCtx() noexcept
        {
            return m_mainCtx;
        }

    private:

        TaskContext& m_mainCtx;
};

/**
 * @brief Template prototype for wrappers of task subcontexts accessible via thread local singletons.
 *
 * When a task enters a thread and a TaskContext::beforeThreadProcessing() is called then all task subcontexts
 * of that task are registered as thread_local pointers to corresponding subcontexts.
 * Those subcontexts' pointers can be accessed using static calls such as ThreadSubcontext<SomeSubcontextType>::value(),
 * where SomeSubcontextType can correspond to any type of subcontext contained in the current task context.
 * Thus, subcontexts of the current task can be accesses globally at any point of code during task processing.
 * When the tasks leaves the thread a TaskContext::afterThreadProcessing() must be called releasing
 * corresponding ThreadSubcontext, i.e. ThreadSubcontext<SomeSubcontextType>::value() becomes nullptr in that thread
 * for each task's subcontext unless other task is registered for processing in that thread.
 *
 * Use HATN_TASK_CONTEXT_DECLARE() and HATN_TASK_CONTEXT_DEFINE() macros to declare and instantiate
 * spicializations of the TLSubCtxWrapper with types derived from TaskSubcontext.
 */
template <typename T>
class ThreadSubcontext
{
    // Implement below methods in class specialization.
    // Macros HATN_TASK_CONTEXT_DECLARE() and HATN_TASK_CONTEXT_DEFINE() already do the work
    // and can be used instead of manual specialization.

    // static T* value() noexcept;
    // static void setValue(T* val) noexcept;
    // static void reset() noexcept;
};

namespace detail {

template <typename Subcontexts>
struct SubcontextRefs
{
    constexpr static auto toRefs()
    {
        tupleToTupleCType<Subcontexts> tc;
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
 * @brief Template for actual task contexts.
 *
 *  Actual task context is a main task context with a tuple of subcontexts.
 *  Actual task context must be derived either from TaskContext or from other ActualTaskContext.
 *
 *  To create an actual task context either makeTaskContext() or allocateTaskContext() helpers can be used.
 */
template <typename Subcontexts, typename BaseTaskContext=TaskContext>
class ActualTaskContext : public BaseTaskContext
{
    public:

        using selfT=ActualTaskContext<Subcontexts,BaseTaskContext>;

        /**
         * @brief Constructor.
         * @param args Arguments to forward to base class.
         */
        template <typename ...Args>
        ActualTaskContext(Args&& ...args):
            BaseTaskContext(std::forward<Args>(args)...),
            m_subcontexts(replicateThis(this)),
            m_refs(subctxRefs())
        {
        }

        /**
         * @brief Get interface of base class.
         * @return Result.
         */
        BaseTaskContext* baseTaskContext() noexcept
        {
            return static_cast<BaseTaskContext*>(this);
        }

        /**
         * @brief Method to invoke just after the task enters a thread.
         */
        void beforeThreadProcessing() override
        {
            BaseTaskContext::beforeThreadProcessing();

            boost::hana::for_each(
                m_subcontexts,
                [](auto& subcontext)
                {
                    using type=typename std::decay_t<decltype(subcontext)>;
                    ThreadSubcontext<type>::setValue(&subcontext);
                }
            );
        }

        /**
         * @brief Method to invoke just before the task leaves a thread.
         */
        void afterThreadProcessing() override
        {
            BaseTaskContext::afterThreadProcessing();

            boost::hana::for_each(
                m_subcontexts,
                [](const auto& subcontext)
                {
                    using type=typename std::decay_t<decltype(subcontext)>;
                    ThreadSubcontext<type>::setValue(nullptr);
                }
            );
        }

        /**
         * @brief Get tuple of subcontexts.
         * @return Const reference to subcontexts.
         */
        const Subcontexts& subcontexts() const noexcept
        {
            return m_subcontexts;
        }

        /**
         * @brief Get tuple of subcontexts.
         * @return Reference to subcontexts.
         */
        Subcontexts& subcontexts() noexcept
        {
            return m_subcontexts;
        }

        /**
         * @brief Get subcontext by type.
         * @return Const reference to subcontext.
         */
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
            auto b=hana::type_c<BaseTaskContext>;

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

        /**
         * @brief Get subcontext by type.
         * @return Reference to subcontext.
         */
        template <typename T>
        T& get()
        {
            auto constSelf=const_cast<const selfT*>(this);
            return const_cast<T&>(constSelf->template get<T>());
        }

    private:

        static auto replicateThis(TaskContext* self)
        {
            return hana::replicate<hana::tuple_tag>(self,hana::size(tupleToTupleCType<Subcontexts>{}));
        }

        auto subctxRefs() const
        {
            return hana::transform(
                m_subcontexts,
                [](const auto& v)
                {
                    return std::cref(v);
                }
            );
        }

        Subcontexts m_subcontexts;
        typename detail::SubcontextRefs<Subcontexts>::type m_refs;
};

namespace detail {

template <typename ...Types>
struct ActualTaskContexTraits
{
    constexpr static auto typeFn()
    {
        auto xs=hana::tuple_t<Types...>;
        auto first=hana::front(xs);
        using firstT=typename std::decay_t<decltype(first)>::type;
        auto tc=hana::eval_if(
            hana::is_a<TaskContexTag,firstT>,
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
        using wrappersT=common::tupleCToTupleType<std::decay_t<decltype(wrappersC)>>;
        auto base=hana::second(tc);
        return hana::template_<ActualTaskContext>(hana::type_c<wrappersT>,base);
    }

    using tupleC=decltype(typeFn());
    using type=typename tupleC::type;
};

}

/**
 * @brief Helper to make actual task context.
 *
 * Each Type of Types... parameter pack represents a subcontext derived from TaskSubcontext.
 * The first type can also be hana::is_a(TaskContexTag), i.e. some other ActualTaskContext that will
 * be used as a base class for created ActualTaskContext.
 *
 * Arguments of a functor operator are forwarded to constructor of the base class.
 */
template <typename ...Types>
struct makeTaskContextT
{
    template <typename ...Args>
    auto operator()(Args&&... args) const
    {
        using type=typename detail::ActualTaskContexTraits<Types...>::type;
        return makeShared<type>(std::forward<Args>(args)...);
    }
};
template <typename ...Types>
constexpr makeTaskContextT<Types...> makeTaskContext{};

/**
 * @brief Helper to allocate actual task context.
 *
 * @see makeTaskContextT for more details. The first argument of dunctor operator is an allocator.
 */
template <typename ...Types>
struct allocateTaskContextT
{
    template <typename ...Args>
    auto operator()(const pmr::polymorphic_allocator<typename detail::ActualTaskContexTraits<Types...>::type>& allocator, Args&&... args) const
    {
        using type=typename detail::ActualTaskContexTraits<Types...>::type;
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
    class Export ThreadSubcontext<Type> \
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
        thread_local static Type* ThreadSubcontextValue{nullptr}; \
    } \
    HATN_COMMON_NAMESPACE_BEGIN \
    Type* ThreadSubcontext<Type>::value() noexcept \
    { \
            return ThreadSubcontextValue; \
    } \
    void ThreadSubcontext<Type>::setValue(Type* val) noexcept \
    { \
            ThreadSubcontextValue=val; \
    } \
    void ThreadSubcontext<Type>::reset() noexcept \
    { \
            ThreadSubcontextValue=nullptr; \
    } \
    template class ThreadSubcontext<Type>; \
    HATN_COMMON_NAMESPACE_END \
    HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_END

#endif // HATNTASKCONTEXT_H
