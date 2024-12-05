/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/modelembedded.h
 *
 *     Declaration of model with embedded object.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTMODELEMBED_H
#define HATNDBTESTMODELEMBED_H

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

HDU_UNIT_WITH(embed,(HDU_BASE(object)),
    HDU_FIELD(f1,plain::TYPE,1)
)

HATN_DB_INDEX(embed_f1_idx,nested(embed::f1,plain::f1))
HATN_DB_INDEX(embed_f2_idx,nested(embed::f1,plain::f2))
HATN_DB_INDEX(embed_f3_idx,nested(embed::f1,plain::f3))
HATN_DB_INDEX(embed_f4_idx,nested(embed::f1,plain::f4))
HATN_DB_INDEX(embed_f5_idx,nested(embed::f1,plain::f5))
HATN_DB_INDEX(embed_f6_idx,nested(embed::f1,plain::f6))
HATN_DB_INDEX(embed_f7_idx,nested(embed::f1,plain::f7))
HATN_DB_INDEX(embed_f8_idx,nested(embed::f1,plain::f8))
HATN_DB_INDEX(embed_f9_idx,nested(embed::f1,plain::f9))

HATN_DB_INDEX(embed_f10_idx,nested(embed::f1,plain::f10))
HATN_DB_INDEX(embed_f11_idx,nested(embed::f1,plain::f11))
HATN_DB_INDEX(embed_f12_idx,nested(embed::f1,plain::f12))

HATN_DB_INDEX(embed_f13_idx,nested(embed::f1,plain::f13))
HATN_DB_INDEX(embed_f14_idx,nested(embed::f1,plain::f14))
HATN_DB_INDEX(embed_f15_idx,nested(embed::f1,plain::f15))
HATN_DB_INDEX(embed_f16_idx,nested(embed::f1,plain::f16))
HATN_DB_INDEX(embed_f17_idx,nested(embed::f1,plain::f17))

HATN_DB_MODEL(modelEmbed,embed,
          embed_f1_idx(),
          embed_f2_idx(),
          embed_f3_idx(),
          embed_f4_idx(),
          embed_f5_idx(),
          embed_f6_idx(),
          embed_f7_idx(),
          embed_f8_idx(),
          embed_f9_idx(),
          embed_f10_idx()
          ,embed_f11_idx()
          ,embed_f12_idx()
          ,embed_f13_idx()
          ,embed_f14_idx()
          ,embed_f15_idx()
          ,embed_f16_idx()
          ,embed_f17_idx()
)

void registerModelEmbedded();

#endif // HATNDBTESTMODELEMBED_H
