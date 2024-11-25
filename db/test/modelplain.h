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

HDU_UNIT_WITH(plain,(HDU_BASE(object)),
              HDU_ENUM(MyEnum,One=1,Two=2,Three=3)
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
              HDU_FIELD(f12,HDU_TYPE_FIXED_STRING(64),12)
              HDU_FIELD(f13,TYPE_DATETIME,13)
              HDU_FIELD(f14,TYPE_DATE,14)
              HDU_FIELD(f15,TYPE_TIME,15)
              HDU_FIELD(f16,TYPE_OBJECT_ID,16)
              HDU_FIELD(f17,TYPE_DATE_RANGE,17)
              HDU_FIELD(f18,TYPE_BYTES,18)
              HDU_FIELD(f19,TYPE_FLOAT,19)
              HDU_FIELD(f20,TYPE_DOUBLE,20)
              )

using TestEnum=plain::MyEnum;

HATN_DB_INDEX(plain_f1_idx,plain::f1)
HATN_DB_INDEX(plain_f2_idx,plain::f2)
HATN_DB_INDEX(plain_f3_idx,plain::f3)
HATN_DB_INDEX(plain_f4_idx,plain::f4)
HATN_DB_INDEX(plain_f5_idx,plain::f5)
HATN_DB_INDEX(plain_f6_idx,plain::f6)
HATN_DB_INDEX(plain_f7_idx,plain::f7)
HATN_DB_INDEX(plain_f8_idx,plain::f8)
HATN_DB_INDEX(plain_f9_idx,plain::f9)

HATN_DB_INDEX(plain_f10_idx,plain::f10)
HATN_DB_INDEX(plain_f11_idx,plain::f11)
HATN_DB_INDEX(plain_f12_idx,plain::f12)

HATN_DB_INDEX(plain_f13_idx,plain::f13)
HATN_DB_INDEX(plain_f14_idx,plain::f14)
HATN_DB_INDEX(plain_f15_idx,plain::f15)
HATN_DB_INDEX(plain_f16_idx,plain::f16)
HATN_DB_INDEX(plain_f17_idx,plain::f17)

HATN_DB_MODEL(modelPlain,plain,plain_f1_idx()
          ,
          plain_f2_idx(),
          plain_f3_idx(),
          plain_f4_idx(),
          plain_f5_idx(),
          plain_f6_idx(),
          plain_f7_idx(),
          plain_f8_idx(),
          plain_f9_idx(),
          plain_f10_idx()
          ,plain_f11_idx()
          ,plain_f12_idx()
          ,plain_f13_idx()
          ,plain_f14_idx()
          ,plain_f15_idx()
          ,plain_f16_idx()
          ,plain_f17_idx()
)

void registerModelPlain();

#endif // HATNDBTESTMODELS9_H
