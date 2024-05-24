/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file datauint/detail/unittraits.ipp
  *
  *      Implementations of DataUnit templates
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSIMPL_H
#define HATNDATAUNITSIMPL_H

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/unittraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/********************** UnitImpl **************************/

//---------------------------------------------------------------
template <typename ...Fields>
UnitImpl<Fields...>::UnitImpl(Unit* self)
    : m_fields(Fields{self}...)
{}

//---------------------------------------------------------------

template <typename ...Fields>
const common::FlatMap<int,uintptr_t>& UnitImpl<Fields...>::fieldsMap()
{
    static Unit sampleUnit{};
    static const UnitImpl<Fields...> sample(&sampleUnit);

    auto f = [](common::FlatMap<int,uintptr_t> m, const auto& field) {
        auto m1=std::move(m);
        m1[field.fieldId()]=reinterpret_cast<uintptr_t>(&field)-reinterpret_cast<uintptr_t>(&sample);
        return m1;
    };

    static const common::FlatMap<int,uintptr_t> map=hana::fold(sample.m_fields,common::FlatMap<int,uintptr_t>{},f);
    return map;
}

//---------------------------------------------------------------

template <typename ...Fields>
const common::FlatMap<common::lib::string_view,uintptr_t>& UnitImpl<Fields...>::fieldsNameMap()
{
    static Unit sampleUnit{};
    static const UnitImpl<Fields...> sample(&sampleUnit);

    auto f = [](common::FlatMap<common::lib::string_view,uintptr_t> m, const auto& field) {
        auto m1=std::move(m);
        m1[field.fieldName()]=reinterpret_cast<uintptr_t>(&field)-reinterpret_cast<uintptr_t>(&sample);
        return m1;
    };

    static const common::FlatMap<common::lib::string_view,uintptr_t> map=hana::fold(sample.m_fields,common::FlatMap<common::lib::string_view,uintptr_t>{},f);
    return map;
}

/********************** UnitConcat **************************/

//---------------------------------------------------------------

template <typename Conf, typename ...Fields>
UnitConcat<Conf,Fields...>::UnitConcat(
        AllocatorFactory* factory
    ) : Unit(factory),
        UnitImpl<Fields...>(this)
{}

//---------------------------------------------------------------

template <typename Conf, typename ...Fields>
common::CUID_TYPE UnitConcat<Conf,Fields...>::cuid() noexcept
{
    static int dummy;
    return reinterpret_cast<common::CUID_TYPE>(&dummy);
}

/********************** EmptyUnit **************************/

template <typename Conf>
common::CUID_TYPE EmptyUnit<Conf>::cuid() noexcept
{
    static int dummy;
    return reinterpret_cast<common::CUID_TYPE>(&dummy);
}

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSIMPL_H
