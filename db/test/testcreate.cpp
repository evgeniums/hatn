/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/test/testcreate.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include "initdbplugins.h"
#include "preparedb.h"

HATN_USING
HATN_DB_USING
HATN_TEST_USING

BOOST_AUTO_TEST_SUITE(DbTestCrud, *boost::unit_test::fixture<DbTestFixture>())

BOOST_AUTO_TEST_CASE(PrepareDb)
{
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
