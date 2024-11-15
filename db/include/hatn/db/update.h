/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/update.h
  *
  * Contains types for db update operations.
  *
  */

/****************************************************************************/

#ifndef HATNDBUPDATE_H
#define HATNDBUPDATE_H

#include <hatn/common/objectid.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/db/db.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/query.h>

HATN_DB_NAMESPACE_BEGIN

#define HATN_DB_UPDATE_VALUE_TYPES(DO) \
    DO(bool), \
    DO(int8_t), \
    DO(int16_t), \
    DO(int32_t), \
    DO(int64_t), \
    DO(uint8_t), \
    DO(uint16_t), \
    DO(uint32_t), \
    DO(uint64_t), \
    DO(String), \
    DO(common::DateTime), \
    DO(common::Date), \
    DO(common::Time), \
    DO(common::DateRange), \
    DO(ObjectId)

#define HATN_DB_UPDATE_VALUE_TYPE_IDS(DO) \
    DO(Bool), \
    DO(Int8_t), \
    DO(Int16_t), \
    DO(Int32_t), \
    DO(Int64_t), \
    DO(Uint8_t), \
    DO(Uint16_t), \
    DO(Uint32_t), \
    DO(Uint64_t), \
    DO(String), \
    DO(DateTime), \
    DO(Date), \
    DO(Time), \
    DO(DateRange), \
    DO(ObjectId)

#define HATN_DB_UPDATE_VALUE_TYPE(Type) \
    Type, \
    common::pmr::vector<Type>

#define HATN_DB_UPDATE_VALUE_TYPE_ID(Type) \
    Type, \
    Vector##Type

namespace update
{

using String=query::String;

enum class Operator : uint8_t
{
    set,
    unset,
    inc,
    push,
    pop,
    push_unique,
    replace_element,
    erase_element,
    inc_element
};

using ValueVariant=lib::variant<
    HATN_DB_UPDATE_VALUE_TYPES(HATN_DB_UPDATE_VALUE_TYPE)
>;

enum class ValueEnum : uint8_t
{
    HATN_DB_UPDATE_VALUE_TYPE_IDS(HATN_DB_UPDATE_VALUE_TYPE_ID)
};

using Operand=query::ValueT<ValueVariant,ValueEnum>;

using FieldInfo=IndexFieldInfo;

struct Field
{
    //! @todo optimization: Use prepared FieldInfo for often used fields in order not to make it on each constructor call.

    template <typename T>
    Field(
            const FieldInfo& fieldInfo,
            Operator op,
            T&& value
        ) : fieldInfo(&fieldInfo),
            op(op),
            value(std::forward<T>(value))
    {
        checkOperator();
    }

    template <typename T>
    Field(
        FieldInfo&& fieldInfo,
        Operator op,
        T&& value
    ) = delete;

    void checkOperator() const
    {
        //! @todo Check combination of operator and operand
        bool ok=fieldInfo->name()!=ObjectIdFieldName
            &&
            fieldInfo->name()!=CreatedAtFieldName
            &&
            fieldInfo->name()!=UpdatedAtFieldName
            ;
        Assert(ok,"Invalid combination of operator and operand");
    }

    const FieldInfo* fieldInfo;
    Operator op;
    Operand value;
};

using Request=common::pmr::vector<Field>;

enum class ModifyReturn : int
{
    None=0,
    Before=1,
    After=2
};

} // namespace update

HATN_DB_NAMESPACE_END

#endif // HATNDBUPDATE_H
