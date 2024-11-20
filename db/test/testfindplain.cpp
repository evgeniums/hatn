/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testfindplain.cpp
*/

/****************************************************************************/

#include "findplain.h"
#include "finddefs.h"
#include "findhandlers.h"
#include "findhelpers.ipp"

BOOST_AUTO_TEST_SUITE(TestFindPlain, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

#include "findcases.ipp"

BOOST_AUTO_TEST_SUITE_END()
