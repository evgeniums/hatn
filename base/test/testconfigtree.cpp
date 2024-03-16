/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/test/testconfigtree.cpp
  *
  *  Hatn Base Library contains common types and helper functions that
  *  are not part of hatncommon library because hatnbase depends on hatnvalidator and hatndataunit.
  *
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/base/configtree.h>

HATN_USING
HATN_COMMON_USING
HATN_BASE_USING
HATN_TEST_USING

BOOST_AUTO_TEST_SUITE(TestConfigTree)

BOOST_AUTO_TEST_CASE(ValueSetGet)
{
    ConfigTreeValue t1;
}

BOOST_AUTO_TEST_SUITE_END()
