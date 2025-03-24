/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/updateserialization.h
  *
  */

/****************************************************************************/

#ifndef HATNDBUPDATESERIALIZATION_H
#define HATNDBUPDATESERIALIZATION_H

#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>
#include <hatn/db/update.h>

HATN_DB_NAMESPACE_BEGIN

namespace update {

//! @todo Use fixed string for name?

HDU_UNIT(field_id,
    HDU_FIELD(id,TYPE_INT32,1)
    HDU_FIELD(idx,TYPE_UINT32,2)
    HDU_FIELD(field_name,TYPE_STRING,3)
)

HDU_UNIT(a_field,
    HDU_REPEATED_FIELD(path,field_id::TYPE,1,true)
    HDU_FIELD(op,HDU_TYPE_ENUM(Operator),2,true)
    HDU_FIELD(value_type,HDU_TYPE_ENUM(ValueType),3,true)
    HDU_FIELD(scalar,TYPE_BYTES,4)
    HDU_REPEATED_FIELD(vector,TYPE_BYTES,5)
)

HDU_UNIT(message,
    HDU_REPEATED_FIELD(the_fields,a_field::TYPE,1)
)

} // namespace update

HATN_DB_NAMESPACE_END

#endif // HATNDBUPDATESERIALIZATION_H

//! @todo Test update serialization
