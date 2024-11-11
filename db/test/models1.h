/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/models1.h
 *
 *     Models declarations.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTMODELS1_H
#define HATNDBTESTMODELS1_H

#include <hatn/dataunit/syntax.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/db/object.h>
#include <hatn/db/model.h>

#include "hatn_test_config.h"

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

HATN_TEST_NAMESPACE_BEGIN

HDU_UNIT_WITH(u1_bool,(HDU_BASE(object)),
            HDU_FIELD(f1,TYPE_BOOL,1)
            )
#if 1
HDU_UNIT_WITH(u1_int8,(HDU_BASE(object)),
            HDU_FIELD(f1,TYPE_INT8,1)
            )

HDU_UNIT_WITH(u1_int16,(HDU_BASE(object)),
            HDU_FIELD(f1,TYPE_INT16,1)
            )

HDU_UNIT_WITH(u1_int32,(HDU_BASE(object)),
            HDU_FIELD(f1,TYPE_INT32,1)
            )

HDU_UNIT_WITH(u1_int64,(HDU_BASE(object)),
            HDU_FIELD(f1,TYPE_INT64,1)
            )

HDU_UNIT_WITH(u1_uint8,(HDU_BASE(object)),
              HDU_FIELD(f1,TYPE_UINT8,1)
              )

HDU_UNIT_WITH(u1_uint16,(HDU_BASE(object)),
              HDU_FIELD(f1,TYPE_UINT16,1)
              )

HDU_UNIT_WITH(u1_uint32,(HDU_BASE(object)),
              HDU_FIELD(f1,TYPE_UINT32,1)
              )

HDU_UNIT_WITH(u1_uint64,(HDU_BASE(object)),
              HDU_FIELD(f1,TYPE_UINT64,1)
              )

HDU_UNIT_WITH(u1_enum,(HDU_BASE(object)),
              HDU_ENUM(MyEnum,One=1,Two=2)
              HDU_DEFAULT_FIELD(f1,HDU_TYPE_ENUM(MyEnum),19,MyEnum::Two)
              )

HDU_UNIT_WITH(u1_str,(HDU_BASE(object)),
              HDU_FIELD(f1,TYPE_STRING,1)
              )

HDU_UNIT_WITH(u1_fix_str,(HDU_BASE(object)),
              HDU_FIELD(f1,HDU_TYPE_FIXED_STRING(8),18)
              )

HDU_UNIT_WITH(u1_dt,(HDU_BASE(object)),
              HDU_FIELD(f1,TYPE_DATETIME,1)
              )

HDU_UNIT_WITH(u1_date,(HDU_BASE(object)),
              HDU_FIELD(f1,TYPE_DATE,1)
              )

HDU_UNIT_WITH(u1_time,(HDU_BASE(object)),
              HDU_FIELD(f1,TYPE_TIME,1)
              )

HDU_UNIT_WITH(u1_oid,(HDU_BASE(object)),
              HDU_FIELD(f1,TYPE_OBJECT_ID,1)
              )
#endif

HATN_DB_INDEX(u1_bool_f1_idx,u1_bool::f1)
HATN_DB_MODEL(m1_bool,u1_bool,u1_bool_f1_idx())

HATN_DB_INDEX(u1_int8_f1_idx,u1_int8::f1)
HATN_DB_MODEL(m1_int8,u1_int8,u1_int8_f1_idx())

HATN_DB_INDEX(u1_int16_f1_idx,u1_int16::f1)
HATN_DB_MODEL(m1_int16,u1_int16,u1_int16_f1_idx())

HATN_DB_INDEX(u1_int32_f1_idx,u1_int32::f1)
HATN_DB_MODEL(m1_int32,u1_int32,u1_int32_f1_idx())

HATN_DB_INDEX(u1_int64_f1_idx,u1_int64::f1)
HATN_DB_MODEL(m1_int64,u1_int64,u1_int64_f1_idx())

HATN_DB_INDEX(u1_uint8_f1_idx,u1_uint8::f1)
HATN_DB_MODEL(m1_uint8,u1_uint8,u1_uint8_f1_idx())

HATN_DB_INDEX(u1_uint16_f1_idx,u1_uint16::f1)
HATN_DB_MODEL(m1_uint16,u1_uint16,u1_uint16_f1_idx())

HATN_DB_INDEX(u1_uint32_f1_idx,u1_uint32::f1)
HATN_DB_MODEL(m1_uint32,u1_uint32,u1_uint32_f1_idx())

HATN_DB_INDEX(u1_uint64_f1_idx,u1_uint64::f1)
HATN_DB_MODEL(m1_uint64,u1_uint64,u1_uint64_f1_idx())

HATN_DB_INDEX(u1_enum_f1_idx,u1_enum::f1)
HATN_DB_MODEL(m1_enum,u1_enum,u1_enum_f1_idx())

HATN_DB_INDEX(u1_str_f1_idx,u1_str::f1)
HATN_DB_MODEL(m1_str,u1_str,u1_str_f1_idx())

HATN_DB_INDEX(u1_fix_str_f1_idx,u1_fix_str::f1)
HATN_DB_MODEL(m1_fix_str,u1_fix_str,u1_fix_str_f1_idx())

HATN_DB_INDEX(u1_dt_f1_idx,u1_dt::f1)
HATN_DB_MODEL(m1_dt,u1_dt,u1_dt_f1_idx())

HATN_DB_INDEX(u1_date_f1_idx,u1_date::f1)
HATN_DB_MODEL(m1_date,u1_date,u1_date_f1_idx())

HATN_DB_INDEX(u1_time_f1_idx,u1_time::f1)
HATN_DB_MODEL(m1_time,u1_time,u1_time_f1_idx())

HATN_DB_INDEX(u1_oid_f1_idx,u1_oid::f1)
HATN_DB_MODEL(m1_oid,u1_oid,u1_oid_f1_idx())

HATN_TEST_NAMESPACE_END

#endif // HATNDBTESTMODELS1_H
