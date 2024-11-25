/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/findembedded.h
 *
 *     Helpers for TestFindEmbedded.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTFINDEMBEDDED_H
#define HATNDBTESTFINDEMBEDDED_H

#include "modelembedded.h"

void registerModels()
{
    registerModelEmbedded();
}

#define ModelRef modelEmbed()

#define FieldInt8 embed::f1,plain::f2
#define FieldInt16 embed::f1,plain::f3
#define FieldInt32 embed::f1,plain::f4
#define FieldInt64 embed::f1,plain::f5

#define FieldUInt8 embed::f1,plain::f6
#define FieldUInt16 embed::f1,plain::f7
#define FieldUInt32 embed::f1,plain::f8
#define FieldUInt64 embed::f1,plain::f9

#define FieldString embed::f1,plain::f10
#define FieldFixedString embed::f1,plain::f12

#define FieldDateTime embed::f1,plain::f13
#define FieldDate embed::f1,plain::f14
#define FieldTime embed::f1,plain::f15
#define FieldDateRange embed::f1,plain::f17

#define FieldObjectId embed::f1,plain::f16

#define FieldBool embed::f1,plain::f1
#define FieldEnum embed::f1,plain::f11

#define FieldBytes embed::f1,plain::f18

#define FieldFloat embed::f1,plain::f19
#define FieldDouble embed::f1,plain::f20

#define IdxInt8 embed_f2_idx()
#define IdxInt16 embed_f3_idx()
#define IdxInt32 embed_f4_idx()
#define IdxInt64 embed_f5_idx()

#define IdxUInt8 embed_f6_idx()
#define IdxUInt16 embed_f7_idx()
#define IdxUInt32 embed_f8_idx()
#define IdxUInt64 embed_f9_idx()

#define IdxString embed_f10_idx()
#define IdxFixedString embed_f12_idx()

#define IdxDateTime embed_f13_idx()
#define IdxDate embed_f14_idx()
#define IdxTime embed_f15_idx()
#define IdxDateRange embed_f17_idx()

#define IdxObjectId embed_f16_idx()

#define IdxBool embed_f1_idx()
#define IdxEnum embed_f11_idx()

#endif // HATNDBTESTFINDPLAIN_H
