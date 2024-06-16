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
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {

    HDU_UNIT(n1,
             HDU_FIELD(f1,TYPE_DATETIME,1)
             )

    HDU_UNIT_WITH(nu1,(HDU_BASE(object)),
                  HDU_FIELD(nf1,n1::TYPE,1)
                  HDU_FIELD(f2,TYPE_UINT32,2)
                  )

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(DbTestCrud, *boost::unit_test::fixture<DbTestFixture>())

BOOST_AUTO_TEST_CASE(PrepareDb)
{
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
