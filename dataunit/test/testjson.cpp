#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <iostream>

#include <hatn/common/thread.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/makeshared.h>

#include <hatn/common/logger.h>
#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/memorypool/newdeletepool.h>

#include <hatn/thirdparty/base64/base64.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/types.h>

#include <hatn/test/multithreadfixture.h>

#include <hatn/dataunit/unitmacros.h>

#include "simpleunitdeclaration.h"
#include "testunitdeclarations2.h"
#include "testunitdeclarations3.h"
#include "testunitdeclarations4.h"
#include "testunitdeclarations5.h"
#include "testunitdeclarations6.h"
#include "testunitdeclarations7.h"
#include "testunitdeclarations8.h"

//#define HATN_TEST_LOG_CONSOLE

namespace {
const bool PrettyFormat=true;
const int MaxDecimalPlaces=4;

void setLogHandler()
{
    auto handler=[](const ::hatn::common::FmtAllocatedBufferChar &s)
    {
        #ifdef HATN_TEST_LOG_CONSOLE
            std::cout<<::hatn::common::lib::toStringView(s)<<std::endl;
        #endif

        auto str=::hatn::common::fmtBufToString(s);
        str=str.substr(14,str.length());

        BOOST_CHECK_EQUAL(str,"t[main] WARNING:dataunit:json-serialize :: Failed to serialize to JSON DataUnit message all_types: required field type_int8_required is not set");
    };

    ::hatn::common::Logger::setOutputHandler(handler);
    ::hatn::common::Logger::setFatalLogHandler(handler);
}

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
        if (!::hatn::common::Logger::isRunning())
        {
            ::hatn::common::Logger::setFatalTracing(false);
            ::hatn::common::Logger::setDefaultVerbosity(::hatn::common::LoggerVerbosity::WARNING);

            ::hatn::common::Logger::start(false);
        }
    }

    ~Env()
    {
        ::hatn::common::Logger::stop();
        ::hatn::dataunit::AllocatorFactory::resetDefault();
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};
}

BOOST_AUTO_TEST_SUITE(TestJson)

namespace {
template <typename t_int8,typename traits,typename t> void typeChecks(t& allTypes)
{
    const auto& fields=traits::fields;

    // check count
    BOOST_CHECK_EQUAL(allTypes.fieldCount(),15);
    // check field position
    auto int8Pos=allTypes.template fieldPos<t_int8>();
    BOOST_CHECK_EQUAL(int8Pos,1);

    auto& f1=allTypes.field(fields.type_bool);
    BOOST_CHECK(!f1.get());
    BOOST_CHECK(!f1.isSet());
    f1.set(true);
    const auto& f1_=allTypes.field(fields.type_bool);
    BOOST_CHECK(f1_.get());
    BOOST_CHECK(f1_.isSet());

    int8_t val_int8=-10;
    auto& f2=allTypes.template field<t_int8>();
    BOOST_CHECK_EQUAL(static_cast<int>(f2.get()),0);
    BOOST_CHECK(!f2.isSet());
    f2.set(val_int8);
    BOOST_CHECK_EQUAL(static_cast<int>(f2.get()),static_cast<int>(val_int8));
    BOOST_CHECK(f2.isSet());
    const auto& f2_=allTypes.template field<t_int8>();
    BOOST_CHECK(f2_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f2_.get()),static_cast<int>(val_int8));
    // check field by position
    const auto& f2__=allTypes.template field<1>();
    BOOST_CHECK(f2__.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f2__.get()),static_cast<int>(val_int8));

    int16_t val_int16=-2032;
    auto& f3=allTypes.field(fields.type_int16);
    BOOST_CHECK_EQUAL(static_cast<int>(f3.get()),0);
    BOOST_CHECK(!f3.isSet());
    f3.set(val_int16);
    BOOST_CHECK_EQUAL(static_cast<int>(f3.get()),static_cast<int>(val_int16));
    BOOST_CHECK(f3.isSet());
    const auto& f3_=allTypes.field(fields.type_int16);
    BOOST_CHECK(f3_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f3_.get()),static_cast<int>(val_int16));

    int32_t val_int32=0x1F810110;
    auto& f4=allTypes.field(fields.type_int32);
    BOOST_CHECK_EQUAL(static_cast<int>(f4.get()),0);
    BOOST_CHECK(!f4.isSet());
    f4.set(val_int32);
    BOOST_CHECK_EQUAL(static_cast<int>(f4.get()),static_cast<int>(val_int32));
    BOOST_CHECK(f4.isSet());
    const auto& f4_=allTypes.field(fields.type_int32);
    BOOST_CHECK(f4_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f4_.get()),static_cast<int>(val_int32));

    uint8_t val_uint8=10;
    auto& f5=allTypes.field(fields.type_uint8);
    BOOST_CHECK_EQUAL(static_cast<int>(f5.get()),0);
    BOOST_CHECK(!f5.isSet());
    f5.set(val_uint8);
    BOOST_CHECK_EQUAL(static_cast<int>(f5.get()),static_cast<int>(val_uint8));
    BOOST_CHECK(f5.isSet());
    const auto& f5_=allTypes.field(fields.type_uint8);
    BOOST_CHECK(f5_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f5_.get()),static_cast<int>(val_uint8));

    uint16_t val_uint16=0xF810;
    auto& f6=allTypes.field(fields.type_uint16);
    BOOST_CHECK_EQUAL(static_cast<int>(f6.get()),0);
    BOOST_CHECK(!f6.isSet());
    f6.set(val_uint16);
    BOOST_CHECK_EQUAL(static_cast<int>(f6.get()),static_cast<int>(val_uint16));
    BOOST_CHECK(f6.isSet());
    const auto& f6_=allTypes.field(fields.type_uint16);
    BOOST_CHECK(f6_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f6_.get()),static_cast<int>(val_uint16));

    uint32_t val_uint32=0x1F810110;
    auto& f7=allTypes.field(fields.type_uint32);
    BOOST_CHECK_EQUAL(static_cast<int>(f7.get()),0);
    BOOST_CHECK(!f7.isSet());
    f7.set(val_uint32);
    BOOST_CHECK_EQUAL(static_cast<int>(f7.get()),static_cast<int>(val_uint32));
    BOOST_CHECK(f7.isSet());
    const auto& f7_=allTypes.field(fields.type_uint32);
    BOOST_CHECK(f7_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f7_.get()),static_cast<int>(val_uint32));

    float val_float=253245.7686f;
    auto& f8=allTypes.field(fields.type_float);
    BOOST_CHECK_CLOSE(f8.get(),0.0f,0.0001f);
    f8.set(val_float);
    BOOST_CHECK_CLOSE(f8.get(),val_float,0.0001);
    BOOST_CHECK(f8.isSet());
    const auto& f8_=allTypes.field(fields.type_float);
    BOOST_CHECK_CLOSE(f8_.get(),val_float,0.0001);
    BOOST_CHECK(f8_.isSet());

    float val_double=253245.7686f;
    auto& f9=allTypes.field(fields.type_double);
    BOOST_CHECK_CLOSE(f9.get(),0,0.0001);
    f9.set(val_double);
    BOOST_CHECK_CLOSE(f9.get(),val_double,0.0001);
    BOOST_CHECK(f9.isSet());
    const auto& f9_=allTypes.field(fields.type_double);
    BOOST_CHECK_CLOSE(f9_.get(),val_double,0.0001);
    BOOST_CHECK(f9_.isSet());
HATN_DATAUNIT_NAMESPACE_END

BOOST_FIXTURE_TEST_CASE(TestBasic,Env)
{
    setLogHandler();

    using traits=all_types::traits;
    using type=traits::type;
    const auto& fields=traits::fields;

    type allTypes;

    auto& aaa=allTypes.field<1>();
    std::ignore=aaa;

    typeChecks<decltype(fields.type_int8),traits>(allTypes);
    std::string json=allTypes.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (required field not set):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,"");

    auto& required=allTypes.field(fields.type_int8_required);
    required.set(89);
    json=allTypes.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (required field set):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,
                      "{\n"
                      "    \"type_bool\": true,\n"
                      "    \"type_int8\": -10,\n"
                      "    \"type_int16\": -2032,\n"
                      "    \"type_int32\": 528548112,\n"
                      "    \"type_uint8\": 10,\n"
                      "    \"type_uint16\": 63504,\n"
                      "    \"type_uint32\": 528548112,\n"
                      "    \"type_float\": 253245.7656,\n"
                      "    \"type_double\": 253245.7656,\n"
                      "    \"type_int8_required\": 89\n"
                      "}"
                      );

    type allTypesCopy;
    BOOST_CHECK(allTypesCopy.loadFromJSON(json));
    auto jsonCheck=allTypesCopy.toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);

    auto& str=allTypes.field(fields.type_string);
    str.set("Hello world");
    json=allTypes.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (string field set):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,
                      "{\n"
                      "    \"type_bool\": true,\n"
                      "    \"type_int8\": -10,\n"
                      "    \"type_int16\": -2032,\n"
                      "    \"type_int32\": 528548112,\n"
                      "    \"type_uint8\": 10,\n"
                      "    \"type_uint16\": 63504,\n"
                      "    \"type_uint32\": 528548112,\n"
                      "    \"type_float\": 253245.7656,\n"
                      "    \"type_double\": 253245.7656,\n"
                      "    \"type_string\": \"Hello world\",\n"
                      "    \"type_int8_required\": 89\n"                      "}"
                      );

    BOOST_CHECK(allTypesCopy.loadFromJSON(json));
    jsonCheck=allTypesCopy.toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);


    auto& bytes=allTypes.field(fields.type_bytes);
    bytes.set("Hello world");
    json=allTypes.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (bytes field set):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,
                      "{\n"
                      "    \"type_bool\": true,\n"
                      "    \"type_int8\": -10,\n"
                      "    \"type_int16\": -2032,\n"
                      "    \"type_int32\": 528548112,\n"
                      "    \"type_uint8\": 10,\n"
                      "    \"type_uint16\": 63504,\n"
                      "    \"type_uint32\": 528548112,\n"
                      "    \"type_float\": 253245.7656,\n"
                      "    \"type_double\": 253245.7656,\n"
                      "    \"type_string\": \"Hello world\",\n"
                      "    \"type_bytes\": \"SGVsbG8gd29ybGQ=\",\n"
                      "    \"type_int8_required\": 89\n"
                      "}"
                    );

    BOOST_CHECK(allTypesCopy.loadFromJSON(json));
    jsonCheck=allTypesCopy.toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);

    empty_unit::traits::type empt;
    json=empt.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (empty unit):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,
                      "{}"
                    );

    empty_unit::traits::type emptCopy;
    BOOST_CHECK(emptCopy.loadFromJSON(json));
    jsonCheck=emptCopy.toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);
}

namespace {
template <typename T> void fillUnitFields(T& allTypes, int n)
{
    // check field position
    auto int8Pos=allTypes.fieldPos(all_types::type_int8);
    BOOST_CHECK_EQUAL(int8Pos,1);

    auto& f1=allTypes.field(all_types::type_bool);
    BOOST_CHECK(!f1.get());
    BOOST_CHECK(!f1.isSet());
    f1.set(true);

    int8_t val_int8=-10+n;
    auto& f2=allTypes.field(all_types::type_int8);
    BOOST_CHECK_EQUAL(static_cast<int>(f2.get()),0);
    BOOST_CHECK(!f2.isSet());
    f2.set(val_int8);
    BOOST_CHECK_EQUAL(static_cast<int>(f2.get()),static_cast<int>(val_int8));
    BOOST_CHECK(f2.isSet());

    int8_t val_int8_required=123+n;
    auto& f2_2=allTypes.field(all_types::type_int8_required);
    BOOST_CHECK_EQUAL(static_cast<int>(f2_2.get()),0);
    BOOST_CHECK(!f2_2.isSet());
    f2_2.set(val_int8_required);
    BOOST_CHECK_EQUAL(static_cast<int>(f2_2.get()),static_cast<int>(val_int8_required));
    BOOST_CHECK(f2_2.isSet());

    int16_t val_int16=0xF810+n;
    auto& f3=allTypes.field(all_types::type_int16);
    BOOST_CHECK_EQUAL(static_cast<int>(f3.get()),0);
    BOOST_CHECK(!f3.isSet());
    f3.set(val_int16);
    BOOST_CHECK_EQUAL(static_cast<int>(f3.get()),static_cast<int>(val_int16));
    BOOST_CHECK(f3.isSet());

    int32_t val_int32=0x1F810110+n;
    auto& f4=allTypes.field(all_types::type_int32);
    BOOST_CHECK_EQUAL(static_cast<int>(f4.get()),0);
    BOOST_CHECK(!f4.isSet());
    f4.set(val_int32);
    BOOST_CHECK_EQUAL(static_cast<int>(f4.get()),static_cast<int>(val_int32));
    BOOST_CHECK(f4.isSet());

    uint8_t val_uint8=10+n;
    auto& f5=allTypes.field(all_types::type_uint8);
    BOOST_CHECK_EQUAL(static_cast<int>(f5.get()),0);
    BOOST_CHECK(!f5.isSet());
    f5.set(val_uint8);
    BOOST_CHECK_EQUAL(static_cast<int>(f5.get()),static_cast<int>(val_uint8));
    BOOST_CHECK(f5.isSet());

    uint16_t val_uint16=0xF810+n;
    auto& f6=allTypes.field(all_types::type_uint16);
    BOOST_CHECK_EQUAL(static_cast<int>(f6.get()),0);
    BOOST_CHECK(!f6.isSet());
    f6.set(val_uint16);
    BOOST_CHECK_EQUAL(static_cast<int>(f6.get()),static_cast<int>(val_uint16));
    BOOST_CHECK(f6.isSet());

    uint32_t val_uint32=0x1F810110+n;
    auto& f7=allTypes.field(all_types::type_uint32);
    BOOST_CHECK_EQUAL(static_cast<int>(f7.get()),0);
    BOOST_CHECK(!f7.isSet());
    f7.set(val_uint32);
    BOOST_CHECK_EQUAL(static_cast<int>(f7.get()),static_cast<int>(val_uint32));
    BOOST_CHECK(f7.isSet());

    float val_float=253245.7686f+static_cast<float>(n);
    auto& f8=allTypes.field(all_types::type_float);
    BOOST_CHECK_CLOSE(f8.get(),0.0f,0.0001f);
    f8.set(val_float);
    BOOST_CHECK_CLOSE(f8.get(),val_float,0.0001f);
    BOOST_CHECK(f8.isSet());

    float val_double=253245.7686f+static_cast<float>(n);
    auto& f9=allTypes.field(all_types::type_double);
    BOOST_CHECK_CLOSE(f9.get(),0.0f,0.0001f);
    f9.set(val_double);
    BOOST_CHECK_CLOSE(f9.get(),val_double,0.0001f);
    BOOST_CHECK(f9.isSet());
}

template <typename traits> void checkSubUnit()
{
    typename traits::type unit1;
    const auto& fields=traits::fields;

    static_assert(traits::type::hasField(fields.f0),"");

    auto& field1=unit1.field(fields.f0);
    BOOST_CHECK(!field1.isSet());

    auto* obj1=field1.mutableValue();
    BOOST_REQUIRE(field1.isSet());
    BOOST_REQUIRE(obj1!=nullptr);
    fillUnitFields(*obj1,0);

    auto json=unit1.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (subunit):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,
                      "{\n"
                      "    \"f0\": {\n"
                      "        \"type_bool\": true,\n"
                      "        \"type_int8\": -10,\n"
                      "        \"type_int16\": -2032,\n"
                      "        \"type_int32\": 528548112,\n"
                      "        \"type_uint8\": 10,\n"
                      "        \"type_uint16\": 63504,\n"
                      "        \"type_uint32\": 528548112,\n"
                      "        \"type_float\": 253245.7656,\n"
                      "        \"type_double\": 253245.7656,\n"
                      "        \"type_int8_required\": 123\n"
                      "    }\n"
                      "}"
                );

    typename traits::type unit2;
    BOOST_REQUIRE(unit2.loadFromJSON(json));
    auto jsonCheck=unit2.toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);
}

template <typename traits> void checkRepeatedUnit()
{
    const auto& fields=traits::fields;
    typename traits::type unit1;
    auto& field1=unit1.field(fields.f0);

    static_assert(traits::type::hasField(fields.f0),"");

    int n=1;

    for (int i=0;i<n;i++)
    {
        auto& obj=field1.createAndAddValue();
        fillUnitFields(*obj.mutableValue(),i);
    }

    auto json=unit1.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (repeated fields):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,
                      "{\n"
                      "    \"f0\": [\n"
                      "        {\n"
                      "            \"type_bool\": true,\n"
                      "            \"type_int8\": -10,\n"
                      "            \"type_int16\": -2032,\n"
                      "            \"type_int32\": 528548112,\n"
                      "            \"type_uint8\": 10,\n"
                      "            \"type_uint16\": 63504,\n"
                      "            \"type_uint32\": 528548112,\n"
                      "            \"type_float\": 253245.7656,\n"
                      "            \"type_double\": 253245.7656,\n"
                      "            \"type_int8_required\": 123\n"
                      "        }\n"
                      "    ]\n"
                      "}"
                );

    typename traits::type unit2;
    BOOST_REQUIRE(unit2.loadFromJSON(json));
    auto jsonCheck=unit2.toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);
HATN_DATAUNIT_NAMESPACE_END

BOOST_FIXTURE_TEST_CASE(TestSubsUnit,Env)
{
    checkSubUnit<embedded_unit::traits>();
}

BOOST_FIXTURE_TEST_CASE(TestRepeatedUnit,Env)
{
    checkRepeatedUnit<wire_unit_repeated::traits>();
}

BOOST_FIXTURE_TEST_CASE(TestRepeatedUint32,Env)
{
    wire_uint32_repeated::type unit1;
    auto& field1=unit1.field(wire_uint32_repeated::f0);
    BOOST_CHECK(!field1.isSet());
    field1.addValue(123);
    field1.addValue(456);
    field1.addValue(789);
    field1.addValue(555);
    BOOST_CHECK(field1.isSet());
    BOOST_CHECK_EQUAL(field1.count(),4);
    BOOST_CHECK_EQUAL(field1.value(0),123);
    BOOST_CHECK_EQUAL(field1.value(1),456);
    BOOST_CHECK_EQUAL(field1.value(2),789);
    BOOST_CHECK_EQUAL(field1.value(3),555);

    auto json=unit1.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (repeated uint32):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,
                      "{\n"
                      "    \"f0\": [\n"
                      "        123,\n"
                      "        456,\n"
                      "        789,\n"
                      "        555\n"
                      "    ]\n"
                      "}"
            );

    decltype(unit1) unit2;
    BOOST_REQUIRE(unit2.loadFromJSON(json));
    auto jsonCheck=unit2.toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);
}

namespace {
template <typename traits> void checkRepeatedDouble()
{
    typename traits::type unit1;
    auto& field1=unit1.field(traits::fields.f0);
    field1.addValue(0.);
    field1.addValue(10.2);
    field1.addValue(20.3);
    field1.addValue(30.4);
    field1.addValue(130.);

    BOOST_CHECK(field1.isSet());
    BOOST_CHECK_EQUAL(field1.count(),5);
    BOOST_CHECK_CLOSE(field1.value(0),0,0.0001);
    BOOST_CHECK_CLOSE(field1.value(1),10.2,0.0001);
    BOOST_CHECK_CLOSE(field1.value(2),20.3,0.0001);
    BOOST_CHECK_CLOSE(field1.value(3),30.4,0.0001);
    BOOST_CHECK_CLOSE(field1.value(4),130.,0.0001);

    auto json=unit1.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (repeated double):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,
                      "{\n"
                      "    \"f0\": [\n"
                      "        0.0,\n"
                      "        10.2,\n"
                      "        20.3,\n"
                      "        30.4,\n"
                      "        130.0\n"
                      "    ]\n"
                      "}"
                );

    decltype(unit1) unit2;
    BOOST_REQUIRE(unit2.loadFromJSON(json));
    auto jsonCheck=unit2.toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);
HATN_DATAUNIT_NAMESPACE_END

BOOST_FIXTURE_TEST_CASE(TestRepeatedDouble,Env)
{
    checkRepeatedDouble<wire_double_repeated::traits>();
}

namespace {

using MemoryResource=::hatn::common::memorypool::NewDeletePoolResource;
using MemoryPool=::hatn::common::memorypool::NewDeletePool;

template <typename traits> void checkRepeatedBytes(bool shared=false)
{
    std::ignore=shared;
    auto resource=std::make_unique<MemoryResource>();
    ::hatn::common::pmr::polymorphic_allocator<::hatn::common::ByteArrayManaged> byteArrayAllocator(resource.get());
    {
        typename traits::type unit1;
        auto& field1=unit1.field(traits::fields.f0);
        field1.resize(3);
        BOOST_REQUIRE(field1.isSet());
        BOOST_REQUIRE_EQUAL(field1.count(),3);

        auto& arr1=field1.value(0);
        arr1.setSharedByteArray(::hatn::common::allocateShared<::hatn::common::ByteArrayManaged>(byteArrayAllocator));

        ::hatn::common::ByteArray* arr1_1=arr1.buf();
        arr1_1->resize(100);
        for (size_t i=0;i<arr1_1->size();i++)
        {
            auto d=i+i*i;
            (*arr1_1)[i]=static_cast<char>(d);
        }

        ::hatn::common::ByteArray* arr1_2=field1.value(1).buf();
        arr1_2->resize(100);
        for (size_t i=0;i<arr1_2->size();i++)
        {
            auto d=i+i*i+1234;
            (*arr1_2)[i]=static_cast<char>(d);
        }

        ::hatn::common::ByteArray* arr1_3=field1.value(2).buf();
        arr1_3->resize(200);
        for (size_t i=0;i<arr1_3->size();i++)
        {
            auto d=i+i*i+5678;
            (*arr1_3)[i]=static_cast<char>(d);
        }

        auto json=unit1.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
        std::cerr<<"JSON (repeated bytes):"<<std::endl<<json<<std::endl;
#endif
        BOOST_CHECK_EQUAL(json,
              "{\n"
              "    \"f0\": [\n"
              "        \"AAIGDBQeKjhIWm6EnLbS8BAyVnykzvooWIq+9CxmouAgYqbsNH7KGGi6DmS8FnLQMJL2XMQumgh46l7UTMZCwEDCRsxU3mr4iBquRNx2ErBQ8pY85I466JhK/rRsJuKgYCLmrA==\",\n"
              "        \"0tTY3ubw/AoaLEBWboikwuIEKE52oMz6KlyQxv44dLLyNHi+BlCc6jqM4DaO6ESiAmTILpYAbNpKvDCmHpgUkhKUGJ4msDzKWuyAFq5I5IIixGgOtmAMumoc0IY++LRyMvS4fg==\",\n"
              "        \"LjA0OkJMWGZ2iJyyyuQAHj5ghKrS/ChWhrjsIlqU0A5OkNQaYqz4RpboPJLqRKD+XsAkivJcyDamGIwCevRw7m7wdPqCDJgmtkjccgqkQN5+IMRqErxoFsZ4LOKaVBDOjlAU2qJsOAbWqHxSKgTgvp6AZEoyHAj25tjMwrq0sK6usLS6wszY5vYIHDJKZICevuAEKlJ8qNYGOGyi2hRQjs4QVJriLHjGFmi8EmrEIH7eQKQKctxItiaYDIL6dPBu7nD0egKMGKY=\"\n"
              "    ]\n"
              "}"
        );

        decltype(unit1) unit2;
        BOOST_REQUIRE(unit2.loadFromJSON(json));
        auto jsonCheck=unit2.toString(PrettyFormat,MaxDecimalPlaces);
        BOOST_CHECK_EQUAL(json,jsonCheck);
    HATN_DATAUNIT_NAMESPACE_END

template <typename traits> void checkRepeatedString(bool fixed)
{
    {
        typename traits::type unit1;
        auto& field1=unit1.field(traits::fields.f0);
        field1.addValue("Value1");
        field1.addValue("Value2");
        field1.addValue("Value3");
        field1.addValue("Value4");

        auto json=unit1.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
        std::cerr<<"JSON (repeated "<<(fixed?"fixed":"")<<" string):"<<std::endl<<json<<std::endl;
#else
        std::ignore=fixed;
#endif
        BOOST_CHECK_EQUAL(json,
              "{\n"
              "    \"f0\": [\n"
              "        \"Value1\",\n"
              "        \"Value2\",\n"
              "        \"Value3\",\n"
              "        \"Value4\"\n"
              "    ]\n"
              "}"
        );

        decltype(unit1) unit2;
        BOOST_REQUIRE(unit2.loadFromJSON(json));
        auto jsonCheck=unit2.toString(PrettyFormat,MaxDecimalPlaces);
        BOOST_CHECK_EQUAL(json,jsonCheck);
    HATN_DATAUNIT_NAMESPACE_END
}

BOOST_FIXTURE_TEST_CASE(TestRepeatedBytes,Env)
{
    checkRepeatedBytes<wire_bytes_repeated::traits>();
}

BOOST_FIXTURE_TEST_CASE(TestRepeatedString,Env)
{
    checkRepeatedString<wire_fixed_string_repeated::traits>(false);
    checkRepeatedString<wire_string_repeated::traits>(true);
}

BOOST_FIXTURE_TEST_CASE(TestDefaultFields,Env)
{
    default_fields::type unit1;
    auto& field1=unit1.field(default_fields::type_double_repeated);
    field1.addValues(7);
    auto& field2=unit1.field(default_fields::type_enum_repeated);
    field2.addValues(5);

    auto json=unit1.toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON (default field):"<<std::endl<<json<<std::endl;
#endif
    BOOST_CHECK_EQUAL(json,
          "{\n"
          "    \"type_bool\": true,\n"
          "    \"type_int32\": 1000,\n"
          "    \"type_float\": 1010.215,\n"
          "    \"type_double\": 3020.1123,\n"
          "    \"type_enum\": 2,\n"
          "    \"type_double_repeated\": [\n"
          "        3030.3,\n"
          "        3030.3,\n"
          "        3030.3,\n"
          "        3030.3,\n"
          "        3030.3,\n"
          "        3030.3,\n"
          "        3030.3\n"
          "    ],\n"
          "    \"type_enum_repeated\": [\n"
          "        1,\n"
          "        1,\n"
          "        1,\n"
          "        1,\n"
          "        1\n"
          "    ]\n"
          "}"
    );

    decltype(unit1) unit2;
    BOOST_REQUIRE(unit2.loadFromJSON(json));
    auto jsonCheck=unit2.toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);
}

BOOST_AUTO_TEST_SUITE_END()
