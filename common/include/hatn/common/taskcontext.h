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
#include <boost/hana/ext/std/tuple.hpp>

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
class HATN_COMMON_EXPORT TaskContext : public EnableSharedFromThis<TaskContext>
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
         * @param id ID of task
         * @param tz imezone to use for measuring task's times.
         *
         */
        TaskContext(const lib::string_view& id, int8_t tz=DateTime::defaultTz())
                                : m_id(id),
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
         * @brief Constructor form tuple.
         * @param ts Tuple with arguments to passed to other constructors.
         */
        template <template <typename ...> class Ts, typename ...Types>
        TaskContext(
                Ts<Types...>&& ts
            ) : TaskContext(
                std::forward<Ts<Types...>>(ts),
                std::make_index_sequence<std::tuple_size<std::remove_reference_t<Ts<Types...>>>::value>{}
            )
        {}

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
        static Result<std::chrono::time_point<Clock>> extractStarted(const lib::string_view& id);

        /**
         * @brief Method to invoke just after the task enters a thread.
         */
        virtual void beforeThreadProcessing()
        {
            beforeThreadProcessingNV();
        }

        /**
         * @brief Method to invoke just before the task leaves a thread.
         */
        virtual void afterThreadProcessing()
        {
            afterThreadProcessingNV();
        }

        /**
         * @brief Non-virtual vertion of beforeThreadProcessing
         */
        void beforeThreadProcessingNV()
        {}

        /**
         * @brief Non-virtual vertion of afterThreadProcessing
         */
        void afterThreadProcessingNV()
        {}

        /**
         * @brief Method to invoke in the end of async handler.
         */
        void onAsyncHandlerExit()
        {
            afterThreadProcessing();
        }

        /**
         * @brief Method to invoke just after the task enters async handler.
         */
        void onAsyncHandlerEnter()
        {
            beforeThreadProcessing();
        }

        /**
         * @brief Begin task context.
         */
        void beginTaskContext()
        {
            beforeThreadProcessing();
        }

        /**
         * @brief End task context.
         */
        void endTaskContext()
        {
            afterThreadProcessing();
        }

        /**
         * @brief Get ID of the task.
         * @return ID of the task.
         */
        lib::string_view id() const noexcept
        {
            return m_id;
        }

        /**
         * @brief Set ID of the task.
         * @param id New ID.
         */
        void setId(const lib::string_view& id) noexcept
        {
            m_id=id;
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

        template <typename Ts, std::size_t... I>
        TaskContext(
            Ts&& ts,
            std::index_sequence<I...>
            ) : TaskContext(std::get<I>(std::forward<Ts>(ts))...)
        {}

        TaskContextName m_name;
        TaskContextId m_id;

        std::chrono::time_point<Clock> m_started;

        std::chrono::time_point<SteadyClock> m_steadyStarted;
        std::chrono::time_point<SteadyClock> m_steadyFinished;

        int8_t m_tz;
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
        TaskSubcontext(TaskContext* mainContext=nullptr) : m_mainCtx(mainContext)
        {}

        /**
         * @brief Get main task context.
         * @return Result.
         */
        const TaskContext& mainCtx() const noexcept
        {
            return *m_mainCtx;
        }

        /**
         * @brief Get main task context.
         * @return Result.
         */
        TaskContext& mainCtx() noexcept
        {
            return *m_mainCtx;
        }

        void setMainCtx(TaskContext* mainContext)
        {
            m_mainCtx=mainContext;
        }

        template <typename T>
        static auto hasAsyncHandlerFns(T arg)
        {
            return hana::is_valid([](auto v) -> decltype((void)hana::traits::declval(v).onAsyncHandlerEnter()){})(arg);
        }

    private:

        TaskContext *m_mainCtx;
};

using TaskContextShared=SharedPtr<TaskContext>;

struct TaskSubcontextTag{};

/**
 * @brief Helper to wrap arbitrary type to task subcontext.
 */
template <typename T, typename =hana::when<true>>
class TaskSubcontextT : public TaskSubcontext, public T
{
    public:

        using hana_tag=TaskSubcontextTag;

        template <typename ...Args>
        TaskSubcontextT(
            TaskContext* mainContext,
            Args&& ...args
            ) : TaskSubcontext(mainContext),
            T(std::forward<Args>(args)...)
        {}

        template <typename ...Args>
        TaskSubcontextT(
            Args&& ...args
            ) : T(std::forward<Args>(args)...)
        {}

        template <template <typename ...> class Ts, typename ...Types>
        TaskSubcontextT(
            Ts<Types...>&& ts
            ) : TaskSubcontextT(
                std::forward<Ts<Types...>>(ts),
                std::make_index_sequence<std::tuple_size<std::remove_reference_t<Ts<Types...>>>::value>{}
                )
        {}

    private:

        template <typename Ts, std::size_t... I>
        TaskSubcontextT(
            Ts&& ts,
            std::index_sequence<I...>
            ) : TaskSubcontextT(std::get<I>(std::forward<Ts>(ts))...)
        {}
};

/**
 * @brief Helper to wrap arbitrary type to task subcontext.
 */
template <typename T>
class TaskSubcontextT<T,hana::when<std::is_base_of<TaskSubcontext,T>::value>> : public T
{
    public:

        using hana_tag=TaskSubcontextTag;

        template <typename ...Args>
        TaskSubcontextT(
            TaskContext* mainContext,
            Args&& ...args
            ) : T(mainContext,std::forward<Args>(args)...)
        {}

        template <typename ...Args>
        TaskSubcontextT(
            Args&& ...args
            ) : T(std::forward<Args>(args)...)
        {}

        template <template <typename ...> class Ts, typename ...Types>
        TaskSubcontextT(
            Ts<Types...>&& ts
            ) : TaskSubcontextT(
                std::forward<Ts<Types...>>(ts),
                std::make_index_sequence<std::tuple_size<std::remove_reference_t<Ts<Types...>>>::value>{}
                )
        {}

    private:

        template <typename Ts, std::size_t... I>
        TaskSubcontextT(
            Ts&& ts,
            std::index_sequence<I...>
            ) : TaskSubcontextT(std::get<I>(std::forward<Ts>(ts))...)
        {}
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

/**
 * @brief Template for actual task contexts.
 *
 *  Actual task context is a main task context containing a tuple of subcontexts.
 *  Actual task context must be derived either from TaskContext or from other ActualTaskContext.
 *
 *  To create an actual task context either makeTaskContext() or allocateTaskContext() helpers can be used.
 */
template <typename Subcontexts, typename BaseTaskContext=TaskContext>
class ActualTaskContext : public BaseTaskContext
{
    public:

        using selfT=ActualTaskContext<Subcontexts,BaseTaskContext>;
        constexpr static auto isNestedTaskContext=hana::not_(std::is_same<TaskContext,BaseTaskContext>{});

        /**
         * @brief Default constructor.
         */
        ActualTaskContext()
            : m_subcontexts(),
              m_refs(subctxRefs())
        {
            hana::for_each(
                m_subcontexts,
                [this](auto&& subCtx)
                {
                    subCtx.setMainCtx(this);
                }
            );
        }

        /**
         * @brief Constructor from tuples of tuples.
         * @param tts Tuple of tuples to forward to constructors of subcontexts of this class.
         * @param baseTs Arguments to forward to base class.
         */
        template <typename Tts, typename ...BaseTs>
        ActualTaskContext(Tts&& tts, BaseTs&& ...baseTs):
            BaseTaskContext(std::forward<BaseTs>(baseTs)...),
            m_subcontexts(std::forward<Tts>(tts)),
            m_refs(subctxRefs())
        {
            hana::for_each(
                m_subcontexts,
                [this](auto&& subCtx)
                {
                    subCtx.setMainCtx(this);
                }
            );
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
        virtual void beforeThreadProcessing() override
        {
            beforeThreadProcessingNV();
        }

        /**
         * @brief Method to invoke just before the task leaves a thread.
         */
        virtual void afterThreadProcessing() override
        {
            afterThreadProcessingNV();
        }

        /**
         * @brief Method to invoke just after the task enters a thread.
         */
        void beforeThreadProcessingNV()
        {
            hana::eval_if(
                isNestedTaskContext,
                [this](){BaseTaskContext::beforeThreadProcessingNV();},
                [](){}
            );

            boost::hana::for_each(
                m_subcontexts,
                [](auto& subcontext)
                {
                    using type=typename std::decay_t<decltype(subcontext)>;
                    ThreadSubcontext<type>::setValue(&subcontext);
                    hana::eval_if(
                        TaskSubcontext::hasAsyncHandlerFns(hana::type_c<type>),
                        [&](auto _)
                        {
                            _(subcontext).onAsyncHandlerEnter();
                        },
                        [&](auto)
                        {}
                    );
                }
            );
        }

        /**
         * @brief Method to invoke just before the task leaves a thread.
         */
        void afterThreadProcessingNV()
        {
            hana::eval_if(
                isNestedTaskContext,
                [this](){BaseTaskContext::afterThreadProcessingNV();},
                [](){}
            );

            boost::hana::for_each(
                m_subcontexts,
                [](auto& subcontext)
                {
                    using type=typename std::decay_t<decltype(subcontext)>;
                    hana::eval_if(
                        TaskSubcontext::hasAsyncHandlerFns(hana::type_c<type>),
                        [&](auto _)
                        {
                            _(subcontext).onAsyncHandlerExit();
                        },
                        [&](auto)
                        {}
                    );

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
        const TaskSubcontextT<T>& get() const noexcept
        {
            using subcontextT=TaskSubcontextT<T>;

            auto wrapper=hana::find_if(
                m_refs,
                [](auto&& v)
                {
                    using crefT=std::decay_t<decltype(v)>;
                    using type=std::decay_t<typename crefT::type>;
                    return std::is_same<subcontextT,type>{};
                }
            );

            const auto* self=this;
            return hana::eval_if(
                hana::equal(wrapper,hana::nothing),
                [&](auto _) -> const TaskSubcontextT<T>&
                {                    
                    static_assert(!std::is_same<BaseTaskContext,TaskContext>::value,"Unknown context wrapper type");

                    const auto* base=static_cast<const BaseTaskContext*>(_(self));
                    return base->template get<T>();
                },
                [&](auto _) -> const TaskSubcontextT<T>&
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
        TaskSubcontextT<T>& get() noexcept
        {
            auto constSelf=const_cast<const selfT*>(this);
            return const_cast<TaskSubcontextT<T>&>(constSelf->template get<T>());
        }

        /**
         * @brief Get single subcontext of this context.
         * @return Const reference to subcontext.
         *
         * @note Can be used only with single subcontexts.
         */
        const auto& get() const noexcept
        {
            using st=decltype(hana::size(m_subcontexts));
            static_assert(st::value==1,"This method can be used only for contexts with single subcontext");
            return hana::front(m_subcontexts);
        }

        /**
         * @brief Get single subcontext of this context.
         * @return Const reference to subcontext.
         *
         * @note Can be used only with single subcontexts.
         */
        const auto& operator *() const noexcept
        {
            return get();
        }

        /**
         * @brief Get single subcontext of this context.
         * @return Const pointer to subcontext.
         *
         * @note Can be used only with single subcontexts.
         */
        const auto* operator ->() const noexcept
        {
            return &get();
        }

        /**
         * @brief Get single subcontext of this context.
         * @return Reference to subcontext.
         *
         * @note Can be used only with single subcontexts.
         */
        auto& get() noexcept
        {
            using st=decltype(hana::size(m_subcontexts));
            static_assert(st::value==1,"This method can be used only for contexts with single subcontext");
            return hana::front(m_subcontexts);
        }

        /**
         * @brief Get single subcontext of this context.
         * @return Reference to subcontext.
         *
         * @note Can be used only with single subcontexts.
         */
        auto& operator *() noexcept
        {
            return get();
        }

        /**
         * @brief Get single subcontext of this context.
         * @return Pointer to subcontext.
         *
         * @note Can be used only with single subcontexts.
         */
        auto* operator ->() noexcept
        {
            return &get();
        }

    private:

        auto subctxRefs() const
        {
            return hana::fold(
                m_subcontexts,
                hana::make_tuple(),
                [](auto&& ts, const auto& subctx)
                {
                    return hana::append(ts,std::cref(subctx));
                }
            );
        }

        Subcontexts m_subcontexts;
        typename detail::SubcontextRefs<Subcontexts>::type m_refs;
};

/**
 * @brief Helper for forwarding arguments to constructor of a subcontext.
 * @param args Arguments to forward.
 */
template <typename... Args>
constexpr auto subcontext(Args&&... args) noexcept
{
    return std::forward_as_tuple(std::forward<Args>(args)...);
}

/**
 * @brief Helper for forwarding arguments to constructor of base context of ActualTaskContext.
 * @param args Arguments to forward.
 */
template <typename... Args>
constexpr auto basecontext(Args&&... args) noexcept
{
    return std::forward_as_tuple(std::forward<Args>(args)...);
}

/**
 * @brief Helper for forwarding packed arguments to constroctors of subcontexts of ActualTaskContext.
 * @param args Packs of arguments to forward.
 */
template <typename... Args>
constexpr auto subcontexts(Args&&... args) noexcept
{
    return std::forward_as_tuple(std::forward<Args>(args)...);
}

namespace detail {

template <typename ...Types>
struct ActualTaskContexTraits
{
    constexpr static auto typeFn()
    {
        auto xs=hana::tuple_t<Types...>;
        auto last=hana::back(xs);
        using lastT=typename std::decay_t<decltype(last)>::type;
        auto tc=hana::eval_if(
            hana::is_a<TaskContexTag,lastT>,
            [&](auto _)
            {
                return hana::make_pair(hana::drop_back(_(xs)),_(last));
            },
            [&](auto _)
            {
                return hana::make_pair(_(xs),hana::type_c<TaskContext>);
            }
        );
        auto wrappersC=hana::transform(
                hana::first(tc),
                [](auto&& v)
                {
                    using type=typename std::decay_t<decltype(v)>::type;
                    return hana::eval_if(
                        hana::is_a<TaskSubcontextTag,type>,
                        [&](auto&&)
                        {
                            return hana::type_c<type>;
                        },
                        [&](auto&&)
                        {
                            return hana::type_c<TaskSubcontextT<type>>;
                        }
                    );
                }
            );
        using wrappersT=common::tupleCToStdTupleType<std::decay_t<decltype(wrappersC)>>;
        auto base=hana::second(tc);
        return hana::template_<ActualTaskContext>(hana::type_c<wrappersT>,base);
    }

    using tupleC=decltype(typeFn());
    using type=typename tupleC::type;
};

}

template <typename ...Types>
using TaskContextType=typename common::detail::ActualTaskContexTraits<Types...>::type;

/**
 * @brief Helper to make actual task context.
 *
 * Each Type of Types... parameter pack represents a subcontext derived from TaskSubcontext.
 * The first type can also be hana::is_a(TaskContexTag), i.e. some other ActualTaskContext that will
 * be used as a base class for created ActualTaskContext.
 *
 * Arguments are forwarded to constructor of ActualTaskContext. For arguments packing use
 * basecontext(), subcontexts() and subcontext() heplers, e.g.:
 * <pre>
 * auto ctx=makeTaskContext<Type1,Type2,Type3>(              
 *              subcontexts(
 *                  subcontext(type1-ctor-args...),
 *                  subcontext(type2-ctor-args...),
 *                  subcontext(type3-ctor-args...)
 *              ),
 *              basecontext(base-ctor-args...)
 *          );
 * </pre>
 */
template <typename ...Types>
struct makeTaskContextT
{
    using type=typename common::detail::ActualTaskContexTraits<Types...>::type;

    template <typename SubcontextsArgs, typename ...BaseArgs>
    auto operator()(SubcontextsArgs&& subcontextsArgs, BaseArgs&&... baseArgs) const
    {
        return makeShared<type>(std::forward<SubcontextsArgs>(subcontextsArgs),std::forward<BaseArgs>(baseArgs)...);
    }

    template <typename SubcontextsArgs>
    auto operator()(SubcontextsArgs&& subcontextsArgs) const
    {
        return makeShared<type>(std::forward<SubcontextsArgs>(subcontextsArgs));
    }

    auto operator()() const
    {
        return makeShared<type>();
    }
};
template <typename ...Types>
constexpr makeTaskContextT<Types...> makeTaskContext{};

/**
 * @brief Helper to allocate actual task context.
 *
 * @see makeTaskContextT for more details. The first argument of functor operator is an allocator.
 */
template <typename ...Types>
struct allocateTaskContextT
{
    using type=typename common::detail::ActualTaskContexTraits<Types...>::type;

    template <typename SubcontextsArgs, typename ...BaseArgs>
    auto operator()(const pmr::polymorphic_allocator<type>& allocator, SubcontextsArgs&& subcontextsArgs, BaseArgs&&... baseArgs) const
    {
        return allocateShared<type>(allocator,std::forward<SubcontextsArgs>(subcontextsArgs),std::forward<BaseArgs>(baseArgs)...);
    }

    template <typename SubcontextsArgs>
    auto operator()(const pmr::polymorphic_allocator<type>& allocator, SubcontextsArgs&& subcontextsArgs) const
    {
        return allocateShared<type>(allocator,std::forward<SubcontextsArgs>(subcontextsArgs));
    }

    auto operator()(const pmr::polymorphic_allocator<type>& allocator) const
    {
        return allocateShared<type>(allocator);
    }
};
template <typename ...Types>
constexpr allocateTaskContextT<Types...> allocateTaskContext{};

HATN_COMMON_NAMESPACE_END

#define HATN_TASK_CONTEXT_EXPAND(x) x
#define HATN_TASK_CONTEXT_GET_ARG3(arg1, arg2, arg3, ...) arg3

#define HATN_TASK_CONTEXT_DECLARE_EXPORT(Type,Export) \
    HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_BEGIN \
    HATN_COMMON_NAMESPACE_BEGIN \
    template <> \
    class Export ThreadSubcontext<TaskSubcontextT<Type>> \
    { \
        public: \
            static TaskSubcontextT<Type>* value() noexcept; \
            static void setValue(TaskSubcontextT<Type>* val) noexcept; \
            static void reset() noexcept; \
    }; \
    HATN_COMMON_NAMESPACE_END \
    HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_END

#define HATN_TASK_CONTEXT_DECLARE_NO_EXPORT(Type) \
    HATN_TASK_CONTEXT_DECLARE_EXPORT(Type,HATN_NO_EXPORT)

#define HATN_TASK_CONTEXT_DECLARE_SELECT(...) \
    HATN_TASK_CONTEXT_EXPAND(HATN_TASK_CONTEXT_GET_ARG3(__VA_ARGS__, \
                                                    HATN_TASK_CONTEXT_DECLARE_EXPORT, \
                                                    HATN_TASK_CONTEXT_DECLARE_NO_EXPORT \
                                                    ))

#define HATN_TASK_CONTEXT_DECLARE(...) HATN_TASK_CONTEXT_EXPAND(HATN_TASK_CONTEXT_DECLARE_SELECT(__VA_ARGS__)(__VA_ARGS__))

#define HATN_THREAD_SUBCONTEXT(Type) \
    HATN_COMMON_NAMESPACE::ThreadSubcontext<HATN_COMMON_NAMESPACE::TaskSubcontextT<Type>>::value()

#define HATN_TASK_CONTEXT_DEFINE_NAME(Type,Name) \
    HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_BEGIN \
    HATN_IGNORE_UNUSED_FUNCTION_BEGIN \
    HATN_IGNORE_UNUSED_VARIABLE_BEGIN \
    namespace { \
        thread_local static HATN_COMMON_NAMESPACE::TaskSubcontextT<Type>* TSInstance_##Name{nullptr}; \
    } \
    HATN_COMMON_NAMESPACE_BEGIN \
    TaskSubcontextT<Type>* ThreadSubcontext<TaskSubcontextT<Type>>::value() noexcept \
    { \
            return TSInstance_##Name; \
    } \
    void ThreadSubcontext<TaskSubcontextT<Type>>::setValue(TaskSubcontextT<Type>* val) noexcept \
    { \
            TSInstance_##Name=val; \
    } \
    void ThreadSubcontext<TaskSubcontextT<Type>>::reset() noexcept \
    { \
            TSInstance_##Name=nullptr; \
    } \
    template class ThreadSubcontext<TaskSubcontextT<Type>>; \
    HATN_COMMON_NAMESPACE_END \
    HATN_IGNORE_UNUSED_VARIABLE_END \
    HATN_IGNORE_UNUSED_FUNCTION_END \
    HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_END

#define HATN_TASK_CONTEXT_DEFINE_NO_NAME(Type) \
    HATN_TASK_CONTEXT_DEFINE_NAME(Type,Type)

#define HATN_TASK_CONTEXT_DEFINE_SELECT(...) \
    HATN_TASK_CONTEXT_EXPAND(HATN_TASK_CONTEXT_GET_ARG3(__VA_ARGS__, \
                                                    HATN_TASK_CONTEXT_DEFINE_NAME, \
                                                    HATN_TASK_CONTEXT_DEFINE_NO_NAME \
                                                    ))

#define HATN_TASK_CONTEXT_DEFINE(...) HATN_TASK_CONTEXT_EXPAND(HATN_TASK_CONTEXT_DEFINE_SELECT(__VA_ARGS__)(__VA_ARGS__))

#endif // HATNTASKCONTEXT_H
