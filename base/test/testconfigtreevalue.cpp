/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/test/testconfigtreevalue.cpp
  *
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

BOOST_AUTO_TEST_CASE(IntValue)
{
    ConfigTreeValue t1;
    BOOST_CHECK(!t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.type()));

    // check empty value
    auto intR1=t1.as<int64_t>();
    BOOST_CHECK(static_cast<bool>(intR1));
    BOOST_CHECK(!intR1.isValid());
    BOOST_CHECK_EQUAL(intR1.error().value(),static_cast<int>(base::BaseError::VALUE_NOT_SET));

    // set int64
    int64_t int64_1{0x1122334455667788};
    t1.set(int64_1);
    BOOST_CHECK(t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(t1.type()));

    // read int64
    auto intR2=t1.as<int64_t>();
    BOOST_CHECK(!static_cast<bool>(intR2));
    BOOST_CHECK(intR2.isValid());
    BOOST_CHECK_EQUAL(int64_1,intR2.takeValue());
    auto intR3=std::move(intR2);
    int64_t int64_2{0x0177665544332211};
    t1.set(int64_2);
    intR3=t1.as<int64_t>();
    BOOST_CHECK(intR2.isValid());
    BOOST_CHECK_EQUAL(int64_2,*intR3);
    BOOST_CHECK_EQUAL(int64_2,intR3.value());

    // read uint64_t from int64_t
    auto uint64_1=t1.asThrows<uint64_t>();
    BOOST_CHECK_EQUAL(0x0177665544332211,uint64_1);

    // set uint64_t
    uint64_t uint64_2{0xCC77665544332211};
    t1.set(0xCC77665544332211);
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(t1.type()));
    // read uint64_t
    uint64_1=t1.asThrows<uint64_t>();
    BOOST_CHECK_EQUAL(uint64_2,uint64_1);
    // read uint32_t
    auto uint32_1=t1.asThrows<uint32_t>();
    BOOST_CHECK_EQUAL(0x44332211,uint32_1);
    // read uint16_t
    auto uint16_1=t1.asThrows<uint16_t>();
    BOOST_CHECK_EQUAL(0x2211,uint16_1);
    // read uint8_t
    auto uint8_1=t1.asThrows<uint8_t>();
    BOOST_CHECK_EQUAL(0x11,uint8_1);
    // read int64_t
    auto int64_3=t1.asThrows<int64_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int64 from big uint64 {}",int64_3));
    // read int8_t
    auto int8_1=t1.asThrows<int8_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int8 from big uint64 {}",int8_1));

    // set negative int8_t
    int8_t int8_2{-3};
    t1.set(int8_2);
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(t1.type()));
    // read int8_t
    int8_1=t1.asThrows<int8_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int8 from int8 {}",int8_1));
    BOOST_CHECK_EQUAL(int8_2,int8_1);
    // read int16_t
    auto int16_1=t1.asThrows<int16_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int16 from int8 {}",int16_1));
    BOOST_CHECK_EQUAL(int8_2,int16_1);
    // read int32_t
    auto int32_1=t1.asThrows<int32_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int32 from int8 {}",int32_1));
    BOOST_CHECK_EQUAL(int8_2,int32_1);
    // read int64_t
    int64_1=t1.asThrows<int64_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int64 from int8 {}",int64_1));
    BOOST_CHECK_EQUAL(int8_2,int64_1);

    // set negative int32_t
    int32_t int32_2{-128};
    t1.set(int32_2);
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(t1.type()));
    // read int8_t
    int8_1=t1.asThrows<int8_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int8 from int32 {}",int8_1));
    BOOST_CHECK_EQUAL(int32_2,int8_1);
    // read int16_t
    int16_1=t1.asThrows<int16_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int16 from int32 {}",int16_1));
    BOOST_CHECK_EQUAL(int32_2,int16_1);
    // read int32_t
    int32_1=t1.asThrows<int32_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int32 from int32 {}",int32_1));
    BOOST_CHECK_EQUAL(int32_2,int32_1);
    // read int64_t
    int64_1=t1.asThrows<int64_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int64 from int32 {}",int64_1));
    BOOST_CHECK_EQUAL(int32_2,int64_1);

    // set negative int16_t
    int32_t int16_2{-129};
    t1.set(int16_2);
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(t1.type()));
    // read int8_t
    int8_1=t1.asThrows<int8_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int8 from int32 {}",int8_1));
    BOOST_CHECK_EQUAL(127,int8_1);
    // read int16_t
    int16_1=t1.asThrows<int16_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int16 from int32 {}",int16_1));
    BOOST_CHECK_EQUAL(int16_2,int16_1);
    // read int32_t
    int32_1=t1.asThrows<int32_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int32 from int32 {}",int32_1));
    BOOST_CHECK_EQUAL(int16_2,int32_1);
    // read int64_t
    int64_1=t1.asThrows<int64_t>();
    BOOST_TEST_MESSAGE(fmt::format("Read int64 from int32 {}",int64_1));
    BOOST_CHECK_EQUAL(int16_2,int64_1);

    // reset value
    t1.reset();
    BOOST_CHECK(!t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.type()));
    intR1=t1.as<int64_t>();
    BOOST_CHECK(static_cast<bool>(intR1));
    BOOST_CHECK(!intR1.isValid());
    common::Error ec;
    auto val=t1.as<int64_t>(ec);
    std::ignore=val;
    BOOST_CHECK(static_cast<bool>(ec));
    BOOST_CHECK_EQUAL(ec.value(),static_cast<int>(base::BaseError::VALUE_NOT_SET));

    BOOST_CHECK_THROW(t1.asThrows<int64_t>(),common::ErrorException);
}

BOOST_AUTO_TEST_CASE(DefaultValue)
{
    ConfigTreeValue t1;
    BOOST_CHECK(!t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.type()));
    BOOST_CHECK(!t1.isDefaultSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.defaultType()));

    // check empty value
    auto intR1=t1.as<int64_t>();
    BOOST_CHECK(static_cast<bool>(intR1));
    BOOST_CHECK(!intR1.isValid());
    BOOST_CHECK_EQUAL(intR1.error().value(),static_cast<int>(base::BaseError::VALUE_NOT_SET));
    intR1=t1.getDefault<int64_t>();
    BOOST_CHECK(static_cast<bool>(intR1));
    BOOST_CHECK(!intR1.isValid());
    BOOST_CHECK_EQUAL(intR1.error().value(),static_cast<int>(base::BaseError::VALUE_NOT_SET));

    // set default
    t1.setDefault(123);
    BOOST_CHECK(!t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.type()));
    BOOST_CHECK(t1.isDefaultSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.type()));
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(t1.defaultType()));
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(t1.type(true)));
    BOOST_CHECK(t1.isSet(true));
    BOOST_CHECK(!t1.isSet());

    // read default
    auto def1=t1.getDefault<int32_t>();
    BOOST_CHECK(def1.isValid());
    BOOST_CHECK_EQUAL(123,def1.takeValue());
    // read value
    auto val1=t1.as<int32_t>();
    BOOST_CHECK(val1.isValid());
    BOOST_CHECK_EQUAL(123,val1.takeValue());

    // reset default
    t1.resetDefault();
    intR1=t1.as<int64_t>();
    BOOST_CHECK(static_cast<bool>(intR1));
    BOOST_CHECK(!intR1.isValid());
    BOOST_CHECK_EQUAL(intR1.error().value(),static_cast<int>(base::BaseError::VALUE_NOT_SET));
    intR1=t1.getDefault<int64_t>();
    BOOST_CHECK(static_cast<bool>(intR1));
    BOOST_CHECK(!intR1.isValid());
    BOOST_CHECK_EQUAL(intR1.error().value(),static_cast<int>(base::BaseError::VALUE_NOT_SET));
    BOOST_CHECK_THROW(t1.getDefaultThrows<int64_t>(),common::ErrorException);
    common::Error ec;
    auto val=t1.getDefault<int64_t>(ec);
    std::ignore=val;
    BOOST_CHECK(static_cast<bool>(ec));
    BOOST_CHECK_EQUAL(ec.value(),static_cast<int>(base::BaseError::VALUE_NOT_SET));
}

BOOST_AUTO_TEST_CASE(BoolValue)
{
    ConfigTreeValue t1;
    BOOST_CHECK(!t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.type()));

    // set true
    bool val1=true;
    t1.set(val1);
    BOOST_CHECK(t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::Bool),int(t1.type()));

    // read bool
    auto r1=t1.as<bool>();
    BOOST_CHECK(!static_cast<bool>(r1));
    BOOST_CHECK(r1.isValid());
    BOOST_CHECK_EQUAL(val1,r1.takeValue());

    // set false
    bool val2=false;
    t1.set(val2);
    BOOST_CHECK(t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::Bool),int(t1.type()));

    // read bool
    auto r2=t1.as<bool>();
    BOOST_CHECK(r2.isValid());
    BOOST_CHECK_EQUAL(val2,r2.takeValue());

    // try to read int32_t
    auto r3=t1.as<int32_t>();
    BOOST_CHECK(!r3.isValid());
    BOOST_CHECK_EQUAL(r3.error().value(),static_cast<int>(base::BaseError::INVALID_TYPE));
}

BOOST_AUTO_TEST_CASE(FloatingValue, *boost::unit_test::tolerance(0.000001))
{
    ConfigTreeValue t1;
    BOOST_CHECK(!t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.type()));

    // set float
    float val1=float(0.107);
    t1.set(val1);
    BOOST_CHECK(t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::Double),int(t1.type()));

    // read float
    auto r1=t1.as<float>();
    BOOST_CHECK(!static_cast<bool>(r1));
    BOOST_CHECK(r1.isValid());
    BOOST_TEST(val1==r1.takeValue());

    // read double
    auto r1_1=t1.as<double>();
    BOOST_CHECK(!static_cast<bool>(r1));
    BOOST_CHECK(r1.isValid());
    BOOST_TEST(val1==r1_1.takeValue());

    // set double
    double val2=0.12345;
    t1.set(val2);
    BOOST_CHECK(t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::Double),int(t1.type()));

    // read float
    auto r2=t1.as<float>();
    BOOST_CHECK(r2.isValid());
    BOOST_TEST(val2==r2.takeValue());

    // read double
    auto r2_1=t1.as<double>();
    BOOST_CHECK(r2.isValid());
    BOOST_TEST(val2==r2_1.takeValue());

    // try to read int32_t
    auto r3=t1.as<int32_t>();
    BOOST_CHECK(!r3.isValid());
    BOOST_CHECK_EQUAL(r3.error().value(),static_cast<int>(base::BaseError::INVALID_TYPE));
}

BOOST_AUTO_TEST_CASE(StringValue)
{
    ConfigTreeValue t1;
    BOOST_CHECK(!t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.type()));

    // set string
    std::string val1{"Hello world"};
    t1.set(val1);
    BOOST_CHECK(t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::String),int(t1.type()));

    // read string
    auto r1=t1.as<std::string>();
    BOOST_CHECK(!static_cast<bool>(r1));
    BOOST_CHECK(r1.isValid());
    BOOST_CHECK_EQUAL(val1,r1.takeValue());

    // set const char*
    const char* val2="How are you?";
    t1.set(val2);
    BOOST_CHECK(t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::String),int(t1.type()));

    // read string
    auto r2=t1.as<std::string>();
    BOOST_CHECK(r2.isValid());
    BOOST_CHECK_EQUAL(std::string(val2),r2.takeValue());

    // set firect const char*
    t1.set("Hi!");
    BOOST_CHECK(t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::String),int(t1.type()));

    // read string
    auto r2_1=t1.as<std::string>();
    BOOST_CHECK(r2_1.isValid());
    BOOST_CHECK_EQUAL(std::string("Hi!"),*r2_1);

    // check if value is a const reference
    BOOST_CHECK(std::is_reference<decltype(r2_1.takeValue())>::value);
    BOOST_CHECK(std::is_const<std::remove_reference_t<decltype(r2_1.takeValue())>>::value);

    // try to read int32_t
    auto r3=t1.as<int32_t>();
    BOOST_CHECK(!r3.isValid());
    BOOST_CHECK_EQUAL(r3.error().value(),static_cast<int>(base::BaseError::INVALID_TYPE));
}

BOOST_AUTO_TEST_CASE(MapValue)
{
    const ConfigTreeValue constV1;
    auto constM1=constV1.asMap();
    BOOST_CHECK(static_cast<bool>(constM1));
    BOOST_CHECK(!constM1.isValid());

    //--------------------
    ConfigTreeValue t1;

    auto m1=t1.asMap();
    BOOST_CHECK(static_cast<bool>(m1));
    BOOST_CHECK(!m1.isValid());
    t1.toMap();
    auto m2=t1.asMap();
    BOOST_CHECK(!static_cast<bool>(m2));
    BOOST_CHECK(m2.isValid());

    auto& mm2=m2.takeValue();
    mm2["one"]=std::make_shared<ConfigTree>();
    mm2["one"]->set("Hello world!");

    const auto& t2=t1;
    auto m3=t2.asMap();
    BOOST_CHECK(!static_cast<bool>(m3));
    BOOST_CHECK(m3.isValid());
    const auto& mm3=m3.takeValue();
    BOOST_CHECK_EQUAL(std::string("Hello world!"),mm3.at("one")->as<std::string>().takeValue());
    BOOST_CHECK_EQUAL(std::string("Hello world!"),t2.asMap().takeValue().at("one")->as<std::string>().takeValue());
    BOOST_CHECK_EQUAL(std::string("Hello world!"),t2.asMap()->at("one")->as<std::string>().takeValue());
    auto rr=t2.asMap()->at("one")->as<std::string>();
    auto cc=*rr;
    BOOST_CHECK_EQUAL(std::string("Hello world!"),cc);
    BOOST_CHECK_EQUAL(std::string("Hello world!"),*t2.asMap()->at("one")->as<std::string>());
}

BOOST_AUTO_TEST_CASE(ValueTypes)
{
    auto boolT=config_tree::ValueType<bool>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Bool),int(boolT));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayBool),int(config_tree::ValueType<bool>::arrayId));

    auto int8T=config_tree::ValueType<int8_t>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(int8T));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayInt),int(config_tree::ValueType<int8_t>::arrayId));

    auto int16T=config_tree::ValueType<int16_t>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(int16T));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayInt),int(config_tree::ValueType<int16_t>::arrayId));

    auto int32T=config_tree::ValueType<int32_t>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(int32T));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayInt),int(config_tree::ValueType<int32_t>::arrayId));

    auto int64T=config_tree::ValueType<int64_t>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(int64T));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayInt),int(config_tree::ValueType<int64_t>::arrayId));

    auto uint8T=config_tree::ValueType<uint8_t>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(uint8T));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayInt),int(config_tree::ValueType<uint16_t>::arrayId));

    auto uint16T=config_tree::ValueType<uint16_t>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(uint16T));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayInt),int(config_tree::ValueType<uint16_t>::arrayId));

    auto uint32T=config_tree::ValueType<uint32_t>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(uint32T));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayInt),int(config_tree::ValueType<uint32_t>::arrayId));

    auto uint64T=config_tree::ValueType<uint64_t>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Int),int(uint64T));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayInt),int(config_tree::ValueType<uint16_t>::arrayId));

    auto floatT=config_tree::ValueType<float>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Double),int(floatT));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayDouble),int(config_tree::ValueType<float>::arrayId));

    auto doubleT=config_tree::ValueType<double>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::Double),int(doubleT));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayDouble),int(config_tree::ValueType<double>::arrayId));

    auto stringT=config_tree::ValueType<std::string>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::String),int(stringT));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayString),int(config_tree::ValueType<std::string>::arrayId));

    auto constCharT=config_tree::ValueType<const char*>::id;
    BOOST_CHECK_EQUAL(int(config_tree::Type::String),int(constCharT));
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayString),int(config_tree::ValueType<const char*>::arrayId));

    auto configTreeArrayT=config_tree::ValueType<ConfigTree>::arrayId;
    BOOST_CHECK_EQUAL(int(config_tree::Type::ArrayTree),int(configTreeArrayT));
}

BOOST_AUTO_TEST_CASE(ArrayValue)
{
    const ConfigTreeValue constV1;
    const auto& constA1=constV1.asArray<int32_t>();
    BOOST_CHECK(static_cast<bool>(constA1));
    BOOST_CHECK(!constA1.isValid());

    //--------------------
    ConfigTreeValue t1;

    auto a1=t1.asArray<int32_t>();
    BOOST_CHECK(static_cast<bool>(a1));
    BOOST_CHECK(!a1.isValid());
    t1.toArray<int32_t>();
    auto a2=t1.asArray<int32_t>();
    BOOST_CHECK(!static_cast<bool>(a2));
    BOOST_CHECK(a2.isValid());
    a2->reserve(100);

    const auto& t3=t1;
    auto a3=t3.asArray<int32_t>();
    BOOST_CHECK(!static_cast<bool>(a3));
    BOOST_CHECK(a3.isValid());
    BOOST_CHECK_EQUAL(100,a3->capacity());

    auto a4=t3.asArray<uint64_t>();
    BOOST_CHECK(!static_cast<bool>(a4));
    BOOST_CHECK(a4.isValid());
    BOOST_CHECK_EQUAL(100,a4->capacity());

    auto a5=t1.asArray<float>();
    BOOST_CHECK(static_cast<bool>(a5));
    BOOST_CHECK(!a5.isValid());

    auto a6=t1.asArray<bool>();
    BOOST_CHECK(static_cast<bool>(a6));
    BOOST_CHECK(!a6.isValid());

    auto a7=t1.asArray<std::string>();
    BOOST_CHECK(static_cast<bool>(a7));
    BOOST_CHECK(!a7.isValid());

    // check ec and exception
    t1.reset();
    BOOST_CHECK(!t1.isSet());
    BOOST_CHECK_EQUAL(int(config_tree::Type::None),int(t1.type()));
    auto intR1=t1.asArray<int32_t>();
    BOOST_CHECK(static_cast<bool>(intR1));
    BOOST_CHECK(!intR1.isValid());
    common::Error ec;
    auto val=t1.asArray<int32_t>(ec);
    std::ignore=val;
    BOOST_CHECK(static_cast<bool>(ec));
    BOOST_CHECK_EQUAL(ec.value(),static_cast<int>(base::BaseError::VALUE_NOT_SET));
    BOOST_CHECK_THROW(t1.asArrayThrows<int64_t>(),common::ErrorException);
    common::Error ec1;
    auto val1=t3.asArray<int32_t>(ec1);
    std::ignore=val1;
    BOOST_CHECK(static_cast<bool>(ec1));
    BOOST_CHECK_EQUAL(ec1.value(),static_cast<int>(base::BaseError::VALUE_NOT_SET));
    BOOST_CHECK_THROW(t3.asArrayThrows<int64_t>(),common::ErrorException);
}

BOOST_AUTO_TEST_CASE(ArrayInt)
{
    ConfigTreeValue t1;
    const auto& t2=t1;

    auto a1=t1.asArray<int32_t>();
    BOOST_CHECK(static_cast<bool>(a1));
    BOOST_CHECK(!a1.isValid());
    t1.toArray<int32_t>();
    auto a2=t1.asArray<int32_t>();
    BOOST_CHECK(!static_cast<bool>(a2));
    BOOST_CHECK(a2.isValid());
    a2->reserve(100);

    auto a3=t1.asArray<int32_t>();
    auto a4=t2.asArray<int64_t>();
    auto a5=t1.asArray<int64_t>();

    BOOST_CHECK(!static_cast<bool>(a3));
    BOOST_CHECK(a3.isValid());
    BOOST_CHECK_EQUAL(100,a3->capacity());
    BOOST_CHECK(!static_cast<bool>(a4));
    BOOST_CHECK(a4.isValid());
    BOOST_CHECK_EQUAL(100,a4->capacity());

    BOOST_CHECK_EQUAL(0,a3->size());
    BOOST_CHECK_EQUAL(0,a4->size());
    BOOST_CHECK(a3->empty());
    BOOST_CHECK(a4->empty());
    a3->append(1122);
    BOOST_CHECK_EQUAL(1,a3->size());
    BOOST_CHECK_EQUAL(1122,a3->at(0));
    BOOST_CHECK_EQUAL(1122,(*a3)[0]);
    BOOST_CHECK_EQUAL(1,a4->size());
    BOOST_CHECK_EQUAL(1122,a4->at(0));
    BOOST_CHECK_EQUAL(1122,(*a4)[0]);
    BOOST_CHECK(!a3->empty());
    BOOST_CHECK(!a4->empty());
    a3->emplaceBack(3344);
    BOOST_CHECK_EQUAL(2,a3->size());
    BOOST_CHECK_EQUAL(1122,a3->at(0));
    BOOST_CHECK_EQUAL(1122,(*a3)[0]);
    BOOST_CHECK_EQUAL(3344,a3->at(1));
    BOOST_CHECK_EQUAL(3344,(*a3)[1]);
    BOOST_CHECK_EQUAL(2,a4->size());
    BOOST_CHECK_EQUAL(1122,a4->at(0));
    BOOST_CHECK_EQUAL(1122,(*a4)[0]);
    BOOST_CHECK_EQUAL(3344,a4->at(1));
    BOOST_CHECK_EQUAL(3344,(*a4)[1]);
    a3->append(5678);
    a3->insert(1,3333);
    BOOST_CHECK_EQUAL(4,a3->size());
    BOOST_CHECK_EQUAL(1122,a3->at(0));
    BOOST_CHECK_EQUAL(1122,(*a3)[0]);
    BOOST_CHECK_EQUAL(3333,a3->at(1));
    BOOST_CHECK_EQUAL(3333,(*a3)[1]);
    BOOST_CHECK_EQUAL(3344,a3->at(2));
    BOOST_CHECK_EQUAL(3344,(*a3)[2]);
    BOOST_CHECK_EQUAL(4,a4->size());
    BOOST_CHECK_EQUAL(1122,a4->at(0));
    BOOST_CHECK_EQUAL(1122,(*a4)[0]);
    BOOST_CHECK_EQUAL(3333,a4->at(1));
    BOOST_CHECK_EQUAL(3333,(*a4)[1]);
    BOOST_CHECK_EQUAL(3344,a4->at(2));
    BOOST_CHECK_EQUAL(3344,(*a4)[2]);
    a3->emplace(2,5566);
    BOOST_CHECK_EQUAL(5,a3->size());
    BOOST_CHECK_EQUAL(1122,a3->at(0));
    BOOST_CHECK_EQUAL(1122,(*a3)[0]);
    BOOST_CHECK_EQUAL(3333,a3->at(1));
    BOOST_CHECK_EQUAL(3333,(*a3)[1]);
    BOOST_CHECK_EQUAL(5566,a3->at(2));
    BOOST_CHECK_EQUAL(5566,(*a3)[2]);
    BOOST_CHECK_EQUAL(3344,a3->at(3));
    BOOST_CHECK_EQUAL(3344,(*a3)[3]);
    BOOST_CHECK_EQUAL(5,a4->size());
    BOOST_CHECK_EQUAL(1122,a4->at(0));
    BOOST_CHECK_EQUAL(1122,(*a4)[0]);
    BOOST_CHECK_EQUAL(3333,a4->at(1));
    BOOST_CHECK_EQUAL(3333,(*a4)[1]);
    BOOST_CHECK_EQUAL(5566,a4->at(2));
    BOOST_CHECK_EQUAL(5566,(*a4)[2]);
    BOOST_CHECK_EQUAL(3344,a4->at(3));
    BOOST_CHECK_EQUAL(3344,(*a4)[3]);
    a3->set(2,7788);
    BOOST_CHECK_EQUAL(5,a3->size());
    BOOST_CHECK_EQUAL(1122,a3->at(0));
    BOOST_CHECK_EQUAL(1122,(*a3)[0]);
    BOOST_CHECK_EQUAL(3333,a3->at(1));
    BOOST_CHECK_EQUAL(3333,(*a3)[1]);
    BOOST_CHECK_EQUAL(7788,a3->at(2));
    BOOST_CHECK_EQUAL(7788,(*a3)[2]);
    BOOST_CHECK_EQUAL(3344,a3->at(3));
    BOOST_CHECK_EQUAL(3344,(*a3)[3]);
    BOOST_CHECK_EQUAL(5,a4->size());
    BOOST_CHECK_EQUAL(1122,a4->at(0));
    BOOST_CHECK_EQUAL(1122,(*a4)[0]);
    BOOST_CHECK_EQUAL(3333,a4->at(1));
    BOOST_CHECK_EQUAL(3333,(*a4)[1]);
    BOOST_CHECK_EQUAL(7788,a4->at(2));
    BOOST_CHECK_EQUAL(7788,(*a4)[2]);
    BOOST_CHECK_EQUAL(3344,a4->at(3));
    BOOST_CHECK_EQUAL(3344,(*a4)[3]);
    auto& arr=a5.value();
    arr[2]=9900;
    BOOST_CHECK_EQUAL(5,a3->size());
    BOOST_CHECK_EQUAL(1122,a3->at(0));
    BOOST_CHECK_EQUAL(1122,(*a3)[0]);
    BOOST_CHECK_EQUAL(3333,a3->at(1));
    BOOST_CHECK_EQUAL(3333,(*a3)[1]);
    BOOST_CHECK_EQUAL(9900,a3->at(2));
    BOOST_CHECK_EQUAL(9900,(*a3)[2]);
    BOOST_CHECK_EQUAL(3344,a3->at(3));
    BOOST_CHECK_EQUAL(3344,(*a3)[3]);
    BOOST_CHECK_EQUAL(5,a4->size());
    BOOST_CHECK_EQUAL(1122,a4->at(0));
    BOOST_CHECK_EQUAL(1122,(*a4)[0]);
    BOOST_CHECK_EQUAL(3333,a4->at(1));
    BOOST_CHECK_EQUAL(3333,(*a4)[1]);
    BOOST_CHECK_EQUAL(9900,a4->at(2));
    BOOST_CHECK_EQUAL(9900,(*a4)[2]);
    BOOST_CHECK_EQUAL(3344,a4->at(3));
    BOOST_CHECK_EQUAL(3344,(*a4)[3]);
    a3->erase(2);
    BOOST_CHECK_EQUAL(4,a3->size());
    BOOST_CHECK_EQUAL(1122,a3->at(0));
    BOOST_CHECK_EQUAL(1122,(*a3)[0]);
    BOOST_CHECK_EQUAL(3333,a3->at(1));
    BOOST_CHECK_EQUAL(3333,(*a3)[1]);
    BOOST_CHECK_EQUAL(3344,a3->at(2));
    BOOST_CHECK_EQUAL(3344,(*a3)[2]);
    BOOST_CHECK_EQUAL(4,a4->size());
    BOOST_CHECK_EQUAL(1122,a4->at(0));
    BOOST_CHECK_EQUAL(1122,(*a4)[0]);
    BOOST_CHECK_EQUAL(3333,a4->at(1));
    BOOST_CHECK_EQUAL(3333,(*a4)[1]);
    BOOST_CHECK_EQUAL(3344,a4->at(2));
    BOOST_CHECK_EQUAL(3344,(*a4)[2]);

    a3->resize(10);
    BOOST_CHECK_EQUAL(10,a3->size());
    BOOST_CHECK_EQUAL(10,a4->size());

    a3->clear();
    BOOST_CHECK_EQUAL(0,a3->size());
    BOOST_CHECK_EQUAL(0,a4->size());
    BOOST_CHECK(a3->empty());
    BOOST_CHECK(a4->empty());

    auto a6=t1.asArray<std::string>();
    BOOST_CHECK(static_cast<bool>(a6));
    BOOST_CHECK_EQUAL(a6.error().value(),static_cast<int>(base::BaseError::INVALID_TYPE));
}

BOOST_AUTO_TEST_CASE(ArrayString)
{
    ConfigTreeValue t1;
    const auto& t2=t1;
    t1.toArray<int32_t>();

    t1.toArray<std::string>();
    auto a7=t1.asArray<std::string>();
    BOOST_CHECK(a7.isValid());
    auto a8=t2.asArray<std::string>();
    BOOST_CHECK(a8.isValid());
    BOOST_CHECK_EQUAL(0,a7->size());
    BOOST_CHECK_EQUAL(0,a8->size());
    BOOST_CHECK(a7->empty());
    BOOST_CHECK(a8->empty());

    std::string str1("Hello world!");
    a7->append("Hello world!");
    BOOST_CHECK_EQUAL(1,a7->size());
    BOOST_CHECK_EQUAL(str1,a7->at(0));
    BOOST_CHECK_EQUAL(str1,(*a7)[0]);
    BOOST_CHECK_EQUAL(1,a8->size());
    BOOST_CHECK_EQUAL(str1,a8->at(0));
    BOOST_CHECK_EQUAL(str1,(*a8)[0]);
    BOOST_CHECK(!a7->empty());
    BOOST_CHECK(!a8->empty());
    std::string str2("Hi!");
    a7->emplaceBack("Hi!");
    BOOST_CHECK_EQUAL(str1,a7->at(0));
    BOOST_CHECK_EQUAL(str1,(*a7)[0]);
    BOOST_CHECK_EQUAL(str1,a8->at(0));
    BOOST_CHECK_EQUAL(str1,(*a8)[0]);
    BOOST_CHECK_EQUAL(2,a7->size());
    BOOST_CHECK_EQUAL(str2,a7->at(1));
    BOOST_CHECK_EQUAL(str2,(*a7)[1]);
    BOOST_CHECK_EQUAL(2,a8->size());
    BOOST_CHECK_EQUAL(str2,a8->at(1));
    BOOST_CHECK_EQUAL(str2,(*a8)[1]);
    std::string str3("How are you!");
    std::string str4("How are you!");
    a7->set(0,std::move(str3));
    BOOST_CHECK_EQUAL(str4,a7->at(0));
    BOOST_CHECK_EQUAL(str4,(*a7)[0]);
    BOOST_CHECK_EQUAL(str4,a8->at(0));
    BOOST_CHECK_EQUAL(str4,(*a8)[0]);
    std::string str5("What's up?");
    std::string str6("What's up?");
    (*a7)[0]=std::move(str5);
    BOOST_CHECK_EQUAL(str6,a7->at(0));
    BOOST_CHECK_EQUAL(str6,(*a7)[0]);
    BOOST_CHECK_EQUAL(str6,a8->at(0));
    BOOST_CHECK_EQUAL(str6,(*a8)[0]);
    std::string str7("Bye!");
    a7->emplace(0,"Bye!");
    BOOST_CHECK_EQUAL(str7,a7->at(0));
    BOOST_CHECK_EQUAL(str7,(*a7)[0]);
    BOOST_CHECK_EQUAL(str7,a8->at(0));
    BOOST_CHECK_EQUAL(str7,(*a8)[0]);
    BOOST_CHECK_EQUAL(3,a7->size());
    BOOST_CHECK_EQUAL(str6,a7->at(1));
    BOOST_CHECK_EQUAL(str6,(*a7)[1]);
    BOOST_CHECK_EQUAL(3,a8->size());
    BOOST_CHECK_EQUAL(str6,a8->at(1));
    BOOST_CHECK_EQUAL(str6,(*a8)[1]);
    std::string str8("Chiao!");
    std::string str9("Chiao!");
    a7->insert(0,std::move(str8));
    BOOST_CHECK_EQUAL(str9,a7->at(0));
    BOOST_CHECK_EQUAL(str9,(*a7)[0]);
    BOOST_CHECK_EQUAL(str9,a8->at(0));
    BOOST_CHECK_EQUAL(str9,(*a8)[0]);
    BOOST_CHECK_EQUAL(4,a7->size());
    BOOST_CHECK_EQUAL(str7,a7->at(1));
    BOOST_CHECK_EQUAL(str7,(*a7)[1]);
    BOOST_CHECK_EQUAL(4,a8->size());
    BOOST_CHECK_EQUAL(str7,a8->at(1));
    BOOST_CHECK_EQUAL(str7,(*a8)[1]);
    std::string str10("See you!");
    a7->insert(0,str10);
    BOOST_CHECK_EQUAL(str10,a7->at(0));
    BOOST_CHECK_EQUAL(str10,(*a7)[0]);
    BOOST_CHECK_EQUAL(str10,a8->at(0));
    BOOST_CHECK_EQUAL(str10,(*a8)[0]);
    BOOST_CHECK_EQUAL(5,a7->size());
    BOOST_CHECK_EQUAL(str9,a7->at(1));
    BOOST_CHECK_EQUAL(str9,(*a7)[1]);
    BOOST_CHECK_EQUAL(5,a8->size());
    BOOST_CHECK_EQUAL(str9,a8->at(1));
    BOOST_CHECK_EQUAL(str9,(*a8)[1]);
}

BOOST_AUTO_TEST_SUITE_END()
