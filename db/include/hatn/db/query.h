/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/query.h
  *
  * Contains declarations of db queries.
  *
  */

/****************************************************************************/

#ifndef HATNDBQUERY_H
#define HATNDBQUERY_H

#include <boost/hana.hpp>

#include <hatn/common/objectid.h>
#include <hatn/common/pmr/pmrtypes.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

namespace query
{

HDU_UNIT(value,
    HDU_FIELD(_int64,TYPE_INT64,1)
    HDU_FIELD(_uint64,TYPE_UINT64,2)
    HDU_FIELD(_float,TYPE_FLOAT,3)
    HDU_FIELD(_double,TYPE_DOUBLE,4)
    HDU_FIELD(_string,TYPE_STRING,5)
)

enum class ValueKind : uint8_t
{
    Value,
    Interval,
    Range
};

enum class IntervalType : uint8_t
{
    Closed,
    Open,
    LeftOpen,
    LeftClosed
};

HDU_UNIT(operand,
    HDU_FIELD(from,value::TYPE,1)
    HDU_FIELD(to,value::TYPE,2)
    HDU_REPEATED_FIELD(range,value::TYPE,3)
    HDU_FIELD(kind,HDU_TYPE_ENUM(ValueKind),4,false,ValueKind::Value)
    HDU_FIELD(interval_type,HDU_TYPE_ENUM(IntervalType),5,false,IntervalType::Closed)
)

enum class Operator : uint8_t
{
    eq,
    gt,
    gte,
    lt,
    lte,
    in,
    between,
    neq
};

enum class Sort : uint8_t
{
    None,
    Asc,
    Desc
};

HDU_UNIT(field_op,
    HDU_FIELD(field_id,TYPE_UINT32,1,true)
    HDU_FIELD(op,HDU_TYPE_ENUM(Operator),2,true)
    HDU_FIELD(value,operand::TYPE,3,true)
    HDU_FIELD(sort,HDU_TYPE_ENUM(Sort),4,false,Sort::None)
)

HDU_UNIT(index_query,
    HDU_FIELD(index_id,TYPE_UINT32,1,true)
    HDU_REPEATED_FIELD(field_ops,field_op::TYPE,2,true)
    HDU_FIELD(limit,TYPE_UINT32,3,false,0)
)

}

// namespace query
// {
//
//     using Value=lib::variant<
//         int64_t,
//         uint64_t,
//         float,
//         double,
//         common::pmr::string
//     >;

//     template <typename FieldT>
//     struct Operand
//     {
//         enum Kind
//         {
//             Value,
//             Interval,
//             Range
//         };

//         using valueT=Value;
//         using intervalT=std::pair<valueT,valueT>;
//         using rangeT=common::pmr::vector<valueT>;

//         using type=lib::variant<valueT,intervalT,rangeT>;
//     };

//     template <typename FieldT>
//     struct Field
//     {
//         using valueT=Operand<FieldT>::type;

//         FieldT field;
//         Operator op;
//         valueT value;
//         Sort sort;
//     };

//     template <typename IndexT>
//     struct Fields
//     {
//         using type=boost::hana::tuple<>;
//     };
// }

// template <typename IndexT>
// struct IndexQuery
// {
//     using fieldsT=typename query::Fields<IndexT>::type;

//     IndexT index;

//     fieldsT fields;
//     uint32_t limit;
// };

HATN_DB_NAMESPACE_END

#endif // HATNDBQUERY_H
