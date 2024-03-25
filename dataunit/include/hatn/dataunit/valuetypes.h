/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/valuetypes.h
  *
  * Contains list of field value types.
  *
  */

/****************************************************************************/

#ifndef HATNFIELDVALUETYPES_H
#define HATNFIELDVALUETYPES_H

#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

enum class ValueType : int
{
    Bool,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float,
    Double,
    String,
    Bytes,
    Dataunit
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNFIELDVALUETYPES_H
