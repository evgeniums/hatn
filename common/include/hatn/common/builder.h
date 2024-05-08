/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/builder.h
 *
 *     Base template class for object builders
 *
 */
/****************************************************************************/

#ifndef HATNBUILDER_H
#define HATNBUILDER_H

#include <memory>
#include <type_traits>

#include <hatn/common/common.h>
#include <hatn/common/interface.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * @brief Base class for object builders
 */
struct Builder
{
    /**
     * @brief Ctor
     * @param cuids IDs of classes this builder can build
     */
    Builder(std::set<CUID_TYPE> cuids):m_cuids(std::move(cuids))
    {}

    virtual ~Builder()=default;
    Builder(const Builder&)=default;
    Builder(Builder&&)=default;
    Builder& operator=(const Builder&)=default;
    Builder& operator=(Builder&&)=default;

    /**
     * @brief Get IDs of classes this builder can build
     * @return IDs
     */
    std::set<CUID_TYPE> cuids() const
    {
        return m_cuids;
    }

    protected:

        std::set<CUID_TYPE> m_cuids;
};

/**
 * @brief Traits for builders that can build objects with certain signature of consructor
 */
template <typename ...CtorArgs>
struct BuilderTraits
{
    /**
     * @brief Abstract builder interface
     */
    struct Interface : public Builder
    {
        using Builder::Builder;

        virtual ~Interface()=default;
        Interface(const Interface&)=default;
        Interface(Interface&&) =default;
        Interface& operator=(const Interface&)=default;
        Interface& operator=(Interface&&) =default;

        /**
         * @brief Create object
         * @param args Constructor arguments
         * @return Created object
         */
        template <typename T>
        T* create(CtorArgs&&... args) const
        {
            Assert(m_cuids.find(T::cuid())!=m_cuids.end(),"Type not supported by this builder");
            return static_cast<T*>(doCreate(std::forward<CtorArgs>(args)...));
        }

        protected:

            virtual void* doCreate(CtorArgs&&... args) const =0;
    };

    template <typename T>
    struct Build : public Interface
    {
        using Type=T;
        using Interface::Interface;

        protected:

            virtual void* doCreate(CtorArgs&&... args) const override
            {
                return new T(std::forward<CtorArgs>(args)...);
            }
    };

    /**
     * @brief Cast dynamically builder to specific interface.
     * @param builder Builder
     * @return Casted interface or nulptr if failed to cast
     */
    static Interface* dynamicCast(common::Builder* builder)
    {
        return dynamic_cast<Interface*>(builder);
    }
};

/**
 * @brief Meta of a builder that collects IDs of objects that this builder can create.
 */
template <typename ...>
struct BuilderMeta
{
};

template <typename BuilderT,typename Type>
struct BuilderMeta<BuilderT,Type>
{
    static_assert(std::is_base_of<Type,typename BuilderT::Type>::value,"This builder can not be used to build such types");
    static std::set<CUID_TYPE> cuids()
    {
        return std::set<CUID_TYPE>{Type::cuid()};
    }
    static std::shared_ptr<Builder> builder()
    {
        return std::make_shared<BuilderT>(cuids());
    }
};

template <typename BuilderT,typename Type,typename ...Types>
struct BuilderMeta<BuilderT,Type,Types...> : public BuilderMeta<BuilderT,Types...>
{
    static_assert(std::is_base_of<Type,typename BuilderT::Type>::value,"This builder can not be used to build such types");

    using BaseT=BuilderMeta<BuilderT,Types...>;
    using BaseT::builder;

    static std::set<CUID_TYPE> cuids()
    {
        auto s=BaseT::cuids();
        s.insert(Type::cuid());
        return s;
    }
    static std::shared_ptr<Builder> builder()
    {
        return std::make_shared<BuilderT>(cuids());
    }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNBUILDER_H
