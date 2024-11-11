/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/models10.h
 *
 *     Models declarations.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTMODELS10_H
#define HATNDBTESTMODELS10_H

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

HDU_UNIT_WITH(u3,(HDU_BASE(object)),
              HDU_ENUM(MyEnum,One=1,Two=2)
              // HDU_REPEATED_FIELD(f1,TYPE_BOOL,1)
              HDU_REPEATED_FIELD(f2,TYPE_INT8,2)
              HDU_REPEATED_FIELD(f3,TYPE_INT16,3)
              HDU_REPEATED_FIELD(f4,TYPE_INT32,4)
              HDU_REPEATED_FIELD(f5,TYPE_INT64,5)
              HDU_REPEATED_FIELD(f6,TYPE_UINT8,6)
              HDU_REPEATED_FIELD(f7,TYPE_UINT16,7)
              HDU_REPEATED_FIELD(f8,TYPE_UINT32,8)
              HDU_REPEATED_FIELD(f9,TYPE_UINT64,9)
              HDU_REPEATED_FIELD(f10,TYPE_STRING,10)
              HDU_REPEATED_FIELD(f11,HDU_TYPE_ENUM(MyEnum),11)
              HDU_REPEATED_FIELD(f12,HDU_TYPE_FIXED_STRING(8),12)
              // HDU_REPEATED_FIELD(f13,TYPE_DATETIME,13)
              // HDU_REPEATED_FIELD(f14,TYPE_DATE,14)
              // HDU_REPEATED_FIELD(f15,TYPE_TIME,15)
              // HDU_REPEATED_FIELD(f16,TYPE_OBJECT_ID,16)
              // HDU_REPEATED_FIELD(f17,TYPE_DATE_RANGE,17)
              HDU_REPEATED_FIELD(f18,TYPE_BYTES,18)
              HDU_REPEATED_FIELD(f19,TYPE_FLOAT,19)
              HDU_REPEATED_FIELD(f20,TYPE_DOUBLE,20)
              )

//! @todo Fix repeated bool field

// HATN_DB_INDEX(u3_f1_idx,u3::f1)
HATN_DB_INDEX(u3_f2_idx,u3::f2)
HATN_DB_INDEX(u3_f3_idx,u3::f3)
HATN_DB_INDEX(u3_f4_idx,u3::f4)
HATN_DB_INDEX(u3_f5_idx,u3::f5)
HATN_DB_INDEX(u3_f6_idx,u3::f6)
HATN_DB_INDEX(u3_f7_idx,u3::f7)
HATN_DB_INDEX(u3_f8_idx,u3::f8)
HATN_DB_INDEX(u3_f9_idx,u3::f9)
HATN_DB_INDEX(u3_f10_idx,u3::f10)
HATN_DB_INDEX(u3_f11_idx,u3::f11)
HATN_DB_INDEX(u3_f12_idx,u3::f12)

HATN_DB_MODEL(m3,
              u3,
              // u3_f1_idx(),
              u3_f2_idx()
              ,
          u3_f3_idx(),
          u3_f4_idx(),
          u3_f5_idx(),
          u3_f6_idx(),
          u3_f7_idx(),
          u3_f8_idx(),
          u3_f9_idx()
          ,u3_f10_idx()
          ,u3_f11_idx()
          ,u3_f12_idx()
)

void registerModels10();

#endif // HATNDBTESTMODELS10_H
