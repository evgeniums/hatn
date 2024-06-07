/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testobject.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/test/multithreadfixture.h>

//! @todo Move *.ipp from detail.

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/detail/syntax.ipp>

#include <hatn/db/object.h>
#include <hatn/db/detail/objectid.ipp>

#include "hatn_test_config.h"

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {

HDU_UNIT_WITH(u1,(HDU_BASE(object)),
    HDU_FIELD(f1,TYPE_UINT32,1)
)

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestDbObject)

BOOST_AUTO_TEST_CASE(BaseObjectInit)
{
    object::type o1;

    BOOST_TEST_MESSAGE(fmt::format("original object:\n{}",o1.toString(true)));
    initObject(o1);
    BOOST_TEST_MESSAGE(fmt::format("initialized object:\n{}",o1.toString(true)));

    WireBufSolid buf1;
    Error ec;
    io::serialize(o1,buf1,ec);
    BOOST_REQUIRE(!ec);

    object::type o2;
    io::deserialize(o2,buf1,ec);
    BOOST_REQUIRE(!ec);
    BOOST_TEST_MESSAGE(fmt::format("parsed object:\n{}",o1.toString(true)));
    BOOST_CHECK_EQUAL(o1.toString(),o2.toString());

    auto str1=o1.toString();
    object::type o3;
    auto ok=o3.loadFromJSON(str1);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(o1.toString(),o3.toString());
}

BOOST_AUTO_TEST_CASE(DerivedObjectInit)
{
    u1::type o1;

    BOOST_TEST_MESSAGE(fmt::format("original object:\n{}",o1.toString(true)));
    initObject(o1);
    o1.field(u1::f1).set(12345678);
    BOOST_TEST_MESSAGE(fmt::format("initialized object:\n{}",o1.toString(true)));

    WireBufSolid buf1;
    Error ec;
    io::serialize(o1,buf1,ec);
    BOOST_REQUIRE(!ec);

    u1::type o2;
    io::deserialize(o2,buf1,ec);
    BOOST_REQUIRE(!ec);
    BOOST_TEST_MESSAGE(fmt::format("parsed object:\n{}",o1.toString(true)));
    BOOST_CHECK_EQUAL(o1.toString(),o2.toString());

    auto str1=o1.toString();
    u1::type o3;
    auto ok=o3.loadFromJSON(str1);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(o1.toString(),o3.toString());
}

BOOST_AUTO_TEST_SUITE_END()
