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

namespace hana=boost::hana;

#include <hatn/common/common.h>
#include <hatn/common/managedobject.h>
#include <hatn/common/meta/decaytuple.h>
#include <hatn/common/meta/tupletypec.h>
#include <hatn/common/makeshared.h>

HATN_COMMON_NAMESPACE_BEGIN

struct TaskContextTag{};

class HATN_COMMON_EXPORT TaskContext : public ManagedObject
{
    public:

        using hana_tag=TaskContextTag;

        virtual void beforeThreadProcessing();

        virtual void afterThreadProcessing();
};

template <typename T>
class TaskContextWrapper
{
    public:

        using type=T;

        // Impement in derived class.

        // const T* value() const noexcept
        // {
        //     return &...;
        // }
        // T* value() noexcept
        // {
        //     return &...;
        // }
};

template <typename T>
class ThreadLocalContext
{
    static T* value() noexcept;
    static void setValue(T* val) noexcept;
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

        TaskContextT():
            m_wrappers(),
            m_refs(wrapperRefs())
        {}

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
        using wrappersT=std::decay_t<decltype(tupleCToTuple(wrappersC))>;
        auto base=hana::second(tc);
        return hana::template_<TaskContextT>(hana::type_c<wrappersT>,base);
    }

    using tupleC=decltype(typeFn());
    using type=typename tupleC::type;
};

}

template <typename ...Types>
auto makeTaskContext()
{
    using type=typename detail::TaskContextTraits<Types...>::type;
    return makeShared<type>();
}

template <typename ...Types>
auto allocateTaskContext(const pmr::polymorphic_allocator<typename detail::TaskContextTraits<Types...>::type>& allocator)
{
    return allocateShared<typename detail::TaskContextTraits<Types...>::type>(allocator);
}

HATN_COMMON_NAMESPACE_END

#define HATN_TASK_CONTEXT_DECLARE(Type,Export) \
    HATN_COMMON_NAMESPACE_BEGIN \
    template <> \
    class Export ThreadLocalContext<Type> \
    { \
        public: \
            static Type* value() noexcept; \
            static void setValue(Type* val) noexcept; \
            static void reset() noexcept; \
    }; \
    HATN_COMMON_NAMESPACE_END

#define HATN_TASK_CONTEXT_DEFINE(Type) \
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
    HATN_COMMON_NAMESPACE_END

#endif // HATNTASKCONTEXT_H
