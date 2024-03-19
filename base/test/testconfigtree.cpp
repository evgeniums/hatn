/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/test/testconfigtree.cpp
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

BOOST_AUTO_TEST_CASE(ConfigTreePath)
{
    ConfigTree t1;
    const auto& t2=t1;

    std::string path1("one.two.three");
    std::string path2("one.two");
    std::string path3("one");

    // path not set yet
    BOOST_CHECK(!t1.isSet(path1));

    // const path not valid
    auto constSubChild1=t2.get(path1);
    BOOST_CHECK(!constSubChild1.isValid());

    // path not valid, no autocreate
    auto subChild1=t1.get(path1);
    BOOST_CHECK(!subChild1.isValid());

    // autocreate path
    auto subChild2=t1.get(path1,true);
    BOOST_CHECK(subChild2.isValid());

    // intermediate paths mus be set now but target path is not yet
    BOOST_CHECK(!t2.isSet(path1));
    BOOST_CHECK(t2.isSet(path2));
    BOOST_CHECK(t2.isSet(path3));

    // path is valid but not set yet
    auto constSubChild2=t2.get(path1);
    BOOST_CHECK(constSubChild2.isValid());
    BOOST_CHECK(!t2.isSet(path1));

    // set subchild, path is valid and set
    subChild2->set(12345);
    auto constSubChild3=t2.get(path1);
    BOOST_CHECK(constSubChild3.isValid());
    BOOST_CHECK(constSubChild3->isSet());
    BOOST_CHECK(t2.isSet(path1));
    BOOST_CHECK_EQUAL(12345,constSubChild3->as<int32_t>().value());
}

BOOST_AUTO_TEST_SUITE_END()
