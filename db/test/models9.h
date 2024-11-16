/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/models9.h
 *
 *     Models declarations.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTMODELS9_H
#define HATNDBTESTMODELS9_H

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

HDU_UNIT_WITH(u9,(HDU_BASE(object)),
              HDU_ENUM(MyEnum,One=1,Two=2)
              HDU_FIELD(f1,TYPE_BOOL,1)
              HDU_FIELD(f2,TYPE_INT8,2)
              HDU_FIELD(f3,TYPE_INT16,3)
              HDU_FIELD(f4,TYPE_INT32,4)
              HDU_FIELD(f5,TYPE_INT64,5)
              HDU_FIELD(f6,TYPE_UINT8,6)
              HDU_FIELD(f7,TYPE_UINT16,7)
              HDU_FIELD(f8,TYPE_UINT32,8)
              HDU_FIELD(f9,TYPE_UINT64,9)
              HDU_FIELD(f10,TYPE_STRING,10)
              HDU_DEFAULT_FIELD(f11,HDU_TYPE_ENUM(MyEnum),11,MyEnum::Two)
              HDU_FIELD(f12,HDU_TYPE_FIXED_STRING(8),12)
              HDU_FIELD(f13,TYPE_DATETIME,13)
              HDU_FIELD(f14,TYPE_DATE,14)
              HDU_FIELD(f15,TYPE_TIME,15)
              HDU_FIELD(f16,TYPE_OBJECT_ID,16)
              HDU_FIELD(f17,TYPE_DATE_RANGE,17)
              HDU_FIELD(f18,TYPE_BYTES,18)
              HDU_FIELD(f19,TYPE_FLOAT,19)
              HDU_FIELD(f20,TYPE_DOUBLE,20)
              )

HATN_DB_INDEX(u9_f1_idx,u9::f1)
HATN_DB_INDEX(u9_f2_idx,u9::f2)
HATN_DB_INDEX(u9_f3_idx,u9::f3)
HATN_DB_INDEX(u9_f4_idx,u9::f4)
HATN_DB_INDEX(u9_f5_idx,u9::f5)
HATN_DB_INDEX(u9_f6_idx,u9::f6)
HATN_DB_INDEX(u9_f7_idx,u9::f7)
HATN_DB_INDEX(u9_f8_idx,u9::f8)
HATN_DB_INDEX(u9_f9_idx,u9::f9)
HATN_DB_INDEX(u9_f10_idx,u9::f10)

HATN_DB_MODEL(m9,u9,u9_f1_idx()
          ,
          u9_f2_idx(),
          u9_f3_idx(),
          u9_f4_idx(),
          u9_f5_idx(),
          u9_f6_idx(),
          u9_f7_idx(),
          u9_f8_idx(),
          u9_f9_idx(),
          u9_f10_idx()
)

void registerModels9();

#endif // HATNDBTESTMODELS9_H
