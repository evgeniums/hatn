/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/findplain.h
 *
 *     Helpers for TestFindPlain.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTFINDPLAIN_H
#define HATNDBTESTFINDPLAIN_H

#include "modelplain.h"

void registerModels()
{
    registerModelPlain();
}

#define ModelRef modelPlain()

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

#define FieldBytes plain::f18

#define FieldFloat plain::f19
#define FieldDouble plain::f20

#define IdxInt8 plain_f2_idx()
#define IdxInt16 plain_f3_idx()
#define IdxInt32 plain_f4_idx()
#define IdxInt64 plain_f5_idx()

#define IdxUInt8 plain_f6_idx()
#define IdxUInt16 plain_f7_idx()
#define IdxUInt32 plain_f8_idx()
#define IdxUInt64 plain_f9_idx()

#define IdxString plain_f10_idx()
#define IdxFixedString plain_f12_idx()

#define IdxDateTime plain_f13_idx()
#define IdxDate plain_f14_idx()
#define IdxTime plain_f15_idx()
#define IdxDateRange plain_f17_idx()

#define IdxObjectId plain_f16_idx()

#define IdxBool plain_f1_idx()
#define IdxEnum plain_f11_idx()

#endif // HATNDBTESTFINDPLAIN_H
