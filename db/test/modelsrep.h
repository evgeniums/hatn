/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/modelsrep.h
 *
 *     Models declarations.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTMODELSREP_H
#define HATNDBTESTMODELSREP_H

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

HDU_UNIT_WITH(rep,(HDU_BASE(object)),
              HDU_ENUM(MyEnum,One=1,Two=2)
              HDU_REPEATED_FIELD(f1,TYPE_BOOL,1)
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
              HDU_REPEATED_FIELD(f13,TYPE_DATETIME,13)
              HDU_REPEATED_FIELD(f14,TYPE_DATE,14)
              HDU_REPEATED_FIELD(f15,TYPE_TIME,15)
              HDU_REPEATED_FIELD(f16,TYPE_OBJECT_ID,16)
              HDU_REPEATED_FIELD(f17,TYPE_DATE_RANGE,17)
              HDU_REPEATED_FIELD(f18,TYPE_BYTES,18)
              HDU_REPEATED_FIELD(f19,TYPE_FLOAT,19)
              HDU_REPEATED_FIELD(f20,TYPE_DOUBLE,20)
              )

using TestEnum=rep::MyEnum;

HATN_DB_INDEX(rep_f1_idx,rep::f1)
HATN_DB_INDEX(rep_f2_idx,rep::f2)
HATN_DB_INDEX(rep_f3_idx,rep::f3)
HATN_DB_INDEX(rep_f4_idx,rep::f4)
HATN_DB_INDEX(rep_f5_idx,rep::f5)
HATN_DB_INDEX(rep_f6_idx,rep::f6)
HATN_DB_INDEX(rep_f7_idx,rep::f7)
HATN_DB_INDEX(rep_f8_idx,rep::f8)
HATN_DB_INDEX(rep_f9_idx,rep::f9)
HATN_DB_INDEX(rep_f10_idx,rep::f10)
HATN_DB_INDEX(rep_f11_idx,rep::f11)
HATN_DB_INDEX(rep_f12_idx,rep::f12)

HATN_DB_INDEX(rep_f13_idx,rep::f13)
HATN_DB_INDEX(rep_f14_idx,rep::f14)
HATN_DB_INDEX(rep_f15_idx,rep::f15)
HATN_DB_INDEX(rep_f16_idx,rep::f16)
HATN_DB_INDEX(rep_f17_idx,rep::f17)

HATN_DB_MODEL(modelRep,
             rep
            ,rep_f1_idx()
            ,rep_f2_idx()
            ,rep_f3_idx()
            ,rep_f4_idx()
            ,rep_f5_idx()
            ,rep_f6_idx()
            ,rep_f7_idx()
            ,rep_f8_idx()
            ,rep_f9_idx()
            ,rep_f10_idx()
            ,rep_f11_idx()
            ,rep_f12_idx()
            ,rep_f13_idx()
            ,rep_f14_idx()
            ,rep_f15_idx()
            ,rep_f16_idx()
            ,rep_f17_idx()
)

#define ModelRef modelRep()

#define FieldInt8 rep::f2
#define FieldInt16 rep::f3
#define FieldInt32 rep::f4
#define FieldInt64 rep::f5

#define FieldUInt8 rep::f6
#define FieldUInt16 rep::f7
#define FieldUInt32 rep::f8
#define FieldUInt64 rep::f9

#define FieldString rep::f10
#define FieldFixedString rep::f12

#define FieldDateTime rep::f13
#define FieldDate rep::f14
#define FieldTime rep::f15
#define FieldDateRange rep::f17

#define FieldObjectId rep::f16

#define FieldBool rep::f1
#define FieldEnum rep::f11

#define FieldBytes rep::f18

#define FieldFloat rep::f19
#define FieldDouble rep::f20

#define IdxInt8 rep_f2_idx()
#define IdxInt16 rep_f3_idx()
#define IdxInt32 rep_f4_idx()
#define IdxInt64 rep_f5_idx()

#define IdxUInt8 rep_f6_idx()
#define IdxUInt16 rep_f7_idx()
#define IdxUInt32 rep_f8_idx()
#define IdxUInt64 rep_f9_idx()

#define IdxString rep_f10_idx()
#define IdxFixedString rep_f12_idx()

#define IdxDateTime rep_f13_idx()
#define IdxDate rep_f14_idx()
#define IdxTime rep_f15_idx()
#define IdxDateRange rep_f17_idx()

#define IdxObjectId rep_f16_idx()

#define IdxBool rep_f1_idx()
#define IdxEnum rep_f11_idx()

void registerModels();
void initRocksDb();

#endif // HATNDBTESTMODELSREP_H
