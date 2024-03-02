/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/*
    
*/
/** @file common/factory.h
 *
 *     Factory is a collection of object builders.
 *
 */
/****************************************************************************/

#ifndef HATNFACTORY_H
#define HATNFACTORY_H

#include <memory>
#include <map>

#include <hatn/common/common.h>
#include <hatn/common/builder.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace detail
{

template <typename T,typename =void>
struct FactoryTraits
{
};
template <typename T>
struct FactoryTraits<T,std::enable_if_t<HasCUID<T>::value>>
{
    template <typename ...Args>
    static T* create(
                const std::map<CUID_TYPE,std::shared_ptr<Builder>>& builders,
                Args&&... args
            )
    {
        auto it=builders.find(T::cuid());
        if (it==builders.end())
        {
            return nullptr;
        }
        auto builder=BuilderTraits<Args...>::dynamicCast(it->second.get());
        if (builder==nullptr)
        {
            return nullptr;
        }
        return builder->template create<T>(std::forward<Args>(args)...);
    }
};
template <typename T>
struct FactoryTraits<T,std::enable_if_t<!HasCUID<T>::value>>
{
    template <typename ...Args>
    static T* create(
            const std::map<CUID_TYPE,std::shared_ptr<Builder>>& builders,
            Args&&...
        )
    {
        std::ignore=builders;
        return nullptr;
    }
};

}

/**
 * @brief Factory of objects.
 *
 * @attention Each type that factory can create must have individual CUID even if it is derived from other type that already has CUID.
 * Otherwise, such situation is possible when factory actually builds the base class even the derived class was requested.

 * Bad example (wrong, don't do like this):
 * <pre>
 * class Base : public ClassUid<Base> {};
 * class Derived : public Base {};
 * Factory factory;
 * factory.addBuilder(BuilderMeta<BuilderTraits<>::Build<Base>,Base>::builder());
 * auto derivedObj=factory.create<Derived>(); // <-- WRONG, WRONG, WRONG - derivedObj is not nullptr, but you can't see the problem
 * assert(derivedObj!=nullptr); // will pass, bad casting is not detected
 * // ... crash or UB is expected ...
 * </pre>
 *
 * To avoid such situations use with builders only objects that have individual CUIDs.
 * Good example (it fails, but you can check it explicitly in runtime):
 *
 * <pre>
 * class Base : public ClassUid<Base> {};
 * class Derived : pulic Base, public ClassUid<Base> { using ClassUid<Base>::cuid; };
 * Factory factory;
 * factory.addBuilder(BuilderMeta<BuilderTraits<>::Build<Base>,Base>::builder());
 * auto derivedObj=factory.create<Derived>(); // <-- derivedObj is nullptr, not good, but you can see that
 * assert(derivedObj!=nullptr); // will fail, bad casting is detected
 * </pre>
 */
class Factory
{
    public:

        Factory()=default;

        virtual ~Factory()=default;
        Factory(const Factory&)=default;
        Factory(Factory&&) =default;
        Factory& operator=(const Factory&)=default;
        Factory& operator=(Factory&&) =default;

        //! Create object of type T
        template <typename T, typename ...Args>
        T* create(Args&&... args) const
        {
            return detail::FactoryTraits<T>::create(m_builders,std::forward<Args>(args)...);
        }

        /**
         * @brief Add builder to factory
         * @param builder Builder to add
         *
         * Builder will be copied to the factory, not moved, because there can be
         * multiple copies of the same builder for diffirent CUIDs.
         */
        void addBuilder(const std::shared_ptr<Builder>& builder)
        {
            for (auto&& it:builder->cuids())
            {
                // copy to table, not move
                m_builders[it]=builder;
            }
        }

    private:

        std::map<CUID_TYPE,std::shared_ptr<Builder>> m_builders;
};

/**
 * @brief Helper class to construct factory with builders from template parameters
 */
template <typename ...BuilderMetas>
class MakeFactory
{
};
template <typename BuilderMetaT>
class MakeFactory<BuilderMetaT> : public Factory
{
    public:

        MakeFactory()
        {
            addBuilder(BuilderMetaT::builder());
        }
};
template <typename BuilderMetaT,typename ...BuilderMetas>
class MakeFactory<BuilderMetaT,BuilderMetas...> : public MakeFactory<BuilderMetas...>
{
    public:

        MakeFactory()
        {
            this->addBuilder(BuilderMetaT::builder());
        }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNENVIRONMENT_H
