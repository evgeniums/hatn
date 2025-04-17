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

//! Field types as defined in Google Protocol Buffers
enum class WireType : int
{
    VarInt=0,
    Fixed64=1,
    WithLength=2,
    Fixed32=5
};

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
    Dataunit,
    DateTime,
    Date,
    Time,
    DateRange,

    Custom=256
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
                                TypeId==ValueType::Int64 ||
                                TypeId==ValueType::UInt8 ||
                                TypeId==ValueType::UInt16 ||
                                TypeId==ValueType::UInt32 ||
                                TypeId==ValueType::UInt64 ||
                                TypeId==ValueType::UInt8 ||
                                TypeId==ValueType::UInt16 ||
                                TypeId==ValueType::UInt32 ||
                                TypeId==ValueType::UInt64
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

template <ValueType TypeId>
constexpr auto IsScalarNotBool=hana::or_(IsInt<TypeId>,IsDouble<TypeId>);

template <ValueType TypeId>
constexpr auto IsString=hana::bool_c<
    TypeId==ValueType::String
    >;

template <ValueType TypeId>
constexpr auto IsDataunit=hana::bool_c<
    TypeId==ValueType::Dataunit
    >;
}

HATN_DATAUNIT_NAMESPACE_END

HATN_DATAUNIT_META_NAMESPACE_BEGIN

template <typename Type>
constexpr auto is_custom_type()
{
    auto check=[](auto x) -> decltype(hana::bool_c<decltype(x)::type::CustomType::value>)
    {
        return hana::bool_c<decltype(x)::type::CustomType::value>;
    };

    auto ok=hana::sfinae(check)(hana::type_c<Type>);
    return hana::equal(ok,hana::just(hana::true_c));
}

HATN_DATAUNIT_META_NAMESPACE_END

#endif // HATNFIELDVALUETYPES_H
