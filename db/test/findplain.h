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

#include "models9.h"

void registerModels()
{
    registerModels9();
}

#define ModelRef m9()

#define FieldInt8 u9::f2
#define FieldInt16 u9::f3
#define FieldInt32 u9::f4
#define FieldInt64 u9::f5

#define FieldUInt8 u9::f6
#define FieldUInt16 u9::f7
#define FieldUInt32 u9::f8
#define FieldUInt64 u9::f9

#define FieldString u9::f10
#define FieldFixedString u9::f12

#define FieldDateTime u9::f13
#define FieldDate u9::f14
#define FieldTime u9::f15
#define FieldDateRange u9::f17

#define FieldObjectId u9::f16

#define FieldBool u9::f1
#define FieldEnum u9::f11

#define IdxInt8 u9_f2_idx()
#define IdxInt16 u9_f3_idx()
#define IdxInt32 u9_f4_idx()
#define IdxInt64 u9_f5_idx()

#define IdxUInt8 u9_f6_idx()
#define IdxUInt16 u9_f7_idx()
#define IdxUInt32 u9_f8_idx()
#define IdxUInt64 u9_f9_idx()

#define IdxString u9_f10_idx()
#define IdxFixedString u9_f12_idx()

#define IdxDateTime u9_f13_idx()
#define IdxDate u9_f14_idx()
#define IdxTime u9_f15_idx()
#define IdxDateRange u9_f17_idx()

#define IdxObjectId u9_f16_idx()

#define IdxBool u9_f1_idx()
#define IdxEnum u9_f11_idx()

#endif // HATNDBTESTFINDPLAIN_H
