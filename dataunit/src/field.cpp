/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/fieldbase.сpp
  *
  *      Base class of dataunit fields
  *
  */

#include <hatn/dataunit/field.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/********************** Field **************************/

//---------------------------------------------------------------
Field::Field(ValueType valeuTypeId,Unit* unit, bool array)
    : m_set(false),
      m_unit(unit),
      m_valueTypeId(valeuTypeId),
      m_array(array)
{}

//---------------------------------------------------------------
Field::~Field()=default;

//---------------------------------------------------------------
WireType Field::wireType() const noexcept
{
    return wireTypeDef();
}

//---------------------------------------------------------------
bool Field::isRepeatedUnpackedProtoBuf() const noexcept
{
    return false;
}

//---------------------------------------------------------------
void Field::setParseToSharedArrays(bool enable,::hatn::dataunit::AllocatorFactory*)
{
    std::ignore=enable;
}

//---------------------------------------------------------------
bool Field::isParseToSharedArrays() const noexcept
{
    return false;
}

//---------------------------------------------------------------
bool Field::hasDefaultValue() const noexcept
{
    return false;
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
