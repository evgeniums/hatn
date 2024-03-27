/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/valuetypes.h
  *
  * Contains list of field value types.
  *
  */

/****************************************************************************/

#ifndef HATNFIELDVALUETYPES_H
#define HATNFIELDVALUETYPES_H

#include <boost/hana.hpp>
#include <hatn/dataunit/dataunit.h>

namespace hana=boost::hana;

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

namespace types {

template <ValueType TypeId>
constexpr auto IsInt=hana::bool_c<
                                TypeId==ValueType::Int8 ||
                                TypeId==ValueType::Int16 ||
                                TypeId==ValueType::Int32 ||
                                TypeId==ValueType::Int64 ||
                                TypeId==ValueType::Int8 ||
                                TypeId==ValueType::Int16 ||
                                TypeId==ValueType::Int32 ||
                                TypeId==ValueType::Int64
                                 >;

template <ValueType TypeId>
constexpr auto IsBool=hana::bool_c<
    TypeId==ValueType::Bool
    >;

template <ValueType TypeId>
constexpr auto IsDouble=hana::bool_c<
    TypeId==ValueType::Double || TypeId==ValueType::Float
    >;

template <ValueType TypeId>
constexpr auto IsScalar=hana::or_(IsInt<TypeId>,IsDouble<TypeId>,IsBool<TypeId>);

}

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNFIELDVALUETYPES_H
