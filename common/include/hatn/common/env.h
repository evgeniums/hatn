/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/env.h
 *
 *     Base class for env classes.
 *
 */
/****************************************************************************/

#ifndef HATNENV_H
#define HATNENV_H

#include <functional>

#include <boost/hana.hpp>

#include <hatn/common/common.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/meta/decaytuple.h>
#include <hatn/common/meta/tupletypec.h>
#include <hatn/common/meta/foreachif.h>

#include <hatn/common/taskcontext.h>

HATN_COMMON_NAMESPACE_BEGIN

struct EnvTag{};

class HATN_COMMON_EXPORT BaseEnv : public common::ClassUid<BaseEnv>,
                                   public EnableSharedFromThis<BaseEnv>
{
    public:

        HATN_CUID_DECLARE()

        using hana_tag=EnvTag;

        BaseEnv(std::string name={}) : m_name(std::move(name))
        {}

        virtual ~BaseEnv()=default;

        const std::string& name() const noexcept
        {
            return m_name;
        }

        void setName(std::string name)
        {
            m_name=std::move(name);
        }

        virtual CUID_TYPE typeID() const noexcept
        {
            return cuid();
        }

    private:

        std::string m_name;
};

struct EnvContextTag{};

class EnvContext : public EnableSharedFromThis<EnvContext>
{
    public:

        using hana_tag=EnvContextTag;

        EnvContext() : m_env(nullptr)
        {}

        void setEnv(BaseEnv* env) noexcept
        {
            m_env=env;
        }

        BaseEnv* env() noexcept
        {
            return m_env;
        }

        const BaseEnv* env() const noexcept
        {
            return m_env;
        }

    private:

        BaseEnv* m_env;
};

/**
 * @brief Helper to wrap arbitrary type to Env context.
 */
template <typename T>
class EnvContextT : public T,
                    public EnvContext
{
    public:

        using Base=T;

        template <typename ...Args>
        EnvContextT(
                Args&& ...args
            ) : T(std::forward<Args>(args)...)
        {}

        template <typename ...Types>
        EnvContextT(
                std::tuple<Types...>&& ts
            ) : EnvContextT(
                      std::forward<std::tuple<Types...>>(ts),
                      std::make_index_sequence<std::tuple_size<std::remove_reference_t<std::tuple<Types...>>>::value>{}
                  )
        {}

    private:

        template <typename Ts, std::size_t... I>
        EnvContextT(
                Ts&& ts,
                std::index_sequence<I...>
            ) : EnvContextT(std::get<I>(std::forward<Ts>(ts))...)
        {}
};

/**
 * @brief The Env class represent a holder of contexts where each context can be accessed with get() method.
 */
template <typename Contexts, typename BaseT=BaseEnv>
class EnvT : public BaseT
{
    public:

        using selfT=EnvT<Contexts,BaseT>;
        using Base=BaseT;

        constexpr static auto isNested=boost::hana::not_(std::is_same<BaseT,BaseEnv>{});

        /**
         * @brief Default constructor.
         */
        EnvT(std::string name={}) :
                Base(std::move(name)),
                m_contexts(),
                m_refs(ctxRefs())
        {
            initContexts();
        }

        /**
             * @brief Constructor from tuples of tuples.
             * @param tts Tuple of tuples to forward to constructors of contexts of this class.
             * @param baseTs Arguments to forward to base class.
             */
        template <typename Tts, typename ...BaseTs>
        EnvT(Tts&& tts, BaseTs&& ...baseTs):
            BaseT(std::forward<BaseTs>(baseTs)...),
            m_contexts(std::forward<Tts>(tts)),
            m_refs(ctxRefs())
        {
            initContexts();
        }

        /**
         * @brief Get interface of base env.
         * @return Result.
         */
        Base* baseEnv() noexcept
        {
            return static_cast<Base*>(this);
        }

        /**
         * @brief Get tuple of contexts.
         * @return Const reference to contexts.
         */
        const Contexts& contexts() const noexcept
        {
            return m_contexts;
        }

        /**
         * @brief Get tuple of contexts.
         * @return Reference to contexts.
         */
        Contexts& contexts() noexcept
        {
            return m_contexts;
        }

        /**
         * @brief Check if environment has a context.
         */
        template <typename T>
        constexpr auto hasContext() const noexcept
        {
            auto wrapper=boost::hana::find_if(
                m_refs,
                [](auto&& v)
                {
                    using crefT=std::decay_t<decltype(v)>;
                    using type=std::decay_t<typename crefT::type>;
                    return std::is_base_of<T,type>{};
                }
            );
            return boost::hana::not_equal(wrapper,boost::hana::nothing);
        }

        auto ctxRefs() const
        {
            return boost::hana::fold(
                m_contexts,
                boost::hana::make_tuple(),
                [](auto&& ts, const auto& subctx)
                {
                    return boost::hana::append(ts,std::cref(subctx));
                }
            );
        }

        auto contextRefs() const
        {
            return m_refs;
        }

        /**
         * @brief Get context by type.
         * @return Const reference to context.
         */
        template <typename T>
        const T& get() const noexcept
        {
            // find context in m_refs of this
            auto wrapper=boost::hana::find_if(
                m_refs,
                [](auto&& v)
                {
                    using crefT=std::decay_t<decltype(v)>;
                    using type=std::decay_t<typename crefT::type>;
                    return std::is_base_of<T,type>{};
                }
            );

            auto tc=boost::hana::type_c<T>;
            auto bc=boost::hana::type_c<Base>;

            const auto* self=this;
            return boost::hana::eval_if(
                boost::hana::equal(wrapper,boost::hana::nothing),
                [&](auto _) -> const T&
                {
                    // context not found in this, try to find it in base env

                    using baseT=typename std::decay_t<decltype(_(bc))>::type;
                    static_assert(!std::is_same<baseT,BaseEnv>::value,"Unknown context type");

                    using type=typename std::decay_t<decltype(_(tc))>::type;
                    const auto* base=static_cast<const baseT*>(_(self));
                    return base->template get<type>();
                },
                [&](auto _) -> const T&
                {
                    // context is present in this
                    return _(wrapper).value();
                }
            );
        }

        /**
         * @brief Get context by type.
         * @return Reference to context.
         */
        template <typename T>
        T& get() noexcept
        {
            auto constSelf=const_cast<const selfT*>(this);
            return const_cast<T&>(constSelf->template get<T>());
        }

        /**
         * @brief Get single context of this env.
         * @return Const reference to env.
         *
         * @note Can be used only with single contexts.
         */
        const auto& get() const noexcept
        {
            using st=decltype(boost::hana::size(m_contexts));
            static_assert(st::value==1,"This method can be used only for env with single context");
            return boost::hana::front(m_contexts);
        }

        /**
         * @brief Get single context of this env.
         * @return Const reference to context.
         *
         * @note Can be used only with single contexts.
         */
        const auto& operator *() const noexcept
        {
            return get();
        }

        /**
         * @brief Get single context of this env.
         * @return Const pointer to context.
         *
         * @note Can be used only with single contexts.
         */
        const auto* operator ->() const noexcept
        {
            return &get();
        }

        /**
         * @brief Get single context of this env.
         * @return Reference to context.
         *
         * @note Can be used only with single contexts.
         */
        auto& get() noexcept
        {
            using st=decltype(boost::hana::size(m_contexts));
            static_assert(st::value==1,"This method can be used only for env with single context");
            return boost::hana::front(m_contexts);
        }

        /**
         * @brief Get single context of this env.
         * @return Reference to context.
         *
         * @note Can be used only with single contexts.
         */
        auto& operator *() noexcept
        {
            return get();
        }

        /**
         * @brief Get single context of this env.
         * @return Pointer to context.
         *
         * @note Can be used only with single contexts.
         */
        auto* operator ->() noexcept
        {
            return &get();
        }

        template <typename VisitorT, typename SelectorT>
        void visitIf(VisitorT&& visitor, SelectorT&& selector)
        {
            auto pred=[](bool found)
            {
                return found;
            };

            common::foreach_if(
                m_contexts,
                pred,
                [&visitor,&selector](auto& v,auto)
                {
                    if (selector(v))
                    {
                        visitor(v);
                        return true;
                    }
                    return false;
                }
            );
        }

        template <typename VisitorT, typename SelectorT>
        void visitIfConst(VisitorT&& visitor, SelectorT&& selector)
        {
            auto pred=[](bool found)
            {
                return found;
            };

            common::foreach_if(
                m_contexts,
                pred,
                [&visitor,&selector,this](auto&& v,auto)
                {
                    using type=std::decay_t<decltype(v)>;
                    auto& ctx=get<type>();

                    return hana::eval_if(
                        selector(ctx),
                        [&](auto _)
                        {
                            _(visitor)(std::forward<decltype(v)>(v));
                            return true;
                        },
                        []()
                        {
                            return true;
                        }
                    );
                }
            );
        }

    private:

        void initContexts()
        {
            boost::hana::for_each(
                m_contexts,
                [this](auto& ctx)
                {
                    ctx.setEnv(this);
                }
            );
        }

        Contexts m_contexts;
        typename TupleRefs<Contexts>::type m_refs;        
};

/**
 * @brief Helper for forwarding arguments to constructor of a context.
 * @param args Arguments to forward.
 */
template <typename... Args>
constexpr auto context(Args&&... args) noexcept
{
    return std::forward_as_tuple(std::forward<Args>(args)...);
}

/**
 * @brief Helper for forwarding arguments to constructor of base Env.
 * @param args Arguments to forward.
 */
template <typename... Args>
constexpr auto baseEnv(Args&&... args) noexcept
{
    return std::forward_as_tuple(std::forward<Args>(args)...);
}

/**
 * @brief Helper for forwarding packed arguments to constroctors of contexts of Env.
 * @param args Packs of arguments to forward.
 */
template <typename... Args>
constexpr auto contexts(Args&&... args) noexcept
{
    return std::forward_as_tuple(std::forward<Args>(args)...);
}

namespace detail {

template <template <typename ...> class EnvClass, typename ...Types>
struct EnvTraits
{
    constexpr static auto typeFn()
    {
        auto xs=boost::hana::tuple_t<Types...>;
        auto last=boost::hana::back(xs);
        using lastT=typename std::decay_t<decltype(last)>::type;
        auto tc=boost::hana::eval_if(
            boost::hana::is_a<EnvTag,lastT>,
            [&](auto _)
            {
                // if last type is base env then drop it from tuple and set as second member of pair
                return boost::hana::make_pair(boost::hana::drop_back(_(xs)),_(last));
            },
            [&](auto _)
            {
                // return pair with first is a tuple and second is a base env
                return boost::hana::make_pair(_(xs),boost::hana::type_c<BaseEnv>);
            }
        );

        // prepare tuple to be used in variadic args of Env
        auto wrappersC=boost::hana::transform(
            boost::hana::first(tc),
            [](auto&& v)
            {
                using type=typename std::decay_t<decltype(v)>::type;
                return hana::eval_if(
                    hana::is_a<EnvContextTag,type>,
                    [&](auto&&)
                    {
                        return hana::type_c<type>;
                    },
                    [&](auto&&)
                    {
                        return hana::type_c<EnvContextT<type>>;
                    }
                );
            }
        );
        using wrappersT=common::tupleCToStdTupleType<std::decay_t<decltype(wrappersC)>>;
        auto base=boost::hana::second(tc);

        // return actual type of Env
        return boost::hana::template_<EnvClass>(boost::hana::type_c<wrappersT>,base);
    }

    using tupleC=decltype(typeFn());
    using type=typename tupleC::type;
};

}

template <template <typename ...> class EnvClass,typename ...Types>
using EnvTmpl=typename detail::EnvTraits<EnvClass,Types...>::type;

template <typename ...Types>
using Env=EnvTmpl<EnvT,Types...>;

template <typename Type>
struct makeEnvTypeT
{
    using type=Type;

    template <typename ContextsArgs, typename ...BaseArgs>
    auto operator()(ContextsArgs&& contextsArgs, BaseArgs&&... baseArgs) const
    {
        return makeShared<type>(std::forward<ContextsArgs>(contextsArgs),std::forward<BaseArgs>(baseArgs)...);
    }

    template <typename ContextsArgs>
    auto operator()(ContextsArgs&& contextsArgs) const
    {
        return makeShared<type>(std::forward<ContextsArgs>(contextsArgs));
    }

    auto operator()() const
    {
        return makeShared<type>();
    }
};
template <typename Type>
constexpr makeEnvTypeT<Type> makeEnvType{};

/**
 * @brief Helper to make actual Env with contexts.
 *
 * Each Type of Types... parameter pack represents a context.
 * The last type can also be hana::is_a(EnvTag), i.e. some other Env that will
 * be used as a base class for created Env.
 *
 * Arguments are forwarded to constructor of Env. For arguments packing use
 * basecontext(), contexts() and context() heplers, e.g.:
 * <pre>
 * auto ctx=makeEnv<Type1,Type2,Type3>(
 *              contexts(
 *                  context(type1-ctor-args...),
 *                  context(type2-ctor-args...),
 *                  context(type3-ctor-args...)
 *              ),
 *              basecontext(base-ctor-args...)
 *          );
 * </pre>
 */
template <typename ...Types>
struct makeEnvT
{
    using type=Env<Types...>;

    template <typename ...Args>
    auto operator()(Args&&... args) const
    {
        return makeEnvType<type>(std::forward<Args>(args)...);
    }
};
template <typename ...Types>
constexpr makeEnvT<Types...> makeEnv{};

template <typename Type>
struct allocateEnvTypeT
{
    using type=Type;

    template <typename ContextsArgs, typename ...BaseArgs>
    auto operator()(const pmr::polymorphic_allocator<type>& allocator, ContextsArgs&& contextsArgs, BaseArgs&&... baseArgs) const
    {
        return allocateShared<type>(allocator,std::forward<ContextsArgs>(contextsArgs),std::forward<BaseArgs>(baseArgs)...);
    }

    template <typename ContextsArgs>
    auto operator()(const pmr::polymorphic_allocator<type>& allocator, ContextsArgs&& contextsArgs) const
    {
        return allocateShared<type>(allocator,std::forward<ContextsArgs>(contextsArgs));
    }

    auto operator()(const pmr::polymorphic_allocator<type>& allocator) const
    {
        return allocateShared<type>(allocator);
    }
};
template <typename Type>
constexpr allocateEnvTypeT<Type> allocateEnvType{};

/**
 * @brief Helper to allocate actual Env.
 *
 * @see makeEnvT for more details. The first argument of functor operator is an allocator.
 */
template <typename ...Types>
struct allocateEnvT
{
    using type=Env<Types...>;

    template <typename ...Args>
    auto operator()(const pmr::polymorphic_allocator<type>& allocator, Args&&... args) const
    {
        return allocateEnvType<type>(allocator,std::forward<Args>(args)...);
    }
};
template <typename ...Types>
constexpr allocateEnvT<Types...> allocateEnv{};

/**
 * @brief The WithEmbeddedEnvBase class
 */
template <typename EmbeddedEnvT>
class WithEmbeddedEnvBase
{
    public:

        WithEmbeddedEnvBase(common::SharedPtr<EmbeddedEnvT> env={}) : m_emddedEnv(std::move(env))
        {}

        void setEmbeddedEnv(common::SharedPtr<EmbeddedEnvT> env)
        {
            m_emddedEnv=std::move(env);
        }

        common::SharedPtr<EmbeddedEnvT> embeddedEnvShared() const noexcept
        {
            return m_emddedEnv;
        }

        const EmbeddedEnvT* embeddedEnv() const noexcept
        {
            return m_emddedEnv.get();
        }

        EmbeddedEnvT* embeddedEnv() noexcept
        {
            return m_emddedEnv.get();
        }

    private:

        common::SharedPtr<EmbeddedEnvT> m_emddedEnv;
};

/**
 * @brief The WithEmbeddedEnvT class
 */
template <typename EmbeddedEnvT, typename Contexts, typename BaseT=common::BaseEnv>
class WithEmbeddedEnvT : public common::EnvT<Contexts,BaseT>,
                         public WithEmbeddedEnvBase<EmbeddedEnvT>
{
    public:

        using selfT=WithEmbeddedEnvT<EmbeddedEnvT,Contexts,BaseT>;
        using baseT=common::EnvT<Contexts,BaseT>;

        using common::EnvT<Contexts,BaseT>::EnvT;

        template <typename T>
        constexpr auto hasContext() const
        {
            Assert(this->embeddedEnv(),"Embedded environment not set");

            const baseT* base=this;
            auto typeC=hana::type_c<T>;

            return hana::eval_if(
                this->embeddedEnv()->template hasContext<T>(),
                []()
                {
                    return hana::true_c;
                },
                [&](auto _)
                {
                    using type=typename std::decay_t<decltype(_(typeC))>::type;
                    return _(base)->template hasContext<type>();
                }
            );
        }

        template <typename T>
        const T& get() const
        {
            Assert(this->embeddedEnv(),"Embedded environment not set");

            const auto* self=this;
            const baseT* base=this;
            auto typeC=hana::type_c<T>;

            return hana::eval_if(
                this->embeddedEnv()->template hasContext<T>(),
                [&](auto _) -> decltype(auto)
                {
                    using type=typename std::decay_t<decltype(_(typeC))>::type;
                    return _(self)->embeddedEnv()->template get<type>();
                },
                [&](auto _) -> decltype(auto)
                {
                    using type=typename std::decay_t<decltype(_(typeC))>::type;
                    return _(base)->template get<type>();
                }
            );
        }

        template <typename T>
        T& get()
        {
            auto constSelf=const_cast<const selfT*>(this);
            return const_cast<T&>(constSelf->template get<T>());
        }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNENV_H
