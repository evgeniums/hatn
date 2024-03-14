/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved


  */

/****************************************************************************/
/*

*/
/** \file datauint/detail/unittraits.ipp
  *
  *      Implementations of DataUnit templates
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSIMPL_H
#define HATNDATAUNITSIMPL_H

#include <hatn/dataunit/unittraits.h>

namespace hatn {
namespace dataunit {

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
template <typename T,int Index>
bool UnitImpl<Fields...>::Iterator<T,Index>::next(
        T& unit,
        const Unit::FieldVisitor& callback
    )
{
    static_assert(Index>=0&&Index<= MaxI,"Iterator index overflow");
    auto& field = std::get<Index>(unit.m_interfaces);
    if (callback)
    {
        if (!callback(field))
        {
            return false;
        }
    }
    return Iterator<T,Index-1>::next(unit,callback);
}

//---------------------------------------------------------------
template <typename ...Fields>
template <typename T>
bool UnitImpl<Fields...>::Iterator<T,0>::next(
        T& unit,
        const Unit::FieldVisitor& callback
    )
{
    auto& field = std::get<0>(unit.m_interfaces);
    if (callback)
    {
        if (!callback(field))
        {
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------
template <typename ...Fields>
template <typename T,int Index>
bool UnitImpl<Fields...>::Iterator<T,Index>::nextConst(
        const T& unit,
        const Unit::FieldVisitorConst& callback
    )
{
    static_assert(Index>=0&&Index<= MaxI,"Iterator index overflow");
    const auto& field = std::get<Index>(unit.m_interfaces);
    if (callback)
    {
        if (!callback(field))
        {
            return false;
        }
    }
    return Iterator<T,Index-1>::nextConst(unit,callback);
}

//---------------------------------------------------------------
template <typename ...Fields>
template <typename T>
bool UnitImpl<Fields...>::Iterator<T,0>::nextConst(
        const T& unit,
        const Unit::FieldVisitorConst& callback
    )
{
    auto& field = std::get<0>(unit.m_interfaces);
    if (callback)
    {
        if (!callback(field))
        {
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------
template <typename ...Fields>
bool UnitImpl<Fields...>::iterate(const Unit::FieldVisitor& visitor)
{
    return Iterator<UnitImpl<Fields...>,MaxI>::next(*this,visitor);
}

//---------------------------------------------------------------
template <typename ...Fields>
bool UnitImpl<Fields...>::iterateConst(const Unit::FieldVisitorConst& visitor) const
{
    return Iterator<UnitImpl<Fields...>,MaxI>::nextConst(*this,visitor);
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

}
}

#endif // HATNDATAUNITSIMPL_H
