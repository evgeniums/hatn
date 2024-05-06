/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file base/test/testconfigobject.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/dataunit/valuetypes.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>
#include <hatn/dataunit/detail/wirebuf.ipp>

#include <hatn/base/baseerror.h>
#include <hatn/base/configtree.h>
#include <hatn/base/configobject.h>

HATN_USING
HATN_COMMON_USING
HATN_BASE_USING
HATN_TEST_USING

namespace {

HDU_UNIT(config1,
    HDU_FIELD(field1,TYPE_INT32,1)
)

HDU_UNIT(config2,
         HDU_FIELD(field2,TYPE_STRING,2)
         )

struct WithConfig1 : public ConfigObject<config1::type>
{
};

struct WithConfig2 : public ConfigObject<config2::type>
{
};

} // anonymous namespace

HDU_INSTANTIATE(config1)

BOOST_AUTO_TEST_SUITE(TestConfigObject)

BOOST_AUTO_TEST_CASE(LoadConfigPlain)
{
    static_assert(dataunit::types::IsScalar<dataunit::ValueType::Int8>.value,"Must be scalar type");

    ConfigTree t1;
    t1.set("foo.config1.field1",100);
    t1.set("foo.config2.field2","hello");

    WithConfig1 o1;
    auto ec=o1.loadConfig(t1,"foo.config1");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(100,o1.config().fieldValue(config1::field1));

    WithConfig2 o2;
    ec=o2.loadConfig(t1,"foo.config2");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL("hello",o2.config().field(config2::field2).c_str());
}

BOOST_AUTO_TEST_CASE(LoadConfigErrors)
{
    ConfigTree t1;
    t1.set("foo.config1.field1","hello");

    WithConfig1 o1;
    auto ec=o1.loadConfig(t1,"foo.config1");
    BOOST_CHECK(ec);
    BOOST_CHECK_EQUAL(ec.message(),"failed to load configuration object: object config1: parameter field1: invalid type");

    WithConfig2 o2;
    ec=o2.loadConfig(t1,"foo.config2");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(nullptr,o2.config().field(config2::field2).c_str());
}

BOOST_AUTO_TEST_SUITE_END()
