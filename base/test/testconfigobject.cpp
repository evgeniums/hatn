/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/test/testconfigobject.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#define HDU_DATAUNIT_EXPORT
#define HATN_WITH_STATIC_ALLOCATOR_INLINE HATN_WITH_STATIC_ALLOCATOR_INLINE_H

#include <hatn/dataunit/valuetypes.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>

#include <hatn/base/configtree.h>
#include <hatn/base/configobject.h>

HATN_USING
HATN_COMMON_USING
HATN_BASE_USING
HATN_TEST_USING

namespace {

HDU_DATAUNIT(config1,
    HDU_FIELD(field1,TYPE_INT32,1)
)
HDU_INSTANTIATE_DATAUNIT(config1)

struct WithConfig1 : public ConfigObject<config1::type>
{
};

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestConfigObject)

BOOST_AUTO_TEST_CASE(LoadConfigScalar1)
{
    static_assert(dataunit::types::IsScalar<dataunit::ValueType::Int8>.value,"Must be scalar type");

    ConfigTree t1;
    t1.set("foo.config1.field1",100);

    WithConfig1 o1;
    auto ec=o1.loadConfig(t1,"foo.config1");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(100,o1.config().fieldValue(config1::field1));
}

BOOST_AUTO_TEST_SUITE_END()
