/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testinitplugins.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include "initdbplugins.h"

BOOST_AUTO_TEST_SUITE(PluginsInit, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(DbPluginLoad)
{
    HATN_TEST_NAMESPACE::DbPluginTest::instance().eachPlugin<HATN_TEST_NAMESPACE::DbTestTraits>();
}

BOOST_AUTO_TEST_SUITE_END()

