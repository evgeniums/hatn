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

    base::ConfigTreePath path3("one");
    auto path2=path3.copyAppend("two");
    auto path1=path2.copyAppend("three");
    auto path0=path1.copyPrepend("zero");
    auto path4=path0.copyDropFront(2);
    auto path5=path0.copyDropBack(3);
    auto path6=path0.copyDropBack(10);
    auto path7=path0.copyDropFront(10);
    auto path8=path0;
    path8.reset();
    BOOST_CHECK_EQUAL(std::string("zero.one.two.three"),std::string(path0));
    BOOST_CHECK_EQUAL(std::string("one.two.three"),std::string(path1));
    BOOST_CHECK_EQUAL(std::string("one.two"),std::string(path2));
    BOOST_CHECK_EQUAL(std::string("one"),std::string(path3));
    BOOST_CHECK_EQUAL(std::string("two.three"),std::string(path4));
    BOOST_CHECK_EQUAL(std::string("zero"),std::string(path5));
    BOOST_CHECK(path6.isRoot());
    BOOST_CHECK(path7.isRoot());
    BOOST_CHECK(path8.isRoot());
    path5.setPath("five.six.seven");
    BOOST_CHECK_EQUAL(std::string("five.six.seven"),path5.path());

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

    // intermediate paths must be already set but target path is not yet
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

BOOST_AUTO_TEST_CASE(ConfigTreeArray)
{
    ConfigTree t1;
    const auto& t2=t1;

    base::ConfigTreePath path1("one.two.three");
    base::ConfigTreePath path2("one.two.three.1.four.five");

    // path not set yet
    BOOST_CHECK(!t1.isSet(path1));

    // const path not valid
    BOOST_CHECK(!t2.isSet(path1));

    // make array
    auto arr1=t1.toArray<ConfigTree>(path1);
    BOOST_CHECK(t1.isSet("one.two.three"));
    BOOST_CHECK(t2.isSet(common::lib::string_view("one.two.three")));
    const auto& arr2=t2.get(path1)->asArray<ConfigTree>();

    // fill array
    arr1->append(config_tree::makeTree());
    arr1->append(config_tree::makeTree());
    arr1->append(config_tree::makeTree());
    BOOST_CHECK_EQUAL(3,arr2->size());

    // set value with array element in path
    t1.set(path2,112233);

    // check added value
    auto subArr1=t2.get(path2);
    BOOST_CHECK(subArr1.isValid());
    BOOST_CHECK(subArr1->isSet());
    BOOST_CHECK_EQUAL(112233,subArr1->as<int32_t>().value());
}

BOOST_AUTO_TEST_SUITE_END()
