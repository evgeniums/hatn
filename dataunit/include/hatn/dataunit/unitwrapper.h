/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/unitwrapper.h
  *
  * Contains definition of unit wrapper.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITWRAPPER_H
#define HATNDATAUNITWRAPPER_H

#include <hatn/dataunit/syntax.h>

#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class UnitWrapper
{
    public:

        UnitWrapper()
        {}

        template <typename T>
        UnitWrapper(HATN_COMMON_NAMESPACE::SharedPtr<T> sharedUnit) :
            m_shared(sharedUnit.template staticCast<Unit>())
        {}

        template <typename T>
        UnitWrapper& operator =(HATN_COMMON_NAMESPACE::SharedPtr<T> sharedUnit)
        {
            m_shared=sharedUnit.template staticCast<Unit>();
            return *this;
        }

        template <typename T>
        auto* unit()
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::unit_tag,T>,"T must be of Unit type");

            static T sample;
            return sample.castToUnit(m_shared.get());
        }

        template <typename T>
        const auto* unit() const
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::unit_tag,T>,"T must be of Unit type");

            static T sample;
            return sample.castToUnit(m_shared.get());
        }

        template <typename T>
        auto* managedUnit()
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>,"T must be of ManagedUnit type");

            static T sample;
            return sample.castToManagedUnit(m_shared.get());
        }

        template <typename T>
        auto* managedUnit() const
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>,"T must be of ManagedUnit type");

            static T sample;
            return sample.castToManagedUnit(m_shared.get());
        }

        bool isNull() const noexcept
        {
            return this->m_shared.isNull();
        }

        operator bool() const noexcept
        {
            return !isNull();
        }

        template <typename T>
        auto* get() const noexcept
        {
            auto self=const_cast<UnitWrapper*>(this);
            return self->template get<T>();
        }

        template <typename T>
        auto* get() noexcept
        {            
            if constexpr (hana::is_a<HATN_DATAUNIT_META_NAMESPACE::unit_tag,T>)
            {
                return unit<T>();
            }
            else if constexpr (hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>)
            {
                return managedUnit<T>();
            }
            else
            {
                static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>
                                  ||
                                  hana::is_a<HATN_DATAUNIT_META_NAMESPACE::unit_tag,T>
                              ,"T must be either of Unit or of ManagedUnit type");
                return T(nullptr);
            }
        }

        template <typename T>
        auto* mutableValue() noexcept
        {
            return *get<T>();
        }

        template <typename T>
        const auto& value() const noexcept
        {
            return *get<T>();
        }

        template <typename T>
        auto* as() const noexcept
        {
            return get<T>();
        }

        template <typename T>
        auto* as() noexcept
        {
            return get<T>();
        }

        auto shared() const noexcept
        {
            return m_shared;
        }

        auto* get() const noexcept
        {
            return m_shared.get();
        }

        template <typename T>
        auto sharedAs() const noexcept
        {
            return as<T>()->sharedFromThis();
        }

    protected:

        HATN_COMMON_NAMESPACE::SharedPtr<Unit> m_shared;
};

template <typename T>
class UnitWrapperT : public UnitWrapper
{
    public:

        static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>,"T must be of ManagedUnit type");

        UnitWrapperT()=default;

        template <typename T1>
        UnitWrapperT(HATN_COMMON_NAMESPACE::SharedPtr<T1> sharedUnit
                     ) : UnitWrapper(std::move(sharedUnit))
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>,"T1 must be of ManagedUnit type");
        }


        UnitWrapperT(UnitWrapper other) : UnitWrapper(std::move(other))
        {}

        auto* get() const noexcept
        {
            return this->template managedUnit<T>();
        }

        auto* get() noexcept
        {
            return this->template managedUnit<T>();
        }

        auto* mutableValue() noexcept
        {
            return *get();
        }

        const auto& value() const noexcept
        {
            return *get();
        }

        auto* operator->() const noexcept
        {
            return get();
        }

        auto* operator->() noexcept
        {
            return get();
        }

        auto& operator*() const noexcept
        {
            return value();
        }

        auto& operator*() noexcept
        {
            return mutableValue();
        }

        //! @note Helper in case UnitWrapperT is used in place of UnitWrapper
        template <typename T1>
        auto* as() const noexcept
        {
            static_assert(std::is_same<T,T1>::value,"T1 must be the same as T");
            return get();
        }

        template <typename T1>
        auto* as() noexcept
        {
            static_assert(std::is_same<T,T1>::value,"T1 must be the same as T");
            return get();
        }

        auto shared() const noexcept
        {
            return get()->sharedFromThis();
        }
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWRAPPER_H
