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
    : ::hatn::common::VInterfacesPack<Fields...>(
                    Fields(std::forward<Unit*>(self))...
            )
{
}

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

    static const common::FlatMap<int,uintptr_t> map=hana::fold(sample.m_interfaces,common::FlatMap<int,uintptr_t>{},f);

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
const Field* UnitConcat<Conf,Fields...>::doFieldById(int id) const
{
    return baseType::findField(this,id);
}

//---------------------------------------------------------------
template <typename Conf, typename ...Fields>
Field* UnitConcat<Conf,Fields...>::doFieldById(int id)
{
    return baseType::findField(this,id);
}

//---------------------------------------------------------------
template <typename Conf, typename ...Fields>
size_t UnitConcat<Conf,Fields...>::doFieldCount() const noexcept
{
    return this->count();
}

/********************** EmptyUnit **************************/

/********************** ManagedUnit **************************/

//---------------------------------------------------------------

/********************** EmptyManagedUnit **************************/

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSIMPL_H
