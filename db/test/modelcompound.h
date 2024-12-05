/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/modelcompund.h
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

HATN_DB_INDEX(comp_f1_idx,plain::f1,comp::ext1)
HATN_DB_INDEX(comp_f2_idx,plain::f2,comp::ext1)
HATN_DB_INDEX(comp_f3_idx,plain::f3,comp::ext1)
HATN_DB_INDEX(comp_f4_idx,plain::f4,comp::ext1)
HATN_DB_INDEX(comp_f5_idx,plain::f5,comp::ext1)
HATN_DB_INDEX(comp_f6_idx,plain::f6,comp::ext1)
HATN_DB_INDEX(comp_f7_idx,plain::f7,comp::ext1)
HATN_DB_INDEX(comp_f8_idx,plain::f8,comp::ext1)
HATN_DB_INDEX(comp_f9_idx,plain::f9,comp::ext1)

HATN_DB_INDEX(comp_f10_idx,plain::f10,comp::ext1)
HATN_DB_INDEX(comp_f11_idx,plain::f11,comp::ext1)
HATN_DB_INDEX(comp_f12_idx,plain::f12,comp::ext1)

HATN_DB_INDEX(comp_f13_idx,plain::f13,comp::ext1)
HATN_DB_INDEX(comp_f14_idx,plain::f14,comp::ext1)
HATN_DB_INDEX(comp_f15_idx,plain::f15,comp::ext1)
HATN_DB_INDEX(comp_f16_idx,plain::f16,comp::ext1)
HATN_DB_INDEX(comp_f17_idx,plain::f17,comp::ext1)

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

void registerModelComp();

#endif // HATNDBTESTMODELCOMPUND_H
