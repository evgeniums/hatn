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

#include <hatn/common/result.h>

HATN_USING
HATN_COMMON_USING
HATN_TEST_USING

struct TestStruct
{
    TestStruct(int val=0) : value(val)
    {
        BOOST_TEST_MESSAGE(fmt::format("TestStruct default constructor {}",value));
    }

    TestStruct(TestStruct&& other):value(other.value)
    {
        other.value=0;
        BOOST_TEST_MESSAGE(fmt::format("TestStruct move constructor {}", value));
    }

    // intentionally deleted copy constructor
    TestStruct(const TestStruct& other)=delete;
    // TestStruct(const TestStruct& other) : value(other.value)
    // {
    //     BOOST_TEST_MESSAGE(fmt::format("TestStruct copy constructor {}", value));
    // }

    TestStruct& operator=(const TestStruct& other)
    {
        if (&other==this)
        {
            return *this;
        }
        value=other.value;
        BOOST_TEST_MESSAGE(fmt::format("TestStruct copy assignment operator {}",value));
        return *this;
    }

    TestStruct& operator=(TestStruct&& other)
    {
        if (&other==this)
        {
            return *this;
        }
        value=other.value;
        other.value=0;
        BOOST_TEST_MESSAGE(fmt::format("TestStruct move assignment operator {}",value));
        return *this;
    }

    ~TestStruct()
    {
        BOOST_TEST_MESSAGE(fmt::format("TestStruct destructor {}",value));
    }

    int value;
};

template <typename T, typename T1=void> struct TestRef
{
    template <typename T2>
    static auto f(T2&& v) -> decltype(auto)
    {
        BOOST_TEST_MESSAGE("TestRef 1");
        return static_cast<T2&&>(v);
    }
};

template <typename T> struct TestRef<T,std::enable_if_t<std::is_lvalue_reference<T>::value>>
{
    template <typename T2>
    static T f(T2&& v)
    {
        BOOST_TEST_MESSAGE("TestRef 2");
        return v;
    }
};


BOOST_AUTO_TEST_SUITE(TestResult)

BOOST_AUTO_TEST_CASE(MoveValue)
{
    TestStruct st{1};
    TestStruct b=TestRef<decltype(st)>::f(std::move(st));
    std::ignore=b;
}

BOOST_AUTO_TEST_CASE(MoveConstReference)
{
    TestStruct st{1};

    const auto& strefb=st;
    const TestStruct& b=TestRef<decltype(strefb)>::f(std::move(strefb));
    std::ignore=b;
}

BOOST_AUTO_TEST_CASE(MoveReference)
{
    TestStruct st{1};

    auto& strefb=st;
    TestStruct& b=TestRef<decltype(strefb)>::f(std::move(strefb));
    std::ignore=b;
}

BOOST_AUTO_TEST_CASE(HoldValue)
{
    auto r1=makeResult(TestStruct{1});
    auto r2=std::move(r1);
    auto val=r2.takeValue();
    BOOST_CHECK_EQUAL(val.value,1);
}

BOOST_AUTO_TEST_CASE(HoldConstReference)
{
    TestStruct st{1};

    const auto& stref=st;
    auto r1=makeResult(stref);
    auto r2=std::move(r1);
    BOOST_CHECK_EQUAL(r2.takeValue().value,1);
}

BOOST_AUTO_TEST_CASE(HoldReference)
{
    TestStruct st{1};

    auto& stref=st;
    auto r1=makeResult(stref);
    auto r2=std::move(r1);
    BOOST_CHECK_EQUAL(r2.takeValue().value,1);
}

BOOST_AUTO_TEST_CASE(HoldValueAccessors)
{
    auto r1=makeResult(TestStruct{1});
    auto& val=r1.value();
    BOOST_CHECK_EQUAL(val.value,1);
    BOOST_CHECK_EQUAL(r1->value,1);
    BOOST_CHECK_EQUAL((*r1).value,1);
}

BOOST_AUTO_TEST_CASE(EmplaceValueAccessors)
{
    auto r1=emplaceResult<TestStruct>(1);
    auto& val=r1.value();
    BOOST_CHECK_EQUAL(val.value,1);
    BOOST_CHECK_EQUAL(r1->value,1);
    BOOST_CHECK_EQUAL((*r1).value,1);
}

BOOST_AUTO_TEST_CASE(HoldConstReferenceAccessors)
{
    TestStruct st{1};

    const auto& stref=st;
    auto r1=makeResult(stref);
    auto&& val=r1.value();
    BOOST_CHECK_EQUAL(val.value,1);
    BOOST_CHECK_EQUAL(r1->value,1);
    BOOST_CHECK_EQUAL((*r1).value,1);
}

BOOST_AUTO_TEST_CASE(HoldReferenceAccessors)
{
    TestStruct st{1};

    auto& stref=st;
    auto r1=makeResult(stref);
    auto&& val=r1.value();
    BOOST_CHECK_EQUAL(val.value,1);
    BOOST_CHECK_EQUAL(r1->value,1);
    BOOST_CHECK_EQUAL((*r1).value,1);
}

//! @todo Add explicit tests for errors. Though, they are implicitly tested in other tests where Result is used.

BOOST_AUTO_TEST_SUITE_END()
