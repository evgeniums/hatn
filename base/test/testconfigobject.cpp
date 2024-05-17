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

#include <hatn/validator/validator.hpp>

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
namespace vld=HATN_VALIDATOR_NAMESPACE;

namespace {

HDU_UNIT(config1,
    HDU_FIELD(field1,TYPE_INT32,1)
)

HDU_UNIT(config2,
         HDU_FIELD(field2,TYPE_STRING,2)
         HDU_FIELD(field3,HDU_TYPE_FIXED_STRING(16),3)
         )

HDU_UNIT(config3,
         HDU_REPEATED_FIELD(field3,TYPE_INT64,3)
         )

HDU_UNIT(config4,
         HDU_REPEATED_FIELD(field3,TYPE_STRING,3)
         HDU_REPEATED_FIELD(field4,HDU_TYPE_FIXED_STRING(16),4)
         )

HDU_UNIT(config5,
         HDU_FIELD(field1,TYPE_INT32,1,false,5000)
         HDU_FIELD(field2,TYPE_STRING,2,false,"Hello world!")
         HDU_FIELD(field3,HDU_TYPE_FIXED_STRING(16),3,false,"Hi!")
         )

HDU_UNIT(config6,
         HDU_FIELD(field6,TYPE_UINT32,6,true)
         )

struct WithConfig1 : public ConfigObject<config1::type>
{
};

struct WithConfig2 : public ConfigObject<config2::type>
{
};

struct WithConfig3 : public ConfigObject<config3::type>
{
};

struct WithConfig4 : public ConfigObject<config4::type>
{
};

struct WithConfig5 : public ConfigObject<config5::type>
{
};

struct WithConfig6 : public ConfigObject<config6::type>
{
};

auto v1=vld::validator(
        vld::_[config2::field2](vld::eq,"hello")
    );

} // anonymous namespace

HDU_INSTANTIATE(config1)

BOOST_AUTO_TEST_SUITE(TestConfigObject)

BOOST_AUTO_TEST_CASE(LoadConfigPlain)
{
    static_assert(dataunit::types::IsScalar<dataunit::ValueType::Int8>.value,"Must be scalar type");

    ConfigTree t1;
    t1.set("foo.config1.field1",100);
    t1.set("foo.config2.field2","hello");
    t1.set("foo.config2.field3","hi");

    WithConfig1 o1;
    auto ec=o1.loadConfig(t1,"foo.config1");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(100,o1.config().fieldValue(config1::field1));

    WithConfig2 o2;
    ec=o2.loadConfig(t1,"foo.config2");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL("hello",o2.config().field(config2::field2).c_str());
    BOOST_CHECK_EQUAL("hi",o2.config().field(config2::field3).c_str());
}

BOOST_AUTO_TEST_CASE(LoadConfigDefault)
{
    ConfigTree t1;

    WithConfig5 o1;
    auto ec=o1.loadConfig(t1,"foo.config5");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(5000,o1.config().fieldValue(config5::field1));
    BOOST_CHECK_EQUAL("Hello world!",o1.config().field(config5::field2).c_str());
    BOOST_CHECK_EQUAL("Hi!",o1.config().field(config5::field3).c_str());

    t1.set("foo.config5.field1",100);
    t1.set("foo.config5.field2","hello");
    t1.set("foo.config5.field3","hi");

    ec=o1.loadConfig(t1,"foo.config5");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(100,o1.config().fieldValue(config5::field1));
    BOOST_CHECK_EQUAL("hello",o1.config().field(config5::field2).c_str());
    BOOST_CHECK_EQUAL("hi",o1.config().field(config5::field3).c_str());
}

BOOST_AUTO_TEST_CASE(LoadConfigErrors)
{
    ConfigTree t1;
    t1.set("foo.config1.field1","hello");

    WithConfig1 o1;
    auto ec=o1.loadConfig(t1,"foo.config1");
    BOOST_CHECK(ec);
    BOOST_CHECK_EQUAL(ec.message(),"failed to load configuration object: config1 at path \"foo.config1\": parameter \"field1\": invalid type");

    WithConfig2 o2;
    ec=o2.loadConfig(t1,"foo.config2");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(nullptr,o2.config().field(config2::field2).c_str());
}

BOOST_AUTO_TEST_CASE(LoadConfigValidate)
{
    ConfigTree t1;

    WithConfig2 o1;
    auto ec=o1.loadConfig(t1,"foo.config2",v1);
    BOOST_CHECK(ec);
    BOOST_CHECK_EQUAL(ec.message(),"failed to validate configuration object: config2 at path \"foo.config2\": field2 must be equal to hello");

    t1.set("foo.config2.field2","hello");

    WithConfig2 o2;
    ec=o2.loadConfig(t1,"foo.config2",v1);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL("hello",o2.config().field(config2::field2).c_str());
}

BOOST_AUTO_TEST_CASE(LoadConfigScalarArray)
{
    ConfigTree t1;

    WithConfig3 o1;
    const auto& arr=o1.config().field(config3::field3);

    auto ec=o1.loadConfig(t1,"foo.config3");
    BOOST_CHECK(!ec);
    BOOST_CHECK(arr.empty());

    auto a1=t1.toArray<int64_t>("foo.config3.field3");
    BOOST_CHECK(!a1);
    a1->emplaceBack(100);
    a1->emplaceBack(200);
    a1->emplaceBack(300);
    a1->emplaceBack(400);
    a1->emplaceBack(500);
    ec=o1.loadConfig(t1,"foo.config3");
    BOOST_CHECK(!ec);
    BOOST_CHECK(!arr.empty());
    BOOST_REQUIRE_EQUAL(arr.count(),5);
    BOOST_CHECK_EQUAL(arr.at(0),100);
    BOOST_CHECK_EQUAL(arr.at(1),200);
    BOOST_CHECK_EQUAL(arr.at(2),300);
    BOOST_CHECK_EQUAL(arr.at(3),400);
    BOOST_CHECK_EQUAL(arr.at(4),500);

    a1->clear();
    ec=o1.loadConfig(t1,"foo.config3");
    BOOST_CHECK(!ec);
    BOOST_CHECK(arr.empty());
    BOOST_REQUIRE_EQUAL(arr.count(),0);
}

BOOST_AUTO_TEST_CASE(LoadConfigStringArrays)
{
    ConfigTree t1;

    WithConfig4 o1;
    const auto& arr=o1.config().field(config4::field3);
    auto ec=o1.loadConfig(t1,"foo.config4");
    BOOST_CHECK(!ec);
    BOOST_CHECK(arr.empty());

    auto a1=t1.toArray<std::string>("foo.config4.field3");
    BOOST_CHECK(!a1);
    a1->emplaceBack("one");
    a1->emplaceBack("two");
    a1->emplaceBack("three");
    a1->emplaceBack("four");
    a1->emplaceBack("five");
    ec=o1.loadConfig(t1,"foo.config4");
    BOOST_CHECK(!ec);
    BOOST_CHECK(!arr.empty());
    BOOST_REQUIRE_EQUAL(arr.count(),5);
    BOOST_CHECK_EQUAL(arr.at(0).c_str(),"one");
    BOOST_CHECK_EQUAL(arr.at(1).c_str(),"two");
    BOOST_CHECK_EQUAL(arr.at(2).c_str(),"three");
    BOOST_CHECK_EQUAL(arr.at(3).c_str(),"four");
    BOOST_CHECK_EQUAL(arr.at(4).c_str(),"five");

    a1->clear();
    ec=o1.loadConfig(t1,"foo.config4");
    BOOST_CHECK(!ec);
    BOOST_CHECK(arr.empty());
    BOOST_REQUIRE_EQUAL(arr.count(),0);

    const auto& arr2=o1.config().field(config4::field4);
    ec=o1.loadConfig(t1,"foo.config4");
    BOOST_CHECK(!ec);
    BOOST_CHECK(arr2.empty());

    auto a2=t1.toArray<std::string>("foo.config4.field4");
    BOOST_CHECK(!a2);
    a2->emplaceBack("one-fixed");
    a2->emplaceBack("two-fixed");
    a2->emplaceBack("three-fixed");
    a2->emplaceBack("four-fixed");
    a2->emplaceBack("five-fixed");
    ec=o1.loadConfig(t1,"foo.config4");
    BOOST_CHECK(!ec);
    BOOST_CHECK(!arr2.empty());
    BOOST_REQUIRE_EQUAL(arr2.count(),5);
    BOOST_CHECK_EQUAL(arr2.at(0).c_str(),"one-fixed");
    BOOST_CHECK_EQUAL(arr2.at(1).c_str(),"two-fixed");
    BOOST_CHECK_EQUAL(arr2.at(2).c_str(),"three-fixed");
    BOOST_CHECK_EQUAL(arr2.at(3).c_str(),"four-fixed");
    BOOST_CHECK_EQUAL(arr2.at(4).c_str(),"five-fixed");

    a2->clear();
    ec=o1.loadConfig(t1,"foo.config4");
    BOOST_CHECK(!ec);
    BOOST_CHECK(arr2.empty());
    BOOST_REQUIRE_EQUAL(arr2.count(),0);
}

BOOST_AUTO_TEST_CASE(LoadConfigRequired)
{
    ConfigTree t1;

    WithConfig6 o1;
    auto ec=o1.loadConfig(t1,"foo.config6");
    BOOST_CHECK(ec);
    BOOST_CHECK_EQUAL(ec.message(),"failed to validate configuration object: config6 at path \"foo.config6\": required parameter \"field6\" not set");

    t1.set("foo.config6.field6",500);

    WithConfig6 o2;
    ec=o2.loadConfig(t1,"foo.config6");
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(500,o2.config().fieldValue(config6::field6));
}

BOOST_AUTO_TEST_CASE(LoadLogConfigPlain)
{
    ConfigTree t1;
    t1.set("foo.config1.field1",100);
    t1.set("foo.config1.field2","hello");
    t1.set("foo.config1.field3","hi");

    config_object::LogRecords records;
    WithConfig5 o1;
    auto r1=o1.loadLogConfig(t1,"foo.config1",records);
    BOOST_CHECK(!r1);
    BOOST_CHECK_EQUAL(100,o1.config().fieldValue(config5::field1));
    BOOST_CHECK_EQUAL("hello",o1.config().field(config5::field2).c_str());
    BOOST_CHECK_EQUAL("hi",o1.config().field(config5::field3).c_str());

    BOOST_REQUIRE_EQUAL(records.size(),3);
    BOOST_CHECK_EQUAL(records[0].name,"field1");
    BOOST_CHECK_EQUAL(records[0].value,"100");
    BOOST_CHECK_EQUAL(records[1].name,"field2");
    BOOST_CHECK_EQUAL(records[1].value,"\"hello\"");
    BOOST_CHECK_EQUAL(records[2].name,"field3");
    BOOST_CHECK_EQUAL(records[2].value,"\"hi\"");

    records.clear();
    config_object::LogSettings settings1;
    settings1.Mask="$$$$$";
    settings1.MaskNames.insert("field2");
    r1=o1.loadLogConfig(t1,"foo.config1",records,settings1);
    BOOST_CHECK(!r1);
    BOOST_CHECK_EQUAL(100,o1.config().fieldValue(config5::field1));
    BOOST_CHECK_EQUAL("hello",o1.config().field(config5::field2).c_str());
    BOOST_CHECK_EQUAL("hi",o1.config().field(config5::field3).c_str());

    BOOST_REQUIRE_EQUAL(records.size(),3);
    BOOST_CHECK_EQUAL(records[0].name,"field1");
    BOOST_CHECK_EQUAL(records[0].value,"100");
    BOOST_CHECK_EQUAL(records[1].name,"field2");
    BOOST_CHECK_EQUAL(records[1].value,"\"$$$$$\"");
    BOOST_CHECK_EQUAL(records[2].name,"field3");
    BOOST_CHECK_EQUAL(records[2].value,"\"hi\"");
}

BOOST_AUTO_TEST_CASE(LoadLogConfigScalarArray)
{
    ConfigTree t1;

    auto a1=t1.toArray<int64_t>("foo.config3.field3");
    BOOST_CHECK(!a1);
    a1->emplaceBack(100);
    a1->emplaceBack(200);
    a1->emplaceBack(300);
    a1->emplaceBack(400);
    a1->emplaceBack(500);

    config_object::LogRecords records;
    WithConfig3 o1;
    const auto& arr=o1.config().field(config3::field3);
    auto ec=o1.loadLogConfig(t1,"foo.config3",records);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!arr.empty());
    BOOST_REQUIRE_EQUAL(arr.count(),5);
    BOOST_CHECK_EQUAL(arr.at(0),100);
    BOOST_CHECK_EQUAL(arr.at(1),200);
    BOOST_CHECK_EQUAL(arr.at(2),300);
    BOOST_CHECK_EQUAL(arr.at(3),400);
    BOOST_CHECK_EQUAL(arr.at(4),500);

    BOOST_REQUIRE_EQUAL(records.size(),1);
    BOOST_CHECK_EQUAL(records[0].name,"field3");
    BOOST_CHECK_EQUAL(records[0].value,"[100,200,300,400,500]");

    records.clear();
    config_object::LogSettings settings1;
    settings1.MaxArrayElements=3;
    ec=o1.loadLogConfig(t1,"foo.config3",records,settings1);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!arr.empty());
    BOOST_REQUIRE_EQUAL(arr.count(),5);
    BOOST_CHECK_EQUAL(arr.at(0),100);
    BOOST_CHECK_EQUAL(arr.at(1),200);
    BOOST_CHECK_EQUAL(arr.at(2),300);
    BOOST_CHECK_EQUAL(arr.at(3),400);
    BOOST_CHECK_EQUAL(arr.at(4),500);

    BOOST_REQUIRE_EQUAL(records.size(),1);
    BOOST_CHECK_EQUAL(records[0].name,"field3");
    BOOST_CHECK_EQUAL(records[0].value,"[100,200,300,...]");
}

BOOST_AUTO_TEST_CASE(LoadLogConfigStringArrays)
{
    ConfigTree t1;

    WithConfig4 o1;
    const auto& arr=o1.config().field(config4::field3);
    auto ec=o1.loadConfig(t1,"foo.config4");
    BOOST_CHECK(!ec);
    BOOST_CHECK(arr.empty());

    auto a1=t1.toArray<std::string>("foo.config4.field3");
    BOOST_CHECK(!a1);
    a1->emplaceBack("one");
    a1->emplaceBack("two");
    a1->emplaceBack("three");
    a1->emplaceBack("four");
    a1->emplaceBack("five");

    config_object::LogRecords records;
    ec=o1.loadLogConfig(t1,"foo.config4",records);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!arr.empty());
    BOOST_REQUIRE_EQUAL(arr.count(),5);
    BOOST_CHECK_EQUAL(arr.at(0).c_str(),"one");
    BOOST_CHECK_EQUAL(arr.at(1).c_str(),"two");
    BOOST_CHECK_EQUAL(arr.at(2).c_str(),"three");
    BOOST_CHECK_EQUAL(arr.at(3).c_str(),"four");
    BOOST_CHECK_EQUAL(arr.at(4).c_str(),"five");

    BOOST_REQUIRE_EQUAL(records.size(),2);
    BOOST_CHECK_EQUAL(records[0].name,"field3");
    BOOST_CHECK_EQUAL(records[0].value,"[\"one\",\"two\",\"three\",\"four\",\"five\"]");
    BOOST_CHECK_EQUAL(records[1].name,"field4");
    BOOST_CHECK_EQUAL(records[1].value,"[]");

    records.clear();
    config_object::LogSettings settings1;
    settings1.MaxArrayElements=3;
    ec=o1.loadLogConfig(t1,"foo.config4",records,settings1);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!arr.empty());
    BOOST_REQUIRE_EQUAL(arr.count(),5);
    BOOST_CHECK_EQUAL(arr.at(0).c_str(),"one");
    BOOST_CHECK_EQUAL(arr.at(1).c_str(),"two");
    BOOST_CHECK_EQUAL(arr.at(2).c_str(),"three");
    BOOST_CHECK_EQUAL(arr.at(3).c_str(),"four");
    BOOST_CHECK_EQUAL(arr.at(4).c_str(),"five");

    BOOST_REQUIRE_EQUAL(records.size(),2);
    BOOST_CHECK_EQUAL(records[0].name,"field3");
    BOOST_CHECK_EQUAL(records[0].value,"[\"one\",\"two\",\"three\",...]");
    BOOST_CHECK_EQUAL(records[1].name,"field4");
    BOOST_CHECK_EQUAL(records[1].value,"[]");

    settings1.MaskNames.insert("field3");
    records.clear();
    settings1.MaxArrayElements=3;
    ec=o1.loadLogConfig(t1,"foo.config4",records,settings1);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!arr.empty());
    BOOST_REQUIRE_EQUAL(arr.count(),5);
    BOOST_CHECK_EQUAL(arr.at(0).c_str(),"one");
    BOOST_CHECK_EQUAL(arr.at(1).c_str(),"two");
    BOOST_CHECK_EQUAL(arr.at(2).c_str(),"three");
    BOOST_CHECK_EQUAL(arr.at(3).c_str(),"four");
    BOOST_CHECK_EQUAL(arr.at(4).c_str(),"five");

    BOOST_REQUIRE_EQUAL(records.size(),2);
    BOOST_CHECK_EQUAL(records[0].name,"field3");
    BOOST_CHECK_EQUAL(records[0].value,"[\"********\",\"********\",\"********\",...]");
    BOOST_CHECK_EQUAL(records[1].name,"field4");
    BOOST_CHECK_EQUAL(records[1].value,"[]");
}

BOOST_AUTO_TEST_SUITE_END()
