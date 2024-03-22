/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/test/testconfigtreejson.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/common/filesystem.h>
#include <hatn/base/configtreejson.h>
#include <hatn/test/multithreadfixture.h>

HATN_USING
HATN_COMMON_USING
HATN_BASE_USING
HATN_TEST_USING

namespace {

void checkConfigTree(const ConfigTree& t)
{
    BOOST_CHECK_EQUAL(t.get("int8")->as<int8_t>().value(),-127);
    BOOST_CHECK_EQUAL(t.get("int16")->as<int16_t>().value(),-32767);
    BOOST_CHECK_EQUAL(t.get("int32")->as<int32_t>().value(),-2147483647);
    BOOST_CHECK_EQUAL(t.get("int64")->as<int64_t>().value(),-9223372036854775807);
    BOOST_CHECK_EQUAL(t.get("uint8")->as<uint8_t>().value(),127);
    BOOST_CHECK_EQUAL(t.get("uint16")->as<uint16_t>().value(),32767);
    BOOST_CHECK_EQUAL(t.get("uint32")->as<uint32_t>().value(),2147483647);
    BOOST_CHECK_EQUAL(t.get("uint64")->as<uint64_t>().value(),9223372036854775807);
    BOOST_CHECK_EQUAL(t.get("bool-true")->as<bool>().value(),true);
    BOOST_CHECK_EQUAL(t.get("bool-false")->as<bool>().value(),false);
    BOOST_CHECK_EQUAL(t.get("double-pos")->as<double>().value(),9.101);
    BOOST_CHECK_EQUAL(t.get("double-neg")->as<double>().value(),-9.101);
    BOOST_CHECK_EQUAL(t.get("string")->as<std::string>().value(),std::string("Hello world!"));

    auto arrInt=t.get("array-int")->asArray<int64_t>();
    BOOST_REQUIRE_EQUAL(arrInt->size(),8);
    BOOST_CHECK_EQUAL(static_cast<int8_t>(arrInt->at(0)),-127);
    BOOST_CHECK_EQUAL(static_cast<int16_t>(arrInt->at(1)),-32767);
    BOOST_CHECK_EQUAL(static_cast<int32_t>(arrInt->at(2)),-2147483647);
    BOOST_CHECK_EQUAL(static_cast<int64_t>(arrInt->at(3)),-9223372036854775807);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(arrInt->at(4)),127);
    BOOST_CHECK_EQUAL(static_cast<uint16_t>(arrInt->at(5)),32767);
    BOOST_CHECK_EQUAL(static_cast<uint32_t>(arrInt->at(6)),2147483647);
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(arrInt->at(7)),9223372036854775807);

    auto arrBool=t.get("array-bool")->asArray<bool>();
    BOOST_REQUIRE_EQUAL(arrBool->size(),4);
    BOOST_CHECK_EQUAL(arrBool->at(0),true);
    BOOST_CHECK_EQUAL(arrBool->at(1),false);
    BOOST_CHECK_EQUAL(arrBool->at(2),false);
    BOOST_CHECK_EQUAL(arrBool->at(3),true);

    auto arrDouble=t.get("array-double")->asArray<double>();
    BOOST_REQUIRE_EQUAL(arrDouble->size(),5);
    BOOST_CHECK_EQUAL(arrDouble->at(0),123.567);
    BOOST_CHECK_EQUAL(arrDouble->at(1),89.0);
    BOOST_CHECK_EQUAL(arrDouble->at(2),100.102);
    BOOST_CHECK_EQUAL(arrDouble->at(3),-15.0);
    BOOST_CHECK_EQUAL(arrDouble->at(4),-109.21);

    auto arrString=t.get("array-string")->asArray<std::string>();
    BOOST_REQUIRE_EQUAL(arrString->size(),4);
    BOOST_CHECK_EQUAL(arrString->at(0),"one");
    BOOST_CHECK_EQUAL(arrString->at(1),"two");
    BOOST_CHECK_EQUAL(arrString->at(2),"three");
    BOOST_CHECK_EQUAL(arrString->at(3),"four");

    BOOST_CHECK_EQUAL(t.get("array-tree.0.1int8")->as<int8_t>().value(),-127);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1int16")->as<int16_t>().value(),-32767);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1int32")->as<int32_t>().value(),-2147483647);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1int64")->as<int64_t>().value(),-9223372036854775807);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1uint8")->as<uint8_t>().value(),127);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1uint16")->as<uint16_t>().value(),32767);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1uint32")->as<uint32_t>().value(),2147483647);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1uint64")->as<uint64_t>().value(),9223372036854775807);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1bool-true")->as<bool>().value(),true);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1bool-false")->as<bool>().value(),false);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1double-pos")->as<double>().value(),9.101);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1double-neg")->as<double>().value(),-9.101);
    BOOST_CHECK_EQUAL(t.get("array-tree.0.1string")->as<std::string>().value(),std::string("Hello world!"));

    arrInt=t.get("array-tree.0.1array-int")->asArray<int64_t>();
    BOOST_REQUIRE_EQUAL(arrInt->size(),8);
    BOOST_CHECK_EQUAL(static_cast<int8_t>(arrInt->at(0)),-127);
    BOOST_CHECK_EQUAL(static_cast<int16_t>(arrInt->at(1)),-32767);
    BOOST_CHECK_EQUAL(static_cast<int32_t>(arrInt->at(2)),-2147483647);
    BOOST_CHECK_EQUAL(static_cast<int64_t>(arrInt->at(3)),-9223372036854775807);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(arrInt->at(4)),127);
    BOOST_CHECK_EQUAL(static_cast<uint16_t>(arrInt->at(5)),32767);
    BOOST_CHECK_EQUAL(static_cast<uint32_t>(arrInt->at(6)),2147483647);
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(arrInt->at(7)),9223372036854775807);

    arrBool=t.get("array-tree.0.1array-bool")->asArray<bool>();
    BOOST_REQUIRE_EQUAL(arrBool->size(),4);
    BOOST_CHECK_EQUAL(arrBool->at(0),true);
    BOOST_CHECK_EQUAL(arrBool->at(1),false);
    BOOST_CHECK_EQUAL(arrBool->at(2),false);
    BOOST_CHECK_EQUAL(arrBool->at(3),true);

    arrDouble=t.get("array-tree.0.1array-double")->asArray<double>();
    BOOST_REQUIRE_EQUAL(arrDouble->size(),5);
    BOOST_CHECK_EQUAL(arrDouble->at(0),123.567);
    BOOST_CHECK_EQUAL(arrDouble->at(1),89.0);
    BOOST_CHECK_EQUAL(arrDouble->at(2),100.102);
    BOOST_CHECK_EQUAL(arrDouble->at(3),-15.0);
    BOOST_CHECK_EQUAL(arrDouble->at(4),-109.21);

    arrString=t.get("array-tree.0.1array-string")->asArray<std::string>();
    BOOST_REQUIRE_EQUAL(arrString->size(),4);
    BOOST_CHECK_EQUAL(arrString->at(0),"1one");
    BOOST_CHECK_EQUAL(arrString->at(1),"1two");
    BOOST_CHECK_EQUAL(arrString->at(2),"1three");
    BOOST_CHECK_EQUAL(arrString->at(3),"1four");

    BOOST_CHECK_EQUAL(t.get("array-tree.1.2int8")->as<int8_t>().value(),-125);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2int16")->as<int16_t>().value(),-32765);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2int32")->as<int32_t>().value(),-2147483645);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2int64")->as<int64_t>().value(),-9223372036854775805);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2uint8")->as<uint8_t>().value(),125);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2uint16")->as<uint16_t>().value(),32765);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2uint32")->as<uint32_t>().value(),2147483645);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2uint64")->as<uint64_t>().value(),9223372036854775805);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2bool-true")->as<bool>().value(),true);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2bool-false")->as<bool>().value(),false);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2double-pos")->as<double>().value(),9.501);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2double-neg")->as<double>().value(),-9.501);
    BOOST_CHECK_EQUAL(t.get("array-tree.1.2string")->as<std::string>().value(),std::string("Hi!"));

    arrInt=t.get("array-tree.1.2array-int")->asArray<int64_t>();
    BOOST_REQUIRE_EQUAL(arrInt->size(),8);
    BOOST_CHECK_EQUAL(static_cast<int8_t>(arrInt->at(0)),-126);
    BOOST_CHECK_EQUAL(static_cast<int16_t>(arrInt->at(1)),-32766);
    BOOST_CHECK_EQUAL(static_cast<int32_t>(arrInt->at(2)),-2147483646);
    BOOST_CHECK_EQUAL(static_cast<int64_t>(arrInt->at(3)),-9223372036854775806);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(arrInt->at(4)),126);
    BOOST_CHECK_EQUAL(static_cast<uint16_t>(arrInt->at(5)),32766);
    BOOST_CHECK_EQUAL(static_cast<uint32_t>(arrInt->at(6)),2147483646);
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(arrInt->at(7)),9223372036854775806);

    arrBool=t.get("array-tree.1.2array-bool")->asArray<bool>();
    BOOST_REQUIRE_EQUAL(arrBool->size(),4);
    BOOST_CHECK_EQUAL(arrBool->at(0),true);
    BOOST_CHECK_EQUAL(arrBool->at(1),false);
    BOOST_CHECK_EQUAL(arrBool->at(2),false);
    BOOST_CHECK_EQUAL(arrBool->at(3),true);

    arrDouble=t.get("array-tree.1.2array-double")->asArray<double>();
    BOOST_REQUIRE_EQUAL(arrDouble->size(),5);
    BOOST_CHECK_EQUAL(arrDouble->at(0),5123.567);
    BOOST_CHECK_EQUAL(arrDouble->at(1),589.0);
    BOOST_CHECK_EQUAL(arrDouble->at(2),5100.102);
    BOOST_CHECK_EQUAL(arrDouble->at(3),-515.0);
    BOOST_CHECK_EQUAL(arrDouble->at(4),-5109.21);

    arrString=t.get("array-tree.1.2array-string")->asArray<std::string>();
    BOOST_REQUIRE_EQUAL(arrString->size(),4);
    BOOST_CHECK_EQUAL(arrString->at(0),"2one");
    BOOST_CHECK_EQUAL(arrString->at(1),"2two");
    BOOST_CHECK_EQUAL(arrString->at(2),"2three");
    BOOST_CHECK_EQUAL(arrString->at(3),"2four");

    BOOST_CHECK_EQUAL(t.get("subtree.4int8")->as<int8_t>().value(),-124);
    BOOST_CHECK_EQUAL(t.get("subtree.4int16")->as<int16_t>().value(),-32764);
    BOOST_CHECK_EQUAL(t.get("subtree.4int32")->as<int32_t>().value(),-2147483644);
    BOOST_CHECK_EQUAL(t.get("subtree.4int64")->as<int64_t>().value(),-9223372036854775804);
    BOOST_CHECK_EQUAL(t.get("subtree.4uint8")->as<uint8_t>().value(),124);
    BOOST_CHECK_EQUAL(t.get("subtree.4uint16")->as<uint16_t>().value(),32764);
    BOOST_CHECK_EQUAL(t.get("subtree.4uint32")->as<uint32_t>().value(),2147483644);
    BOOST_CHECK_EQUAL(t.get("subtree.4uint64")->as<uint64_t>().value(),9223372036854775804);
    BOOST_CHECK_EQUAL(t.get("subtree.4bool-true")->as<bool>().value(),true);
    BOOST_CHECK_EQUAL(t.get("subtree.4bool-false")->as<bool>().value(),false);
    BOOST_CHECK_EQUAL(t.get("subtree.4double-pos")->as<double>().value(),9.504);
    BOOST_CHECK_EQUAL(t.get("subtree.4double-neg")->as<double>().value(),-9.504);
    BOOST_CHECK_EQUAL(t.get("subtree.4string")->as<std::string>().value(),std::string("Hi!"));

    arrInt=t.get("subtree.4array-int")->asArray<int64_t>();
    BOOST_REQUIRE_EQUAL(arrInt->size(),8);
    BOOST_CHECK_EQUAL(static_cast<int8_t>(arrInt->at(0)),-124);
    BOOST_CHECK_EQUAL(static_cast<int16_t>(arrInt->at(1)),-32764);
    BOOST_CHECK_EQUAL(static_cast<int32_t>(arrInt->at(2)),-2147483644);
    BOOST_CHECK_EQUAL(static_cast<int64_t>(arrInt->at(3)),-9223372036854775804);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(arrInt->at(4)),124);
    BOOST_CHECK_EQUAL(static_cast<uint16_t>(arrInt->at(5)),32764);
    BOOST_CHECK_EQUAL(static_cast<uint32_t>(arrInt->at(6)),2147483644);
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(arrInt->at(7)),9223372036854775804);

    arrBool=t.get("subtree.4array-bool")->asArray<bool>();
    BOOST_REQUIRE_EQUAL(arrBool->size(),4);
    BOOST_CHECK_EQUAL(arrBool->at(0),true);
    BOOST_CHECK_EQUAL(arrBool->at(1),false);
    BOOST_CHECK_EQUAL(arrBool->at(2),false);
    BOOST_CHECK_EQUAL(arrBool->at(3),true);

    arrDouble=t.get("subtree.4array-double")->asArray<double>();
    BOOST_REQUIRE_EQUAL(arrDouble->size(),5);
    BOOST_CHECK_EQUAL(arrDouble->at(0),5123.564);
    BOOST_CHECK_EQUAL(arrDouble->at(1),589.4);
    BOOST_CHECK_EQUAL(arrDouble->at(2),5100.104);
    BOOST_CHECK_EQUAL(arrDouble->at(3),-515.4);
    BOOST_CHECK_EQUAL(arrDouble->at(4),-5109.24);

    arrString=t.get("subtree.4array-string")->asArray<std::string>();
    BOOST_REQUIRE_EQUAL(arrString->size(),4);
    BOOST_CHECK_EQUAL(arrString->at(0),"4one");
    BOOST_CHECK_EQUAL(arrString->at(1),"4two");
    BOOST_CHECK_EQUAL(arrString->at(2),"4three");
    BOOST_CHECK_EQUAL(arrString->at(3),"4four");

    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3int8")->as<int8_t>().value(),-123);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3int16")->as<int16_t>().value(),-32763);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3int32")->as<int32_t>().value(),-2147483643);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3int64")->as<int64_t>().value(),-9223372036854775803);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3uint8")->as<uint8_t>().value(),123);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3uint16")->as<uint16_t>().value(),32763);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3uint32")->as<uint32_t>().value(),2147483643);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3uint64")->as<uint64_t>().value(),9223372036854775803);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3bool-true")->as<bool>().value(),true);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3bool-false")->as<bool>().value(),false);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3double-pos")->as<double>().value(),9.503);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3double-neg")->as<double>().value(),-9.503);
    BOOST_CHECK_EQUAL(t.get("subtree.subtree-subtree.3string")->as<std::string>().value(),std::string("How are you?"));

    arrInt=t.get("subtree.subtree-subtree.3array-int")->asArray<int64_t>();
    BOOST_REQUIRE_EQUAL(arrInt->size(),8);
    BOOST_CHECK_EQUAL(static_cast<int8_t>(arrInt->at(0)),-123);
    BOOST_CHECK_EQUAL(static_cast<int16_t>(arrInt->at(1)),-32763);
    BOOST_CHECK_EQUAL(static_cast<int32_t>(arrInt->at(2)),-2147483643);
    BOOST_CHECK_EQUAL(static_cast<int64_t>(arrInt->at(3)),-9223372036854775803);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(arrInt->at(4)),123);
    BOOST_CHECK_EQUAL(static_cast<uint16_t>(arrInt->at(5)),32763);
    BOOST_CHECK_EQUAL(static_cast<uint32_t>(arrInt->at(6)),2147483643);
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(arrInt->at(7)),9223372036854775803);

    arrBool=t.get("subtree.subtree-subtree.3array-bool")->asArray<bool>();
    BOOST_REQUIRE_EQUAL(arrBool->size(),4);
    BOOST_CHECK_EQUAL(arrBool->at(0),true);
    BOOST_CHECK_EQUAL(arrBool->at(1),false);
    BOOST_CHECK_EQUAL(arrBool->at(2),false);
    BOOST_CHECK_EQUAL(arrBool->at(3),true);

    arrDouble=t.get("subtree.subtree-subtree.3array-double")->asArray<double>();
    BOOST_REQUIRE_EQUAL(arrDouble->size(),5);
    BOOST_CHECK_EQUAL(arrDouble->at(0),3123.564);
    BOOST_CHECK_EQUAL(arrDouble->at(1),389.4);
    BOOST_CHECK_EQUAL(arrDouble->at(2),3100.104);
    BOOST_CHECK_EQUAL(arrDouble->at(3),-315.4);
    BOOST_CHECK_EQUAL(arrDouble->at(4),-3109.24);

    arrString=t.get("subtree.subtree-subtree.3array-string")->asArray<std::string>();
    BOOST_REQUIRE_EQUAL(arrString->size(),4);
    BOOST_CHECK_EQUAL(arrString->at(0),"3one");
    BOOST_CHECK_EQUAL(arrString->at(1),"3two");
    BOOST_CHECK_EQUAL(arrString->at(2),"3three");
    BOOST_CHECK_EQUAL(arrString->at(3),"3four");

    auto arrArrInt=t.get("array-array.0")->asArray<int8_t>();
    BOOST_REQUIRE_EQUAL(arrArrInt->size(),2);
    BOOST_CHECK_EQUAL(arrArrInt->at(0),1);
    BOOST_CHECK_EQUAL(arrArrInt->at(1),2);

    auto arrArrStr=t.get("array-array.1")->asArray<std::string>();
    BOOST_REQUIRE_EQUAL(arrArrStr->size(),2);
    BOOST_CHECK_EQUAL(arrArrStr->at(0),"a");
    BOOST_CHECK_EQUAL(arrArrStr->at(1),"b");

    BOOST_CHECK_EQUAL(t.get("array-array.2.0.aa")->as<int8_t>().value(),1);
    BOOST_CHECK_EQUAL(t.get("array-array.2.0.ab")->as<int8_t>().value(),2);
    BOOST_CHECK_EQUAL(t.get("array-array.2.1.ba")->as<bool>().value(),true);
    BOOST_CHECK_EQUAL(t.get("array-array.2.1.bb")->as<std::string>().value(),"Hello world!");
}

}

BOOST_AUTO_TEST_SUITE(TestConfigTree)

BOOST_AUTO_TEST_CASE(ConfigTreeJsonIo, *boost::unit_test::tolerance(0.000001))
{
    ConfigTree t1;

    // test parsing and serializing
    ConfigTreeJson jsonIo;

    auto filename1=MultiThreadFixture::assetsFilePath("base/assets/config1.jsonc");
    auto ec=jsonIo.loadFromFile(t1,filename1);
    HATN_TEST_EC(ec);
    checkConfigTree(t1);

#if 1
    auto keys=t1.allKeys();
    BOOST_REQUIRE(!keys);
    std::cout<<"Parsed keys"<<std::endl;
    std::cout<<"*********************************"<<std::endl;
    for (auto&& it: keys.value())
    {
        std::cout<<it<<std::endl;
    }
    std::cout<<"*********************************"<<std::endl;
#endif

    auto jsonR1=jsonIo.serialize(t1);
    BOOST_CHECK(!jsonR1);
#if 0
    std::cout<<"Parsed and serialized back object 1"<<std::endl;
    std::cout<<"*********************************"<<std::endl;
    std::cout<<jsonR1.value()<<std::endl;
    std::cout<<"*********************************"<<std::endl;
#endif

    ConfigTree t2;
    ec=jsonIo.parse(t2,jsonR1.value());
    HATN_TEST_EC(ec);
    checkConfigTree(t2);

    // write to file
    auto tmpFile=MultiThreadFixture::tmpFilePath("config1-save.jsonc");
    if (lib::filesystem::exists(tmpFile))
    {
        lib::filesystem::remove(tmpFile);
    }
    ec=jsonIo.saveToFile(t1,tmpFile);
    HATN_TEST_EC(ec);
    ConfigTree t3;
    ec=jsonIo.loadFromFile(t3,tmpFile);
    HATN_TEST_EC(ec);
    checkConfigTree(t3);

    // test errors
    auto filename2=MultiThreadFixture::assetsFilePath("base/assets/config_err1.jsonc");
    ec=jsonIo.loadFromFile(t1,filename2);
    BOOST_TEST_MESSAGE(fmt::format("Expected parsing failure: {}", ec.message()));
    BOOST_CHECK(ec);

    auto filename3=MultiThreadFixture::assetsFilePath("base/assets/config_err2.jsonc");
    ec=jsonIo.loadFromFile(t1,filename3);
    BOOST_TEST_MESSAGE(fmt::format("Expected parsing failure: {}", ec.message()));
    BOOST_CHECK(ec);
}

BOOST_AUTO_TEST_CASE(ConfigTreeJsonDefault, *boost::unit_test::tolerance(0.000001))
{
    // test merge with overriding default values
    ConfigTree tm1;
    tm1.setDefault("one.two.default",100);
    BOOST_REQUIRE(tm1.isSet("one.two.default",true));
    BOOST_CHECK_EQUAL(tm1.get("one.two.default")->as<uint32_t>().value(),100);
    BOOST_REQUIRE(tm1.isDefaultSet("one.two.default"));
    BOOST_CHECK_EQUAL(tm1.get("one.two.default")->getDefault<uint32_t>().value(),100);
    tm1.setDefault("one.two.default2",1000);
    BOOST_REQUIRE(tm1.isSet("one.two.default2",true));
    BOOST_CHECK_EQUAL(tm1.get("one.two.default2")->as<uint32_t>().value(),1000);
    BOOST_REQUIRE(tm1.isDefaultSet("one.two.default2"));
    BOOST_CHECK_EQUAL(tm1.get("one.two.default2")->getDefault<uint32_t>().value(),1000);
    tm1.set("one.two.default2",5000);
    BOOST_REQUIRE(tm1.isSet("one.two.default2"));
    BOOST_CHECK_EQUAL(tm1.get("one.two.default2")->as<uint32_t>().value(),5000);

    ConfigTreeJson jsonIo;
    auto jsonR0=jsonIo.serialize(tm1);
    BOOST_CHECK(!jsonR0);
#if 1
    std::cout<<"Serialize target tree with defaults before merge"<<std::endl;
    std::cout<<"*********************************"<<std::endl;
    std::cout<<jsonR0.value()<<std::endl;
    std::cout<<"*********************************"<<std::endl;
#endif

    ConfigTree tm2;
    tm2.set("one.two.default",200);
    BOOST_REQUIRE(tm2.isSet("one.two.default"));
    BOOST_CHECK_EQUAL(tm2.get("one.two.default")->as<uint32_t>().value(),200);
    BOOST_CHECK(!tm2.isDefaultSet("one.two.default"));
    tm2.setDefault("one.two.default2",7000);
    BOOST_REQUIRE(!tm2.isSet("one.two.default2"));
    BOOST_CHECK_EQUAL(tm2.get("one.two.default2")->as<uint32_t>().value(),7000);
    BOOST_CHECK(tm2.isDefaultSet("one.two.default2"));
    BOOST_CHECK_EQUAL(tm2.get("one.two.default2")->getDefault<uint32_t>().value(),7000);

    auto jsonR1=jsonIo.serialize(tm2);
    BOOST_CHECK(!jsonR1);
#if 1
    std::cout<<"Serialize source tree with defaults before merge"<<std::endl;
    std::cout<<"*********************************"<<std::endl;
    std::cout<<jsonR1.value()<<std::endl;
    std::cout<<"*********************************"<<std::endl;
#endif

    auto ec=tm1.merge(std::move(tm2));
    HATN_TEST_EC(ec);
    BOOST_REQUIRE(tm1.isSet("one.two.default"));
    BOOST_CHECK_EQUAL(tm1.get("one.two.default")->as<uint32_t>().value(),200);
    BOOST_REQUIRE(tm1.isDefaultSet("one.two.default"));
    BOOST_CHECK_EQUAL(tm1.get("one.two.default")->getDefault<uint32_t>().value(),100);
    BOOST_REQUIRE(tm1.isSet("one.two.default2"));
    BOOST_CHECK_EQUAL(tm1.get("one.two.default2")->as<uint32_t>().value(),5000);
    BOOST_REQUIRE(tm1.isDefaultSet("one.two.default2"));
    BOOST_CHECK_EQUAL(tm1.get("one.two.default2")->getDefault<uint32_t>().value(),7000);

    auto jsonR2=jsonIo.serialize(tm1);
    BOOST_CHECK(!jsonR2);
#if 1
    std::cout<<"Serialize tree with defaults after merge"<<std::endl;
    std::cout<<"*********************************"<<std::endl;
    std::cout<<jsonR2.value()<<std::endl;
    std::cout<<"*********************************"<<std::endl;
#endif

    // test parsing with default and preset values
    ConfigTree t1;
    t1.setDefault("default.one.two",100);
    t1.setDefault("subtree.subtree-subtree.default-string","Default value");
    t1.setDefault("subtree.4string","Default override");
    t1.set("subtree.subtree-subtree.preset-string","Preset value");
    t1.set("subtree.subtree-subtree.3string","Preset override");
    BOOST_REQUIRE(t1.isSet("default.one.two",true));
    BOOST_CHECK_EQUAL(t1.get("default.one.two")->as<uint32_t>().value(),100);
    BOOST_CHECK(!t1.isSet(std::string("subtree.subtree-subtree.default-string")));
    BOOST_CHECK(!t1.isSet("subtree.subtree-subtree.default-string"));
    BOOST_REQUIRE(t1.isSet("subtree.subtree-subtree.default-string",true));
    BOOST_CHECK_EQUAL(t1.get("subtree.subtree-subtree.default-string")->as<std::string>().value(),"Default value");
    BOOST_REQUIRE(t1.isSet("subtree.subtree-subtree.preset-string"));
    BOOST_CHECK_EQUAL(t1.get("subtree.subtree-subtree.preset-string")->as<std::string>().value(),"Preset value");
    BOOST_REQUIRE(t1.isSet("subtree.subtree-subtree.3string"));
    BOOST_CHECK_EQUAL(t1.get("subtree.subtree-subtree.3string")->as<std::string>().value(),"Preset override");
    BOOST_REQUIRE(t1.isSet("subtree.4string",true));
    BOOST_CHECK_EQUAL(t1.get("subtree.4string")->as<std::string>().value(),"Default override");

    auto filename1=MultiThreadFixture::assetsFilePath("base/assets/config1.jsonc");
    ec=jsonIo.loadFromFile(t1,filename1);
    HATN_TEST_EC(ec);
    checkConfigTree(t1);
    BOOST_REQUIRE(t1.isSet("default.one.two",true));
    BOOST_CHECK_EQUAL(t1.get("default.one.two")->as<uint32_t>().value(),100);
    BOOST_REQUIRE(t1.isSet("subtree.subtree-subtree.default-string",true));
    BOOST_CHECK_EQUAL(t1.get("subtree.subtree-subtree.default-string")->as<std::string>().value(),"Default value");
    BOOST_REQUIRE(t1.isSet("subtree.subtree-subtree.preset-string"));
    BOOST_CHECK_EQUAL(t1.get("subtree.subtree-subtree.preset-string")->as<std::string>().value(),"Preset value");
    BOOST_REQUIRE(t1.isSet("subtree.subtree-subtree.3string"));
    BOOST_CHECK_EQUAL(t1.get("subtree.subtree-subtree.3string")->as<std::string>().value(),"How are you?");
    BOOST_REQUIRE(t1.isSet("subtree.4string"));
    BOOST_CHECK_EQUAL(t1.get("subtree.4string")->as<std::string>().value(),"Hi!");
    BOOST_REQUIRE(t1.isDefaultSet("subtree.4string"));
    BOOST_CHECK_EQUAL(t1.get("subtree.4string")->getDefault<std::string>().value(),"Default override");
}

BOOST_AUTO_TEST_SUITE_END()
