/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/modelcompund2.h
 *
 *     Declaration of model with compound indexes.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTMODELCOMPUND_H
#define HATNDBTESTMODELCOMPUND_H

#include <hatn/dataunit/syntax.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/db/object.h>
#include <hatn/db/model.h>

#include "hatn_test_config.h"
#include "modelplain.h"

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

HDU_UNIT_WITH(comp,(HDU_BASE(plain)),
    HDU_FIELD(ext1,TYPE_STRING,90)
)

HATN_DB_INDEX(comp_f1_idx,comp::ext1,plain::f1)
HATN_DB_INDEX(comp_f2_idx,comp::ext1,plain::f2)
HATN_DB_INDEX(comp_f3_idx,comp::ext1,plain::f3)
HATN_DB_INDEX(comp_f4_idx,comp::ext1,plain::f4)
HATN_DB_INDEX(comp_f5_idx,comp::ext1,plain::f5)
HATN_DB_INDEX(comp_f6_idx,comp::ext1,plain::f6)
HATN_DB_INDEX(comp_f7_idx,comp::ext1,plain::f7)
HATN_DB_INDEX(comp_f8_idx,comp::ext1,plain::f8)
HATN_DB_INDEX(comp_f9_idx,comp::ext1,plain::f9)

HATN_DB_INDEX(comp_f10_idx,comp::ext1,plain::f10)
HATN_DB_INDEX(comp_f11_idx,comp::ext1,plain::f11)
HATN_DB_INDEX(comp_f12_idx,comp::ext1,plain::f12)

HATN_DB_INDEX(comp_f13_idx,comp::ext1,plain::f13)
HATN_DB_INDEX(comp_f14_idx,comp::ext1,plain::f14)
HATN_DB_INDEX(comp_f15_idx,comp::ext1,plain::f15)
HATN_DB_INDEX(comp_f16_idx,comp::ext1,plain::f16)
HATN_DB_INDEX(comp_f17_idx,comp::ext1,plain::f17)

HATN_DB_MODEL(modelComp,comp,
          comp_f1_idx(),
          comp_f2_idx(),
          comp_f3_idx(),
          comp_f4_idx(),
          comp_f5_idx(),
          comp_f6_idx(),
          comp_f7_idx(),
          comp_f8_idx(),
          comp_f9_idx(),
          comp_f10_idx()
          ,comp_f11_idx()
          ,comp_f12_idx()
          ,comp_f13_idx()
          ,comp_f14_idx()
          ,comp_f15_idx()
          ,comp_f16_idx()
          ,comp_f17_idx()
)

void registerModelComp2();

#endif // HATNDBTESTMODELCOMPUND_H
