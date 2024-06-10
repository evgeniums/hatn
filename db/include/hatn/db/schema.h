/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/schema.h
  *
  * Contains helpers for database schema definition.
  *
  */

/****************************************************************************/

#ifndef HATNDBSCHEMA_H
#define HATNDBSCHEMA_H

#include <set>

#include <boost/hana.hpp>

#include <hatn/common/datetime.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>
#include <hatn/db/object.h>

HATN_DB_NAMESPACE_BEGIN

HDU_UNIT_WITH(index,(HDU_BASE(object)),
    HDU_FIELD(model,TYPE_STRING,1)
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_REPEATED_FIELD(field_names,TYPE_UINT32,3)
    HDU_FIELD(unique,TYPE_BOOL,4)
    HDU_FIELD(date_partition,TYPE_BOOL,5)
    HDU_FIELD(prefix,TYPE_UINT32,6)
    HDU_FIELD(ttl,TYPE_UINT32,7)
    HDU_FIELD(topic,TYPE_BOOL,8)
)

struct NestedFieldTag{};

template <typename FieldT, typename SubFieldT>
struct NestedField
{
    using hana_tag=NestedFieldTag;

    FieldT field;
    SubFieldT subField;
};

template <typename IndexT, typename ...Fields>
struct Index
{
    constexpr static const boost::hana::tuple<Fields...> fields{};
    constexpr static const hana::false_ datePartitionField{};

    IndexT storage;
};

template <typename Field>
struct DatePartitionIndex : public Index<Field>
{
    constexpr static const Field datePartitionField{};
};

HDU_UNIT(model_field,
    HDU_FIELD(id,TYPE_UINT32,1)
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_FIELD(field_type,TYPE_STRING,3)
    HDU_FIELD(repeated,TYPE_BOOL,4)
    HDU_FIELD(default_flag,TYPE_BOOL,5)
    HDU_FIELD(default_value,TYPE_STRING,6)
)

using DatePartitionMode=common::DateRange::Type;

HDU_UNIT_WITH(model,(HDU_BASE(object)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_REPEATED_FIELD(model_fields,model_field::TYPE,2)
    HDU_FIELD(date_partition_mode,HDU_TYPE_ENUM(DatePartitionMode),3)
    HDU_FIELD(date_partition_field,TYPE_STRING,4)
)

template <typename ModelT, typename UnitT, typename ...Indexes>
struct Model
{
    boost::hana::tuple<Indexes...> indexes;
    ModelT definition;
    UnitT sample;
};

struct datePartitionFieldT
{
    template <typename IndexesT>
    constexpr decltype(auto) operator()(IndexesT&& indexes) const
    {
        hana::find_if(
            std::forward<IndexesT>(indexes),
            [](auto&& index)
            {
                return hana::not_(std::is_same<decltype(std::decay_t<decltype(index)>::datePartitionField),hana::false_>{});
            }
        );
    }
};
constexpr datePartitionFieldT datePartitionField{};

template <typename ...Models>
struct Schema
{
    std::string name;
    boost::hana::tuple<Models...> models;
    std::set<DatePartitionMode> partitionModes;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBSCHEMA_H
