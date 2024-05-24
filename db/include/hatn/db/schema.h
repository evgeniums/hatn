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

#include <cinttypes>
#include <string>
#include <set>

#include <boost/hana.hpp>

#include <hatn/common/classuid.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>
#include <hatn/db/object.h>

HATN_DB_NAMESPACE_BEGIN

HDU_UNIT_WITH(index,(HDU_BASE(object)),
    HDU_FIELD(model,TYPE_STRING,1)
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_REPEATED_FIELD(field_names,TYPE_UINT32,3)
    HDU_FIELD(unique,TYPE_BOOL,4)
    HDU_FIELD(prefix,TYPE_UINT32,5)
)

struct NestedFieldTag{};

template <typename FieldT, typename SubFieldT>
struct NestedField
{
    using hana_tag=NestedFieldTag;

    FieldT field;
    SubFieldT subField;
};

template <typename ...Fields>
struct Index
{
    std::string name;
    boost::hana::tuple<Fields...> fields;
    bool unique;

    index::type storage;
};

HDU_UNIT(model_field,
    HDU_FIELD(id,TYPE_UINT32,1)
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_FIELD(field_type,TYPE_STRING,3)
    HDU_FIELD(repeated,TYPE_BOOL,4)
    HDU_FIELD(default_flag,TYPE_BOOL,5)
    HDU_FIELD(default_value,TYPE_STRING,6)
)

HDU_UNIT_WITH(model,(HDU_BASE(object)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_REPEATED_FIELD(model_fields,model_field::TYPE,2)
    HDU_FIELD(partitioning,TYPE_STRING,3)
    HDU_REPEATED_FIELD(partition_field_names,TYPE_UINT32,4)
)

template <typename UnitT, typename ...Indexes>
struct Model
{
    boost::hana::tuple<Indexes...> indexes;
    model::type definition;
    UnitT sample;
};

template <typename ...Models>
struct Schema
{
    std::string name;
    boost::hana::tuple<Models...> models;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBSCHEMA_H
