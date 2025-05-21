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

#include <hatn/test/multithreadfixture.h>

#include <hatn/validator/validator.hpp>

#include <hatn/dataunit/valuetypes.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/base/baseerror.h>
#include <hatn/base/configtree.h>
#include <hatn/base/configobject.h>
#include <hatn/base/configtreeloader.h>
#include <hatn/base/configtreejson.h>

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

HDU_UNIT(config7,
         HDU_FIELD(key1,TYPE_BOOL,1)
         HDU_FIELD(key2,TYPE_BOOL,2)
         HDU_FIELD(key3,TYPE_BOOL,3)
         HDU_FIELD(key4,TYPE_BOOL,4)
         )

HDU_UNIT(config8,
         HDU_REPEATED_FIELD(obj1,config1::TYPE,1)
         )


HDU_UNIT(config9,
    HDU_FIELD(obj1,config1::TYPE,1)
    HDU_FIELD(f2,TYPE_UINT32,2)
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

struct WithConfig7 : public ConfigObject<config7::type>
{
};

struct WithConfig8 : public ConfigObject<config8::type>
{
};

struct WithConfig9 : public ConfigObject<config9::type>
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
    BOOST_REQUIRE(!a1);
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
    BOOST_REQUIRE(!a1);
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
    BOOST_REQUIRE(!a2);
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
    BOOST_CHECK_EQUAL(records[0].name,"foo.config1.field1");
    BOOST_CHECK_EQUAL(records[0].value,"100");
    BOOST_CHECK_EQUAL(records[1].name,"foo.config1.field2");
    BOOST_CHECK_EQUAL(records[1].value,"hello");
    BOOST_CHECK_EQUAL(records[2].name,"foo.config1.field3");
    BOOST_CHECK_EQUAL(records[2].value,"hi");

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
    BOOST_CHECK_EQUAL(records[0].name,"foo.config1.field1");
    BOOST_CHECK_EQUAL(records[0].value,"100");
    BOOST_CHECK_EQUAL(records[1].name,"foo.config1.field2");
    BOOST_CHECK_EQUAL(records[1].value,"$$$$$");
    BOOST_CHECK_EQUAL(records[2].name,"foo.config1.field3");
    BOOST_CHECK_EQUAL(records[2].value,"hi");
}

BOOST_AUTO_TEST_CASE(LoadLogConfigScalarArray)
{
    ConfigTree t1;

    auto a1=t1.toArray<int64_t>("foo.config3.field3");
    BOOST_REQUIRE(!a1);
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
    BOOST_CHECK_EQUAL(records[0].name,"foo.config3.field3");
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
    BOOST_CHECK_EQUAL(records[0].name,"foo.config3.field3");
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
    BOOST_REQUIRE(!a1);
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
    BOOST_CHECK_EQUAL(records[0].name,"foo.config4.field3");
    BOOST_CHECK_EQUAL(records[0].value,"[\"one\",\"two\",\"three\",\"four\",\"five\"]");
    BOOST_CHECK_EQUAL(records[1].name,"foo.config4.field4");
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
    BOOST_CHECK_EQUAL(records[0].name,"foo.config4.field3");
    BOOST_CHECK_EQUAL(records[0].value,"[\"one\",\"two\",\"three\",...]");
    BOOST_CHECK_EQUAL(records[1].name,"foo.config4.field4");
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
    BOOST_CHECK_EQUAL(records[0].name,"foo.config4.field3");
    BOOST_CHECK_EQUAL(records[0].value,"[\"********\",\"********\",\"********\",...]");
    BOOST_CHECK_EQUAL(records[1].name,"foo.config4.field4");
    BOOST_CHECK_EQUAL(records[1].value,"[]");
}

BOOST_AUTO_TEST_CASE(BoolValues)
{
    ConfigTreeLoader loader;
    ConfigTree t;

    auto filename=MultiThreadFixture::assetsFilePath("base/assets/config_bool.jsonc");
    auto ec=loader.loadFromFile(t,filename);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);
    ConfigTreeJson jsonIo;
    auto jsonR=jsonIo.serialize(t);
    BOOST_CHECK(!jsonR);
    BOOST_TEST_MESSAGE(fmt::format("Config tree: \n{} \n",jsonR.value()));

    WithConfig7 obj;
    ec=obj.loadConfig(t,"obj1");
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    BOOST_TEST_MESSAGE(fmt::format("Config object: \n{} \n",obj.config().toString(true)));
    BOOST_CHECK(obj.config().fieldValue(config7::key1));
    BOOST_CHECK(!obj.config().fieldValue(config7::key2));
    BOOST_CHECK(obj.config().fieldValue(config7::key3));
    BOOST_CHECK(!obj.config().fieldValue(config7::key4));
}

BOOST_AUTO_TEST_CASE(RepeatedUnits)
{
    ConfigTreeLoader loader;
    ConfigTree t1;
    std::string json1="{\"obj1\" : [{\"field1\":100},{\"field1\":200}]}";
    auto ec=loader.loadFromString(t1,json1);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    WithConfig8 o1;
    config_object::LogRecords records;
    ec=o1.loadLogConfig(t1,"",records);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    ConfigTreeJson jsonIo;
    auto jsonR=jsonIo.serialize(t1);
    BOOST_CHECK(!jsonR);
    BOOST_TEST_MESSAGE(fmt::format("Config tree: \n{} \n",jsonR.value()));
    BOOST_TEST_MESSAGE(fmt::format("Nested object config:\n{}",o1.config().toString(true)));
    for (auto&& record:records)
    {
        BOOST_TEST_MESSAGE(record.string());
    }
    BOOST_REQUIRE(o1.config().isSet(config8::obj1));
    const auto& obj1=o1.config().field(config8::obj1);
    BOOST_REQUIRE_EQUAL(obj1.count(),2);
    BOOST_CHECK_EQUAL(obj1.at(0).fieldValue(config1::field1),100);
    BOOST_CHECK_EQUAL(obj1.at(1).fieldValue(config1::field1),200);
    BOOST_REQUIRE_EQUAL(records.size(),2);
    BOOST_CHECK_EQUAL(records.at(0).string(),"\"obj1.0.field1\": 100");
    BOOST_CHECK_EQUAL(records.at(1).string(),"\"obj1.1.field1\": 200");

    ConfigTree t2;
    std::string json2="{\"level0\": {\"obj2\" : [{\"field1\":300},{\"field1\":400}], \"obj1\" : [{\"field1\":100},{\"field1\":200},{\"field1\":500}]}}";
    ec=loader.loadFromString(t2,json2);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    WithConfig8 o2;
    records.clear();
    ec=o2.loadLogConfig(t2,"level0",records);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    jsonR=jsonIo.serialize(t2);
    BOOST_CHECK(!jsonR);
    BOOST_TEST_MESSAGE(fmt::format("Config tree 2: \n{} \n",jsonR.value()));
    BOOST_TEST_MESSAGE(fmt::format("Nested object 2 config:\n{}",o2.config().toString(true)));
    for (auto&& record:records)
    {
        BOOST_TEST_MESSAGE(record.string());
    }

    BOOST_REQUIRE(o2.config().isSet(config8::obj1));
    const auto& obj2=o2.config().field(config8::obj1);
    BOOST_REQUIRE_EQUAL(obj2.count(),3);
    BOOST_CHECK_EQUAL(obj2.at(0).fieldValue(config1::field1),100);
    BOOST_CHECK_EQUAL(obj2.at(1).fieldValue(config1::field1),200);
    BOOST_CHECK_EQUAL(obj2.at(2).fieldValue(config1::field1),500);
    BOOST_REQUIRE_EQUAL(records.size(),3);
    BOOST_CHECK_EQUAL(records.at(0).string(),"\"level0.obj1.0.field1\": 100");
    BOOST_CHECK_EQUAL(records.at(1).string(),"\"level0.obj1.1.field1\": 200");
    BOOST_CHECK_EQUAL(records.at(2).string(),"\"level0.obj1.2.field1\": 500");
}

BOOST_AUTO_TEST_CASE(Subunit)
{
    ConfigTreeLoader loader;
    ConfigTree t1;
    std::string json1="{\"obj1\" : {\"field1\":100},\"f2\":300}";
    auto ec=loader.loadFromString(t1,json1);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    WithConfig9 o1;
    config_object::LogRecords records;
    ec=o1.loadLogConfig(t1,"",records);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    ConfigTreeJson jsonIo;
    auto jsonR=jsonIo.serialize(t1);
    BOOST_CHECK(!jsonR);
    BOOST_TEST_MESSAGE(fmt::format("Config tree: \n{} \n",jsonR.value()));
    BOOST_TEST_MESSAGE(fmt::format("Nested object config:\n{}",o1.config().toString(true)));
    for (auto&& record:records)
    {
        BOOST_TEST_MESSAGE(record.string());
    }

    BOOST_REQUIRE(o1.config().isSet(config9::obj1));
    BOOST_REQUIRE(o1.config().isSet(config9::f2));
    const auto& obj1=o1.config().field(config9::obj1);
    BOOST_CHECK_EQUAL(obj1.get().fieldValue(config1::field1),100);
    BOOST_CHECK_EQUAL(o1.config().fieldValue(config9::f2),300);
    BOOST_REQUIRE_EQUAL(records.size(),2);
    BOOST_CHECK_EQUAL(records.at(0).string(),"\"obj1.field1\": 100");
    BOOST_CHECK_EQUAL(records.at(1).string(),"\"f2\": 300");

    json1=std::string{"{\"level0\":{\"obj1\" : {\"field1\":500},\"f2\":700}}"};
    t1.reset();
    ec=loader.loadFromString(t1,json1);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    o1.config().reset();
    records.clear();
    ec=o1.loadLogConfig(t1,"level0",records);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    jsonR=jsonIo.serialize(t1);
    BOOST_CHECK(!jsonR);
    BOOST_TEST_MESSAGE(fmt::format("\n\nConfig tree 2: \n{} \n",jsonR.value()));
    BOOST_TEST_MESSAGE(fmt::format("Nested object 2 config:\n{}",o1.config().toString(true)));
    for (auto&& record:records)
    {
        BOOST_TEST_MESSAGE(record.string());
    }

    BOOST_REQUIRE(o1.config().isSet(config9::obj1));
    BOOST_REQUIRE(o1.config().isSet(config9::f2));
    const auto& obj2=o1.config().field(config9::obj1);
    BOOST_CHECK_EQUAL(obj2.get().fieldValue(config1::field1),500);
    BOOST_CHECK_EQUAL(o1.config().fieldValue(config9::f2),700);
    BOOST_REQUIRE_EQUAL(records.size(),2);
    BOOST_CHECK_EQUAL(records.at(0).string(),"\"level0.obj1.field1\": 500");
    BOOST_CHECK_EQUAL(records.at(1).string(),"\"level0.f2\": 700");
}

BOOST_AUTO_TEST_SUITE_END()
