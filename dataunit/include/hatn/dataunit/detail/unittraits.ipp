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
    if (!m_mapReady.load(std::memory_order_acquire))
    {
        fillMap(this);
    }
}

//---------------------------------------------------------------
template <typename ...Fields>
bool UnitImpl<Fields...>::iterate(const Unit::FieldVisitor& visitor)
{
    auto predicate=[](bool ok)
    {
        return ok;
    };

    auto handler=[&visitor](auto& field, auto&&)
    {
        return visitor(field);
    };

    return each(predicate,handler);
}

//---------------------------------------------------------------
template <typename ...Fields>
bool UnitImpl<Fields...>::iterateConst(const Unit::FieldVisitorConst& visitor) const
{
    auto predicate=[](bool ok)
    {
        return ok;
    };

    auto handler=[&visitor](const auto& field, auto&&)
    {
        return visitor(field);
    };

    return each(predicate,handler);
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
bool UnitConcat<Conf,Fields...>::doIterateFields(const Unit::FieldVisitor& visitor)
{
    return this->iterate(visitor);
}

//---------------------------------------------------------------
template <typename Conf, typename ...Fields>
bool UnitConcat<Conf,Fields...>::doIterateFieldsConst(const Unit::FieldVisitorConst& visitor) const
{
    return this->iterateConst(visitor);
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
