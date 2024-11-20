/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/findcompound.h
 *
 *     Helpers for TestFindCompound.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTFINDCOMPOUND_H
#define HATNDBTESTFINDCOMPOUND_H

#include "modelcompound.h"

void registerModels()
{
    registerModelComp();
}

#define ModelRef modelComp()

#define FieldInt8 plain::f2
#define FieldInt16 plain::f3
#define FieldInt32 plain::f4
#define FieldInt64 plain::f5

#define FieldUInt8 plain::f6
#define FieldUInt16 plain::f7
#define FieldUInt32 plain::f8
#define FieldUInt64 plain::f9

#define FieldString plain::f10
#define FieldFixedString plain::f12

#define FieldDateTime plain::f13
#define FieldDate plain::f14
#define FieldTime plain::f15
#define FieldDateRange plain::f17

#define FieldObjectId plain::f16

#define FieldBool plain::f1
#define FieldEnum plain::f11

#define IdxInt8 comp_f2_idx()
#define IdxInt16 comp_f3_idx()
#define IdxInt32 comp_f4_idx()
#define IdxInt64 comp_f5_idx()

#define IdxUInt8 comp_f6_idx()
#define IdxUInt16 comp_f7_idx()
#define IdxUInt32 comp_f8_idx()
#define IdxUInt64 comp_f9_idx()

#define IdxString comp_f10_idx()
#define IdxFixedString comp_f12_idx()

#define IdxDateTime comp_f13_idx()
#define IdxDate comp_f14_idx()
#define IdxTime comp_f15_idx()
#define IdxDateRange comp_f17_idx()

#define IdxObjectId comp_f16_idx()

#define IdxBool comp_f1_idx()
#define IdxEnum comp_f11_idx()

#endif // HATNDBTESTFINDCOMPOUND_H
