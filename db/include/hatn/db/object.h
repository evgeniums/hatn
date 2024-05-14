/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/object.h
  *
  * Contains definition of base db object.
  *
  */

/****************************************************************************/

#ifndef HATNDBOBJECT_H
#define HATNDBOBJECT_H

#include <hatn/common/error.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

HDU_UNIT(object,
    HDU_FIELD(_id,HDU_TYPE_FIXED_STRING(20),1)
    HDU_FIELD(created_at,TYPE_UINT32,2)
    HDU_FIELD(updated_at,TYPE_UINT32,3)
)

HATN_DB_NAMESPACE_END

#endif // HATNDBOBJECT_H
