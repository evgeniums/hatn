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

#include <hatn/dataunit/syntax.h>

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
}
}

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
                      "    \"type_int8_required\": 89\n"
                      "}"
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
        auto& obj=field1.createAndAppendValue();
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
}
}

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
    field1.appendValue(123);
    field1.appendValue(456);
    field1.appendValue(789);
    field1.appendValue(555);
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
    field1.appendValue(0.);
    field1.appendValue(10.2);
    field1.appendValue(20.3);
    field1.appendValue(30.4);
    field1.appendValue(130.);

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
}
}

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
    }
}

template <typename traits> void checkRepeatedString(bool fixed)
{
    {
        typename traits::type unit1;
        auto& field1=unit1.field(traits::fields.f0);
        field1.appendValue("Value1");
        field1.appendValue("Value2");
        field1.appendValue("Value3");
        field1.appendValue("Value4");

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
    }
}
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
    field1.appendValues(7);
    auto& field2=unit1.field(default_fields::type_enum_repeated);
    field2.appendValues(5);

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

BOOST_FIXTURE_TEST_CASE(TestUnitTree,::hatn::test::MultiThreadFixture)
{
    auto factory=hatn::dataunit::AllocatorFactory::getDefault();
    auto unit=factory->createObject<tree::managed>(factory);

    auto unit1=factory->createObject<tree::managed>(factory);
    auto unit2=factory->createObject<tree::managed>(factory);
    auto unit3=factory->createObject<tree::managed>(factory);
    auto unit4=factory->createObject<tree::managed>(factory);

    unit2->field(tree::children).appendValues(2);
    unit3->field(tree::children).appendValues(3);
    unit4->field(tree::children).appendValues(4);

    unit2->field(tree::f0).set(15);
    unit3->field(tree::f0).set(25);
    unit4->field(tree::f0).set(35);

    unit->field(tree::children).appendValue(unit1);
    unit->field(tree::children).appendValue(unit2);
    unit->field(tree::children).appendValue(unit3);
    unit->field(tree::children).appendValue(unit4);

    auto json=unit->toString(PrettyFormat,MaxDecimalPlaces);
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<"JSON:"<<std::endl<<json<<std::endl;
#endif

    auto unitDeser=factory->createObject<tree::managed>(factory);
    BOOST_REQUIRE(unitDeser->loadFromJSON(json));
    auto jsonCheck=unitDeser->toString(PrettyFormat,MaxDecimalPlaces);
    BOOST_CHECK_EQUAL(json,jsonCheck);
}

BOOST_FIXTURE_TEST_CASE(TestParseError,::hatn::test::MultiThreadFixture)
{
    hatn::Error ec;
    bool ok;

    std::string str1="{\"str_field1\":100}";
    ju1::type obj1;
    ok=obj1.loadFromJSON(str1,ec);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(ec.message(),"failed to parse JSON: Terminate parsing due to Handler error. at offset 14, key str_field1: invalid format of JSON field");

    std::string str2="{\"sub2\":{\"str_field1\":\"some string\"}, \"sub1\":{\"int_field1\":\"some string\"}}";
    ju2::type obj2;
    ok=obj2.loadFromJSON(str2,ec);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(ec.message(),"failed to parse JSON: Terminate parsing due to Handler error. at offset 72, key int_field1: invalid format of JSON field");
}

//! Keys that the unit does not declare are skipped whatever their value is: a scalar of any kind, null, an object or an
//! array with anything inside, at the top level and inside a declared subunit, before, between and after declared keys.
//! (Before, only an object value was tolerated; a scalar value made the whole load fail.)
BOOST_FIXTURE_TEST_CASE(TestUnknownKeysAreSkipped,::hatn::test::MultiThreadFixture)
{
    // the same unit once with unknown keys of every kind and once clean: they must load to the same value
    const std::string dirty=
        "{"
        "\"u_str\":\"some string\","
        "\"u_esc\":\"quote \\\" slash \\\\ \\u0436 \\n\","
        "\"u_int\":-12,"
        "\"u_big\":18446744073709551615,"
        "\"u_double\":1.5e3,"
        "\"u_true\":true,\"u_false\":false,\"u_null\":null,"
        "\"sub1\":{"
            "\"u_in_sub\":{\"a\":[1,2,{\"b\":null,\"c\":[[],{}]}]},"
            "\"str_field1\":\"inner\","
            "\"u_scalar_in_sub\":true,"
            "\"int_field1\":5,"
            "\"u_array_in_sub\":[\"x\",7,false]"
        "},"
        "\"u_obj\":{\"deep\":{\"deeper\":{\"str_field1\":\"must not leak\",\"sub1\":{\"int_field1\":99}}}},"
        "\"u_arr\":[1,\"two\",[3,[4]],{\"five\":5},[],{}],"
        "\"u_empty_obj\":{},\"u_empty_arr\":[],"
        "\"sub2\":{\"int_field1\":7,\"u_last\":\"x\"},"
        "\"u_tail\":0"
        "}";
    const std::string clean=
        "{"
        "\"sub1\":{\"str_field1\":\"inner\",\"int_field1\":5},"
        "\"sub2\":{\"int_field1\":7}"
        "}";

    ju2::type fromDirty;
    BOOST_REQUIRE(fromDirty.loadFromJSON(dirty));
    ju2::type fromClean;
    BOOST_REQUIRE(fromClean.loadFromJSON(clean));
    BOOST_CHECK_EQUAL(fromDirty.toString(PrettyFormat,MaxDecimalPlaces),fromClean.toString(PrettyFormat,MaxDecimalPlaces));

    // and the declared values really are there, not only equal to each other
    BOOST_REQUIRE(fromDirty.field(ju2::sub1).isSet());
    BOOST_CHECK_EQUAL(std::string{fromDirty.field(ju2::sub1).value().fieldValue(ju1::str_field1)},"inner");
    BOOST_CHECK_EQUAL(fromDirty.field(ju2::sub1).value().fieldValue(ju1::int_field1),5);
    BOOST_REQUIRE(fromDirty.field(ju2::sub2).isSet());
    BOOST_CHECK_EQUAL(fromDirty.field(ju2::sub2).value().fieldValue(ju1::int_field1),7);
    BOOST_CHECK(!fromDirty.field(ju2::sub2).value().field(ju1::str_field1).isSet());

    // a declared key inside a skipped value is the skipped value's, never the unit's: sub1 is the declared one, the "sub1"
    // inside u_obj must not replace or merge into it, and the "str_field1" there must not land in the top-level unit's subunits
    BOOST_CHECK_EQUAL(std::string{fromDirty.field(ju2::sub1).value().fieldValue(ju1::str_field1)},"inner");

    // the unknown key is the only one, and a unit of only unknown keys is an empty unit
    ju2::type onlyUnknown;
    BOOST_REQUIRE(onlyUnknown.loadFromJSON(std::string{"{\"zz\":1}"}));
    BOOST_CHECK(!onlyUnknown.field(ju2::sub1).isSet());
    BOOST_CHECK(!onlyUnknown.field(ju2::sub2).isSet());
    BOOST_REQUIRE(onlyUnknown.loadFromJSON(std::string{"{\"zz\":{\"a\":[]},\"yy\":[{}]}"}));
    BOOST_CHECK(!onlyUnknown.field(ju2::sub1).isSet());

    // the same unknown key twice, and a value that is the last thing before the end of the object
    BOOST_REQUIRE(onlyUnknown.loadFromJSON(std::string{"{\"zz\":1,\"zz\":[2],\"sub1\":{\"int_field1\":3},\"zz\":{\"x\":null}}"}));
    BOOST_REQUIRE(onlyUnknown.field(ju2::sub1).isSet());
    BOOST_CHECK_EQUAL(onlyUnknown.field(ju2::sub1).value().fieldValue(ju1::int_field1),3);
}

//! Unknown keys inside the elements of a repeated subunit.
BOOST_FIXTURE_TEST_CASE(TestUnknownKeysAreSkippedInRepeatedSubunits,::hatn::test::MultiThreadFixture)
{
    // every element carries the required field of all_types, otherwise the unit does not serialize to a string at all
    const std::string dirty=
        "{\"u_before\":[1,2],"
        "\"f0\":["
            "{\"type_int8\":5,\"u_in_elem\":{\"a\":[1,{\"b\":2}]},\"type_int16\":6,\"type_int8_required\":1},"
            "{\"u_scalar\":null,\"type_int8\":7,\"type_int8_required\":1},"
            "{\"u_only\":true,\"type_int8_required\":1}"
        "],"
        "\"u_after\":\"x\"}";
    const std::string clean=
        "{\"f0\":[{\"type_int8\":5,\"type_int16\":6,\"type_int8_required\":1},"
        "{\"type_int8\":7,\"type_int8_required\":1},{\"type_int8_required\":1}]}";

    wire_unit_repeated::type fromDirty;
    BOOST_REQUIRE(fromDirty.loadFromJSON(dirty));
    wire_unit_repeated::type fromClean;
    BOOST_REQUIRE(fromClean.loadFromJSON(clean));

    const auto& fields=wire_unit_repeated::traits::fields;
    BOOST_CHECK_EQUAL(fromDirty.field(fields.f0).count(),3u);
    BOOST_CHECK_EQUAL(fromClean.field(fields.f0).count(),3u);
    BOOST_CHECK(!fromClean.toString(PrettyFormat,MaxDecimalPlaces).empty());
    BOOST_CHECK_EQUAL(fromDirty.toString(PrettyFormat,MaxDecimalPlaces),fromClean.toString(PrettyFormat,MaxDecimalPlaces));
}

//! Skipping unknown keys must not hide real errors: a declared key of the wrong type, malformed JSON inside or after a
//! skipped value, and a document that is not an object are still refused.
BOOST_FIXTURE_TEST_CASE(TestSkippingUnknownKeysKeepsTheErrors,::hatn::test::MultiThreadFixture)
{
    auto fails=[](const std::string& json, const char* what, const char* inMessage=nullptr)
    {
        hatn::Error ec;
        ju2::type obj;
        BOOST_CHECK_MESSAGE(!obj.loadFromJSON(json,ec),std::string{"loaded: "}+what);
        if (inMessage!=nullptr)
        {
            BOOST_CHECK_MESSAGE(ec.message().find(inMessage)!=std::string::npos,
                                std::string{what}+": message is \""+ec.message()+"\"");
        }
    };

    // a declared key of the wrong type is an error next to unknown keys, and names the key
    fails("{\"zz\":1,\"sub1\":{\"str_field1\":100}}","wrong type of a declared key after an unknown key","str_field1");
    fails("{\"sub1\":{\"zz\":[1],\"int_field1\":\"text\"}}","wrong type of a declared key after an unknown key in a subunit","int_field1");

    // malformed JSON inside a skipped value, or an unterminated one
    fails("{\"zz\":{\"a\":[1,2}}","containers closed in the wrong order inside a skipped value");
    fails("{\"zz\":[1,2","unterminated array skipped");
    fails("{\"zz\":{\"a\":1}","unterminated outer object after a skipped object");
    fails("{\"zz\":{\"a\":}}","a missing value inside a skipped object");
    fails("{\"zz\":,\"sub1\":{}}","a missing value of an unknown key");
    fails("{\"zz\" 1}","a missing colon after an unknown key");

    // the document itself must be an object
    fails("5","a scalar document");
    fails("[1,2]","an array document");
    fails("\"text\"","a string document");
}

//! @todo Test Object ID, DateTime, Date, Time, DateRange

BOOST_AUTO_TEST_SUITE_END()
