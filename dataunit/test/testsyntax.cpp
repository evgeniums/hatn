#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <iostream>

#include <hatn/common/thread.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/makeshared.h>

#include <hatn/common/logger.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/elapsedtimer.h>
#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/memorypool/newdeletepool.h>

#include <hatn/dataunit/detail/fieldserialization.ipp>
#include <hatn/dataunit/visitors.h>

#include <hatn/test/multithreadfixture.h>

#define HDU_DATAUNIT_EXPORT
#include "testunitdeclarations.h"

//#define HATN_TEST_LOG_CONSOLE

static void setLogHandler()
{
    auto logCount=std::make_shared<int>(0);
    auto handler=[logCount](const ::hatn::common::FmtAllocatedBufferChar &s)
    {
        #ifdef HATN_TEST_LOG_CONSOLE
            std::cout<<::hatn::common::lib::toStringView(s)<<std::endl;
        #endif

        auto str=::hatn::common::fmtBufToString(s);
        str=str.substr(14,str.length());

        if (*logCount==0)
        {
            BOOST_CHECK_EQUAL(str,"t[main] WARNING:dataunit:serialize :: Failed to serialize DataUnit message all_types: required field type_int8_required is not set");
        }
        else
        {
            BOOST_CHECK_EQUAL(str,"t[main] DEBUG:1:dataunit:parse :: Failed to parse DataUnit message all_types: required field type_int8_required is not set");
        }
        ++(*logCount);
    };

    ::hatn::common::Logger::setOutputHandler(handler);
    ::hatn::common::Logger::setFatalLogHandler(handler);
    ::hatn::common::Logger::setDefaultVerbosity(::hatn::common::LoggerVerbosity::DEBUG);
    ::hatn::common::Logger::setDefaultDebugLevel(1);
}

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
        if (!::hatn::common::Logger::isRunning())
        {
            ::hatn::common::Logger::setFatalTracing(false);
            ::hatn::common::Logger::start(false);
        }
    }

    ~Env()
    {
        ::hatn::common::Logger::stop();
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};

BOOST_AUTO_TEST_SUITE(TestSyntax)

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

template <typename t_int8,typename traits,typename t> void typeCheckIsSet(const t& allTypes)
{
    const auto& fields=traits::fields;

    // check count
    BOOST_CHECK_EQUAL(allTypes.fieldCount(),15);
    // check field position
    auto int8Pos=allTypes.template fieldPos<t_int8>();
    BOOST_CHECK_EQUAL(int8Pos,1);

    const auto& f1_=allTypes.field(fields.type_bool);
    BOOST_CHECK(f1_.get());
    BOOST_CHECK(f1_.isSet());

    int8_t val_int8=-10;
    const auto& f2=allTypes.template field<t_int8>();
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
    const auto& f3=allTypes.field(fields.type_int16);
    BOOST_CHECK_EQUAL(static_cast<int>(f3.get()),static_cast<int>(val_int16));
    BOOST_CHECK(f3.isSet());
    const auto& f3_=allTypes.field(fields.type_int16);
    BOOST_CHECK(f3_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f3_.get()),static_cast<int>(val_int16));

    int32_t val_int32=0x1F810110;
    const auto& f4=allTypes.field(fields.type_int32);
    BOOST_CHECK_EQUAL(static_cast<int>(f4.get()),static_cast<int>(val_int32));
    BOOST_CHECK(f4.isSet());
    const auto& f4_=allTypes.field(fields.type_int32);
    BOOST_CHECK(f4_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f4_.get()),static_cast<int>(val_int32));

    uint8_t val_uint8=10;
    const auto& f5=allTypes.field(fields.type_uint8);
    BOOST_CHECK_EQUAL(static_cast<int>(f5.get()),static_cast<int>(val_uint8));
    BOOST_CHECK(f5.isSet());
    const auto& f5_=allTypes.field(fields.type_uint8);
    BOOST_CHECK(f5_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f5_.get()),static_cast<int>(val_uint8));

    uint16_t val_uint16=0xF810;
    const auto& f6=allTypes.field(fields.type_uint16);
    BOOST_CHECK_EQUAL(static_cast<int>(f6.get()),static_cast<int>(val_uint16));
    BOOST_CHECK(f6.isSet());
    const auto& f6_=allTypes.field(fields.type_uint16);
    BOOST_CHECK(f6_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f6_.get()),static_cast<int>(val_uint16));

    uint32_t val_uint32=0x1F810110;
    const auto& f7=allTypes.field(fields.type_uint32);
    BOOST_CHECK_EQUAL(static_cast<int>(f7.get()),static_cast<int>(val_uint32));
    BOOST_CHECK(f7.isSet());
    const auto& f7_=allTypes.field(fields.type_uint32);
    BOOST_CHECK(f7_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f7_.get()),static_cast<int>(val_uint32));

    float val_float=253245.7686f;
    const auto& f8=allTypes.field(fields.type_float);
    BOOST_CHECK_CLOSE(f8.get(),val_float,0.0001);
    BOOST_CHECK(f8.isSet());
    const auto& f8_=allTypes.field(fields.type_float);
    BOOST_CHECK_CLOSE(f8_.get(),val_float,0.0001);
    BOOST_CHECK(f8_.isSet());

    float val_double=253245.7686f;
    const auto& f9=allTypes.field(fields.type_double);
    BOOST_CHECK_CLOSE(f9.get(),val_double,0.0001);
    BOOST_CHECK(f9.isSet());
    const auto& f9_=allTypes.field(fields.type_double);
    BOOST_CHECK_CLOSE(f9_.get(),val_double,0.0001);
    BOOST_CHECK(f9_.isSet());
HATN_DATAUNIT_NAMESPACE_END

BOOST_AUTO_TEST_CASE(TestNames)
{
    using traits=names_and_descr::traits;
    const auto& fields=traits::fields;

    BOOST_CHECK_EQUAL(std::string(fields.optional_field.name()),std::string("optional_field"));
    BOOST_CHECK_EQUAL(std::string(fields.optional_field.description()),std::string(""));
    BOOST_CHECK_EQUAL(std::string(fields.optional_field_descr.name()),std::string("optional_field_descr"));
    BOOST_CHECK_EQUAL(std::string(fields.optional_field_descr.description()),std::string("Optional field with description"));

    BOOST_CHECK_EQUAL(std::string(fields.required_field.name()),std::string("required_field"));
    BOOST_CHECK_EQUAL(std::string(fields.required_field.description()),std::string(""));
    BOOST_CHECK_EQUAL(std::string(fields.required_field_descr.name()),std::string("required_field_descr"));
    BOOST_CHECK_EQUAL(std::string(fields.required_field_descr.description()),std::string("Required field with description"));

    BOOST_CHECK_EQUAL(std::string(fields.default_field.name()),std::string("default_field"));
    BOOST_CHECK_EQUAL(std::string(fields.default_field.description()),std::string(""));
    BOOST_CHECK_EQUAL(std::string(fields.default_field_descr.name()),std::string("default_field_descr"));
    BOOST_CHECK_EQUAL(std::string(fields.default_field_descr.description()),std::string("Default field with description"));

    BOOST_CHECK_EQUAL(std::string(fields.repeated_field.name()),std::string("repeated_field"));
    BOOST_CHECK_EQUAL(std::string(fields.repeated_field.description()),std::string(""));
    BOOST_CHECK_EQUAL(std::string(fields.repeated_field_descr.name()),std::string("repeated_field_descr"));
    BOOST_CHECK_EQUAL(std::string(fields.repeated_field_descr.description()),std::string("Repeated field with description"));

    using traitsSubunit=subunit_names_and_descr::traits;
    const auto& fieldsSubunit=traitsSubunit::fields;

    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.dataunit_field.name()),std::string("dataunit_field"));
    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.dataunit_field.description()),std::string(""));
    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.dataunit_field_descr.name()),std::string("dataunit_field_descr"));
    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.dataunit_field_descr.description()),std::string("Dataunit field with description"));

    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.external_field.name()),std::string("external_field"));
    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.external_field.description()),std::string(""));
    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.external_field_descr.name()),std::string("external_field_descr"));
    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.external_field_descr.description()),std::string("External field with description"));

    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.embedded_field.name()),std::string("embedded_field"));
    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.embedded_field.description()),std::string(""));
    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.embedded_field_descr.name()),std::string("embedded_field_descr"));
    BOOST_CHECK_EQUAL(std::string(fieldsSubunit.embedded_field_descr.description()),std::string("Embedded field with description"));
}

BOOST_FIXTURE_TEST_CASE(TestBasic,Env)
{
    using traits=all_types::traits;
    using type=traits::type;
    const auto& fields=traits::fields;

    type allTypes;

    auto& aaa=allTypes.field<1>();
    std::ignore=aaa;

    BOOST_TEST_CONTEXT("Normal construct")
    {
        typeChecks<decltype(fields.type_int8),traits>(allTypes);
    }

    BOOST_TEST_CONTEXT("Copy construct")
    {
        auto allTypesCopy=allTypes;
        typeCheckIsSet<decltype(fields.type_int8),traits>(allTypesCopy);
    }
    BOOST_TEST_CONTEXT("Copy operator")
    {
        type allTypesCopyOp;
        allTypesCopyOp=allTypes;
        typeCheckIsSet<decltype(fields.type_int8),traits>(allTypesCopyOp);
    }

    using extTraits=all_types::ext1::traits;
    const auto extFields=extTraits::fields;

    extTraits::type extTypes;
    auto& f1=extTypes.field(extFields.type_bool);
    BOOST_CHECK(!f1.isSet());
    BOOST_CHECK(!f1.get());
    f1.set(true);
    BOOST_CHECK(f1.isSet());
    BOOST_CHECK(f1.get());
    auto& f4=extTypes.field(fields.type_bool);
    BOOST_CHECK(f4.isSet());
    BOOST_CHECK(f4.get());
    auto& f2=extTypes.field(extFields.type_bool1);
    BOOST_CHECK(!f2.isSet());
    BOOST_CHECK(!f2.get());
    f2.set(true);
    BOOST_CHECK(f2.isSet());
    BOOST_CHECK(f2.get());

    auto& fExt0Field=extTypes.field(extFields.ext0_int32);
    std::ignore=fExt0Field;

    empty_unit::traits::type empt;
    std::ignore=empt;

    using extTraits2=empty_unit::ext2::traits;
    const auto& extFields2=extTraits2::fields;

    extTraits2::type ext2;
    auto& f3=ext2.field(extFields2.type_int8);
    f3.set(100);
    BOOST_CHECK_EQUAL(f3.get(),100);
}

BOOST_FIXTURE_TEST_CASE(TestManaged,::hatn::test::MultiThreadFixture)
{
    using traits=all_types::traits;
    using managed=traits::managed;
    const auto& fields=traits::fields;

    auto allTypes=::hatn::dataunit::AllocatorFactory::getDefault()->createObject<managed>();
    BOOST_CHECK(!allTypes.isNull());

    typeChecks<decltype(fields.type_int8),traits>(*allTypes);

    allTypes.reset();    
}

BOOST_FIXTURE_TEST_CASE(TestManagedRuntime,::hatn::test::MultiThreadFixture)
{
    using traits=all_types::traits;
    using managed=traits::managed;
    const auto& fields=traits::fields;

    auto allTypesP=::hatn::dataunit::AllocatorFactory::getDefault()->createObject<managed>();
    BOOST_CHECK(!allTypesP.isNull());
    auto& allTypes=*allTypesP;

    typeChecks<decltype(fields.type_int8),traits>(allTypes);

    allTypesP.reset();
}

BOOST_FIXTURE_TEST_CASE(TestSerialize,Env)
{
    setLogHandler();

    hatn::dataunit::WireDataSingle wired;

    using traits=all_types::traits;

    traits::type unit1;
    BOOST_CHECK(unit1.serialize(wired)==-1);

    wired.setCurrentOffset(0);

    traits::type unit2;
    BOOST_CHECK(!unit2.parse(wired));
}

BOOST_AUTO_TEST_CASE(TestDefaultFields)
{
    default_fields::type unit;
    const auto& fields=default_fields::traits::fields;

    // check count
    BOOST_CHECK_EQUAL(unit.fieldCount(),7);

    auto& f1=unit.field(fields.type_bool);
    BOOST_CHECK(f1.get());
    BOOST_CHECK(!f1.isSet());

    auto& f2=unit.field(fields.type_int32);
    BOOST_CHECK_EQUAL(f2.get(),1000);
    f2.set(5000);
    BOOST_CHECK_EQUAL(f2.get(),5000);
    f2.clear();
    BOOST_CHECK_EQUAL(f2.get(),1000);

    auto& f3=unit.field(fields.type_enum);
    BOOST_CHECK(f3.get()==default_fields::MyEnum::Two);
    f3.set(default_fields::MyEnum::One);
    BOOST_CHECK(f3.get()==default_fields::MyEnum::One);
}

namespace {
template <typename C, typename F, typename T> void wireSingleVar(const T& val)
{
    hatn::dataunit::WireDataSingle wired;
    BOOST_CHECK_EQUAL(wired.isSingleBuffer(),true);

    C unit1;
    unit1.template field<F>().set(val);
    BOOST_CHECK_EQUAL(unit1.template field<F>().get(),val);

    auto expectedSize=unit1.size();
    auto packedSize=unit1.serialize(wired);
    BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));
    BOOST_CHECK_EQUAL(packedSize,wired.mainContainer()->size());

    wired.setCurrentOffset(0);
    C unit2;
    unit2.parse(wired);
    BOOST_CHECK_EQUAL(unit2.template field<F>().get(),val);
HATN_DATAUNIT_NAMESPACE_END

BOOST_FIXTURE_TEST_CASE(TestSerializeCheckSingleNumbers,::hatn::test::MultiThreadFixture)
{
    wireSingleVar<wire_bool::type,decltype(wire_bool::f0),bool>(true);
    wireSingleVar<wire_bool::type,decltype(wire_bool::f0),bool>(false);

    wireSingleVar<wire_uint8::type,decltype(wire_uint8::f0),uint8_t>(5);
    wireSingleVar<wire_uint8::type,decltype(wire_uint8::f0),uint8_t>(200);
    wireSingleVar<wire_uint8::type,decltype(wire_uint8::f0),uint8_t>(0);

    wireSingleVar<wire_uint16::type,decltype(wire_uint16::f0),uint16_t>(5);
    wireSingleVar<wire_uint16::type,decltype(wire_uint16::f0),uint16_t>(0x1020);
    wireSingleVar<wire_uint16::type,decltype(wire_uint16::f0),uint16_t>(0xf123);
    wireSingleVar<wire_uint16::type,decltype(wire_uint16::f0),uint16_t>(0);

    wireSingleVar<wire_uint32::type,decltype(wire_uint32::f0),uint32_t>(-106);
    wireSingleVar<wire_uint32::type,decltype(wire_uint32::f0),uint32_t>(0x11223344);
    wireSingleVar<wire_uint32::type,decltype(wire_uint32::f0),uint32_t>(0xC8223344);
    wireSingleVar<wire_uint32::type,decltype(wire_uint32::f0),uint32_t>(0);

    wireSingleVar<wire_uint64::type,decltype(wire_uint64::f0),uint64_t>(-106);
    wireSingleVar<wire_uint64::type,decltype(wire_uint64::f0),uint64_t>(0x1122334455667788);
    wireSingleVar<wire_uint64::type,decltype(wire_uint64::f0),uint64_t>(0xC822334455667788);
    wireSingleVar<wire_uint64::type,decltype(wire_uint64::f0),uint64_t>(0);

    wireSingleVar<wire_int8::type,decltype(wire_int8::f0),int8_t>(-106);
    wireSingleVar<wire_int8::type,decltype(wire_int8::f0),int8_t>(0);
    wireSingleVar<wire_int8::type,decltype(wire_int8::f0),int8_t>(-100);

    wireSingleVar<wire_int16::type,decltype(wire_int16::f0),int16_t>(-106);
    wireSingleVar<wire_int16::type,decltype(wire_int16::f0),int16_t>(0);
    wireSingleVar<wire_int16::type,decltype(wire_int16::f0),int16_t>(-30000);

    wireSingleVar<wire_int32::type,decltype(wire_int32::f0),int32_t>(-106);
    wireSingleVar<wire_int32::type,decltype(wire_int32::f0),int32_t>(0);
    wireSingleVar<wire_int32::type,decltype(wire_int32::f0),int32_t>(-1000);

    wireSingleVar<wire_int64::type,decltype(wire_int64::f0),int64_t>(-106);
    wireSingleVar<wire_int64::type,decltype(wire_int64::f0),int64_t>(0x1122334455667788);
    wireSingleVar<wire_int64::type,decltype(wire_int64::f0),int64_t>(0xC822334455667788);
    wireSingleVar<wire_int64::type,decltype(wire_int64::f0),int64_t>(-1);
    wireSingleVar<wire_int64::type,decltype(wire_int64::f0),int64_t>(0);

    wireSingleVar<wire_fixed_uint32::type,decltype(wire_fixed_uint32::f0),uint32_t>(-106);
    wireSingleVar<wire_fixed_uint32::type,decltype(wire_fixed_uint32::f0),uint32_t>(0x11223344);
    wireSingleVar<wire_fixed_uint32::type,decltype(wire_fixed_uint32::f0),uint32_t>(0xC8223344);
    wireSingleVar<wire_fixed_uint32::type,decltype(wire_fixed_uint32::f0),uint32_t>(0);

    wireSingleVar<wire_fixed_uint64::type,decltype(wire_fixed_uint64::f0),uint64_t>(-106);
    wireSingleVar<wire_fixed_uint64::type,decltype(wire_fixed_uint64::f0),uint64_t>(0x1122334455667788);
    wireSingleVar<wire_fixed_uint64::type,decltype(wire_fixed_uint64::f0),uint64_t>(0xC822334455667788);
    wireSingleVar<wire_fixed_uint64::type,decltype(wire_fixed_uint64::f0),uint64_t>(0);

    wireSingleVar<wire_fixed_int32::type,decltype(wire_fixed_int32::f0),int32_t>(-106);
    wireSingleVar<wire_fixed_int32::type,decltype(wire_fixed_int32::f0),int32_t>(0);
    wireSingleVar<wire_fixed_int32::type,decltype(wire_fixed_int32::f0),int32_t>(-1000);

    wireSingleVar<wire_fixed_int64::type,decltype(wire_fixed_int64::f0),int64_t>(-106);
    wireSingleVar<wire_fixed_int64::type,decltype(wire_fixed_int64::f0),int64_t>(0x1122334455667788);
    wireSingleVar<wire_fixed_int64::type,decltype(wire_fixed_int64::f0),int64_t>(0xC822334455667788);
    wireSingleVar<wire_fixed_int64::type,decltype(wire_fixed_int64::f0),int64_t>(-1);
    wireSingleVar<wire_fixed_int64::type,decltype(wire_fixed_int64::f0),int64_t>(0);

    wireSingleVar<wire_float::type,decltype(wire_float::f0),float>(1123.45678f);
    wireSingleVar<wire_double::type,decltype(wire_double::f0),double>(1123.45678);
}

namespace {

using MemoryResource=::hatn::common::memorypool::NewDeletePoolResource;
using MemoryPool=::hatn::common::memorypool::NewDeletePool;

template <typename WiredT>
void checkByteArray(bool inlineBuffers=false)
{
    auto resource=std::make_unique<MemoryResource>();
    ::hatn::common::pmr::polymorphic_allocator<::hatn::common::ByteArrayManaged> byteArrayAllocator(resource.get());

    for (auto j=0;j<2;j++)
    {
        bool shared=j==1;

        WiredT wired;
        bool isSingle=!std::is_same<WiredT,hatn::dataunit::WireDataChained>::value;
        BOOST_CHECK_EQUAL(wired.isSingleBuffer(),isSingle);

        wire_bytes::type unit1;
        auto& field1=unit1.field(wire_bytes::f0);
        if (shared)
        {
            field1.setSharedByteArray(::hatn::common::allocateShared<::hatn::common::ByteArrayManaged>(byteArrayAllocator));
        }
        ::hatn::common::ByteArray* arr1=field1.buf();
        arr1->resize(0x10000);
        for (size_t i=0;i<arr1->size();i++)
        {
            auto d=i+i*i;
            (*arr1)[i]=static_cast<char>(d);
        }

        auto expectedSize=unit1.size();
        auto packedSize=unit1.serialize(wired);
        BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));
        BOOST_CHECK(static_cast<int>(packedSize)>static_cast<int>(arr1->size()));
        BOOST_REQUIRE(wired.mainContainer()!=nullptr);

        wired.setCurrentOffset(0);

        wire_bytes::type unit2;
        const auto& field2=unit2.field(wire_bytes::f0);
        auto& field2_1=unit2.field(wire_bytes::f0);
        if (shared)
        {
            field2_1.set(::hatn::common::allocateShared<::hatn::common::ByteArrayManaged>(byteArrayAllocator));
        }
        if (inlineBuffers)
        {
            wired.setUseInlineBuffers(inlineBuffers);
            BOOST_CHECK_EQUAL(inlineBuffers,wired.isUseInlineBuffers());
        }
        BOOST_CHECK_EQUAL(inlineBuffers,wired.isUseInlineBuffers());
        bool parsed=unit2.parse(wired);
        BOOST_CHECK(parsed);

        const ::hatn::common::ByteArray* arr2=field2.buf();
        BOOST_CHECK_EQUAL(arr1->size(),arr2->size());
        BOOST_CHECK_EQUAL(arr2->isRawBuffer(),inlineBuffers);
        bool passed=true;
        for (size_t i=0;i<arr2->size();i++)
        {
            auto d=i+i*i;
            passed=static_cast<uint8_t>((*arr2)[i])==static_cast<uint8_t>(d);
            if (!passed)
            {
                break;
            }
        }
        BOOST_CHECK(passed);
    HATN_DATAUNIT_NAMESPACE_END
}

BOOST_FIXTURE_TEST_CASE(TestSerializeCheckByteArray,::hatn::test::MultiThreadFixture)
{
    BOOST_TEST_CONTEXT("WireDataSingle,false"){checkByteArray<hatn::dataunit::WireDataSingle>();}
    BOOST_TEST_CONTEXT("WireDataSingleShared,false"){checkByteArray<hatn::dataunit::WireDataSingleShared>();}
    BOOST_TEST_CONTEXT("WireDataChained,false"){checkByteArray<hatn::dataunit::WireDataChained>();}
    BOOST_TEST_CONTEXT("WireDataSingle,true"){checkByteArray<hatn::dataunit::WireDataSingle>(true);}
    BOOST_TEST_CONTEXT("WireDataSingleShared,true"){checkByteArray<hatn::dataunit::WireDataSingleShared>(true);}
    // inline buffers for chained wire are useless because data will be internally copied for parsing
    //checkByteArray<hatn::dataunit::WireDataChained>(true);
}

BOOST_FIXTURE_TEST_CASE(TestSerializeCheckRepeatedUint32,::hatn::test::MultiThreadFixture)
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

    hatn::dataunit::WireDataSingle wired1;
    auto expectedSize=unit1.size();
    auto packedSize=unit1.serialize(wired1);
    BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));
    BOOST_REQUIRE(wired1.mainContainer()!=nullptr);

    wire_uint32_repeated::type unit2;
    unit2.parse(wired1);
    const auto& field1_1=unit2.field(wire_uint32_repeated::f0);
    BOOST_CHECK(field1_1.isSet());
    BOOST_CHECK_EQUAL(field1_1.count(),4);
    BOOST_CHECK_EQUAL(field1.value(0),field1_1.value(0));
    BOOST_CHECK_EQUAL(field1.value(1),field1_1.value(1));
    BOOST_CHECK_EQUAL(field1.value(2),field1_1.value(2));
    BOOST_CHECK_EQUAL(field1.value(3),field1_1.value(3));

    field1.clear();
    BOOST_CHECK_EQUAL(field1.count(),0);
    BOOST_CHECK(!field1.isSet());
}

namespace {
template <typename traits, typename WiredT>
void serializeCheckRepeatedDouble()
{
    const auto& fields=traits::fields;

    typename traits::type unit1;
    auto& field1=unit1.field(fields.f0);
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

    WiredT wired1;
    auto expectedSize=unit1.size();
    auto packedSize=unit1.serialize(wired1);
    BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));
    BOOST_REQUIRE(wired1.mainContainer()!=nullptr);

    typename traits::type  unit2;
    unit2.parse(wired1);
    const auto& field1_1=unit2.field(fields.f0);
    BOOST_REQUIRE(field1_1.isSet());
    BOOST_REQUIRE_EQUAL(field1_1.count(),5);
    BOOST_CHECK_EQUAL(field1.value(0),field1_1.value(0));
    BOOST_CHECK_EQUAL(field1.value(1),field1_1.value(1));
    BOOST_CHECK_EQUAL(field1.value(2),field1_1.value(2));
    BOOST_CHECK_EQUAL(field1.value(3),field1_1.value(3));
    BOOST_CHECK_EQUAL(field1.value(4),field1_1.value(4));

    field1.clear();
    BOOST_CHECK_EQUAL(field1.count(),0);
    BOOST_CHECK(!field1.isSet());
}

template <typename WiredT>
void checkRepeatedDouble()
{
    serializeCheckRepeatedDouble<wire_double_repeated::traits,WiredT>();
    serializeCheckRepeatedDouble<wire_double_repeated_proto_packed::traits,WiredT>();
    serializeCheckRepeatedDouble<wire_double_repeated_proto::traits,WiredT>();
}

}

BOOST_FIXTURE_TEST_CASE(SerializeCheckRepeatedDouble,::hatn::test::MultiThreadFixture)
{
    checkRepeatedDouble<hatn::dataunit::WireDataSingle>();
    checkRepeatedDouble<hatn::dataunit::WireDataSingleShared>();
    checkRepeatedDouble<hatn::dataunit::WireDataChained>();
}

namespace {

template <typename traits, typename WiredT>
void serializeCheckRepeatedBytes(bool shared=false, bool inlineBuffers=false)
{
    const auto& fields=traits::fields;

    std::ignore=shared;
    auto resource=std::make_unique<MemoryResource>();
    ::hatn::common::pmr::polymorphic_allocator<::hatn::common::ByteArrayManaged> byteArrayAllocator(resource.get());
    {
        typename traits::type unit1;
        auto& field1=unit1.field(fields.f0);
        field1.resize(3);
        BOOST_REQUIRE(field1.isSet());
        BOOST_REQUIRE_EQUAL(field1.count(),3);

        auto& arr1=field1.value(0);
        arr1.setSharedByteArray(::hatn::common::allocateShared<::hatn::common::ByteArrayManaged>(byteArrayAllocator));

        ::hatn::common::ByteArray* arr1_1=arr1.buf();
        arr1_1->resize(0x10000);
        for (size_t i=0;i<arr1_1->size();i++)
        {
            auto d=i+i*i;
            (*arr1_1)[i]=static_cast<char>(d);
        }

        ::hatn::common::ByteArray* arr1_2=field1.value(1).buf();
        arr1_2->resize(0x1000);
        for (size_t i=0;i<arr1_2->size();i++)
        {
            auto d=i+i*i+1234;
            (*arr1_2)[i]=static_cast<char>(d);
        }

        ::hatn::common::ByteArray* arr1_3=field1.value(2).buf();
        arr1_3->resize(0x2000);
        for (size_t i=0;i<arr1_3->size();i++)
        {
            auto d=i+i*i+5678;
            (*arr1_3)[i]=static_cast<char>(d);
        }

        WiredT wired;
        auto expectedSize=unit1.size();
        auto packedSize=unit1.serialize(wired);
        BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));
        BOOST_REQUIRE(wired.mainContainer()!=nullptr);

        typename traits::type unit2;

        unit2.setParseToSharedArrays(shared);
        if (inlineBuffers)
        {
            wired.setUseInlineBuffers(inlineBuffers);
            BOOST_CHECK_EQUAL(inlineBuffers,wired.isUseInlineBuffers());
        }
        BOOST_CHECK_EQUAL(inlineBuffers,wired.isUseInlineBuffers());
        bool parsed=unit2.parse(wired);
        BOOST_CHECK(parsed);

        const auto& field2=unit2.field(fields.f0);

        BOOST_REQUIRE(field2.isSet());
        BOOST_REQUIRE_EQUAL(field2.count(),3);

        const ::hatn::common::ByteArray* arr2_1=field2.value(0).buf();
        BOOST_CHECK_EQUAL(arr1_1->size(),arr2_1->size());
        BOOST_CHECK_EQUAL(arr2_1->isRawBuffer(),inlineBuffers);
        bool passed=true;
        for (size_t i=0;i<arr2_1->size();i++)
        {
            auto d=i+i*i;
            passed=static_cast<uint8_t>((*arr2_1)[i])==static_cast<uint8_t>(d);
            if (!passed)
            {
                break;
            }
        }
        BOOST_CHECK(passed);

        const ::hatn::common::ByteArray* arr2_2=field2.value(1).buf();
        BOOST_CHECK_EQUAL(arr1_2->size(),arr2_2->size());
        BOOST_CHECK_EQUAL(arr2_2->isRawBuffer(),inlineBuffers);
        passed=true;
        for (size_t i=0;i<arr2_2->size();i++)
        {
            auto d=i+i*i+1234;
            passed=static_cast<uint8_t>((*arr2_2)[i])==static_cast<uint8_t>(d);
            if (!passed)
            {
                break;
            }
        }
        BOOST_CHECK(passed);

        const ::hatn::common::ByteArray* arr2_3=field2.value(2).buf();
        BOOST_CHECK_EQUAL(arr1_3->size(),arr2_3->size());
        BOOST_CHECK_EQUAL(arr2_3->isRawBuffer(),inlineBuffers);
        passed=true;
        for (size_t i=0;i<arr2_3->size();i++)
        {
            auto d=i+i*i+5678;
            passed=static_cast<uint8_t>((*arr2_3)[i])==static_cast<uint8_t>(d);
            if (!passed)
            {
                break;
            }
        }
        BOOST_CHECK(passed);

    }    
}

template <typename traits, typename WiredT>
void serializeCheckRepeatedFixedString(bool shared=false, bool inlineBuffers=false)
{
    const auto& fields=traits::fields;

    {
        typename traits::type unit1;
        auto& field1=unit1.field(fields.f0);
        field1.resize(3);
        BOOST_REQUIRE(field1.isSet());
        BOOST_REQUIRE_EQUAL(field1.count(),3);

        auto* arr1_1=field1.value(0).buf();
        arr1_1->resize(128);
        for (size_t i=0;i<arr1_1->size();i++)
        {
            auto d=i+i*i;
            (*arr1_1)[i]=static_cast<char>(d);
        }

        auto* arr1_2=field1.value(1).buf();
        arr1_2->resize(100);
        for (size_t i=0;i<arr1_2->size();i++)
        {
            auto d=i+i*i+1234;
            (*arr1_2)[i]= static_cast<char>(d);
        }

        auto* arr1_3=field1.value(2).buf(); // zero size

        WiredT wired;
        auto expectedSize=unit1.size();
        auto packedSize=unit1.serialize(wired);
        BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));
        BOOST_REQUIRE(wired.mainContainer()!=nullptr);

        typename traits::type unit2;
        auto& field2_=unit2.field(fields.f0);
        field2_.setParseToSharedArrays(shared);

        if (inlineBuffers)
        {
            wired.setUseInlineBuffers(inlineBuffers);
            BOOST_CHECK_EQUAL(inlineBuffers,wired.isUseInlineBuffers());
        }
        BOOST_CHECK_EQUAL(inlineBuffers,wired.isUseInlineBuffers());
        bool parsed=unit2.parse(wired);
        BOOST_CHECK(parsed);

        const auto& field2=unit2.field(fields.f0);

        BOOST_REQUIRE(field2.isSet());
        BOOST_REQUIRE_EQUAL(field2.count(),3);

        auto* arr2_1=field2.value(0).buf();
        BOOST_CHECK_EQUAL(arr1_1->size(),arr2_1->size());
        BOOST_CHECK_EQUAL(arr2_1->isRawBuffer(),inlineBuffers);
        bool passed=true;
        for (size_t i=0;i<arr2_1->size();i++)
        {
            auto d=i+i*i;
            passed=static_cast<uint8_t>((*arr2_1)[i])==static_cast<uint8_t>(d);
            if (!passed)
            {
                break;
            }
        }
        BOOST_CHECK(passed);

        auto* arr2_2=field2.value(1).buf();
        BOOST_CHECK_EQUAL(arr1_2->size(),arr2_2->size());
        BOOST_CHECK_EQUAL(arr2_2->isRawBuffer(),inlineBuffers);
        passed=true;
        for (size_t i=0;i<arr2_2->size();i++)
        {
            auto d=i+i*i+1234;
            passed=static_cast<uint8_t>((*arr2_2)[i])==static_cast<uint8_t>(d);
            if (!passed)
            {
                break;
            }
        }
        BOOST_CHECK(passed);

        auto* arr2_3=field2.value(2).buf();
        BOOST_CHECK_EQUAL(arr1_3->size(),arr2_3->size());
        passed=true;
        for (size_t i=0;i<arr2_3->size();i++)
        {
            auto d=i+i*i+5678;
            passed=static_cast<uint8_t>((*arr2_3)[i])==static_cast<uint8_t>(d);
            if (!passed)
            {
                break;
            }
        }
        BOOST_CHECK(passed);
    HATN_DATAUNIT_NAMESPACE_END

template <typename WiredT>
void checkRepeatedBytes(bool inlineBuffers=false)
{
    BOOST_TEST_CONTEXT("wire_bytes_repeated::traits,false"){serializeCheckRepeatedBytes<wire_bytes_repeated::traits,WiredT>(false,inlineBuffers);}
    BOOST_TEST_CONTEXT("wire_bytes_repeated_proto::traits,false"){serializeCheckRepeatedBytes<wire_bytes_repeated_proto::traits,WiredT>(false,inlineBuffers);}
    BOOST_TEST_CONTEXT("wire_string_repeated::traits,false"){serializeCheckRepeatedBytes<wire_string_repeated::traits,WiredT>(false,inlineBuffers);}
    BOOST_TEST_CONTEXT("wire_string_repeated_proto::traits,false"){serializeCheckRepeatedBytes<wire_string_repeated_proto::traits,WiredT>(false,inlineBuffers);}

    BOOST_TEST_CONTEXT("wire_bytes_repeated::traits,true"){serializeCheckRepeatedBytes<wire_bytes_repeated::traits,WiredT>(true,inlineBuffers);}
    BOOST_TEST_CONTEXT("wire_bytes_repeated_proto::traits,true"){serializeCheckRepeatedBytes<wire_bytes_repeated_proto::traits,WiredT>(true,inlineBuffers);}
    BOOST_TEST_CONTEXT("wire_string_repeated::traits,true"){serializeCheckRepeatedBytes<wire_string_repeated::traits,WiredT>(true,inlineBuffers);}
    BOOST_TEST_CONTEXT("wire_string_repeated_proto::traits,true"){serializeCheckRepeatedBytes<wire_string_repeated_proto::traits,WiredT>(true,inlineBuffers);}

    BOOST_TEST_CONTEXT("wire_fixed_string_repeated::traits,false"){serializeCheckRepeatedFixedString<wire_fixed_string_repeated::traits,WiredT>(false,inlineBuffers);}
    BOOST_TEST_CONTEXT("wire_fixed_string_repeated_proto::traits,false"){serializeCheckRepeatedFixedString<wire_fixed_string_repeated_proto::traits,WiredT>(false,inlineBuffers);}

    BOOST_TEST_CONTEXT("wire_fixed_string_repeated::traits,true"){serializeCheckRepeatedFixedString<wire_fixed_string_repeated::traits,WiredT>(true,inlineBuffers);}
    BOOST_TEST_CONTEXT("wire_fixed_string_repeated_proto::traits,true"){serializeCheckRepeatedFixedString<wire_fixed_string_repeated_proto::traits,WiredT>(true,inlineBuffers);}
}

BOOST_FIXTURE_TEST_CASE(TestSerializeCheckRepeatedBytes,Env)
{
    BOOST_TEST_CONTEXT("WireDataSingle,false"){checkRepeatedBytes<hatn::dataunit::WireDataSingle>();}
    BOOST_TEST_CONTEXT("WireDataSingleShared,false"){checkRepeatedBytes<hatn::dataunit::WireDataSingleShared>();}
    BOOST_TEST_CONTEXT("WireDataChained,false"){checkRepeatedBytes<hatn::dataunit::WireDataChained>();}

    BOOST_TEST_CONTEXT("WireDataSingle,true"){checkRepeatedBytes<hatn::dataunit::WireDataSingle>(true);}
    BOOST_TEST_CONTEXT("WireDataSingleShared,true"){checkRepeatedBytes<hatn::dataunit::WireDataSingleShared>(true);}

    // inline buffers for chained wire are useless because data will be internally copied for parsing
    //checkRepeatedBytes<hatn::dataunit::WireDataChained>(true);
}

template <typename traits> void checkRepeatedUnitFields()
{
    const auto& fields=traits::fields;

    typename traits::type unit1;
    auto& field1=unit1.field(fields.f0);

    using allTypesTraits=all_types::traits;
    const auto& fields1=allTypesTraits::fields;

    auto& obj1=field1.createAndAddValue();
    typeChecks<decltype(fields1.type_int8),allTypesTraits>(*obj1.mutableValue());
    field1.clear();
}

}

BOOST_FIXTURE_TEST_CASE(TestRepeatedUnitFields,Env)
{
    checkRepeatedUnitFields<wire_unit_repeated::traits>();
    checkRepeatedUnitFields<wire_embedded_all_repeated::traits>();
    checkRepeatedUnitFields<wire_embedded_all_repeated_protobuf::traits>();
    checkRepeatedUnitFields<wire_external_all_repeated_protobuf::traits>();
}

namespace {
template <typename traits, typename WiredT> void checkSerializeRepeatedSimpleUnit()
{
    const auto& fields=traits::fields;

    typename traits::type unit1;
    auto& field1=unit1.field(fields.f0);
    BOOST_CHECK(!field1.isSet());
    auto& field1_1=field1.createAndAddValue();
    auto* obj1=field1_1.mutableValue();
    auto& obj1_field1=obj1->field(simple_int8::type_int8);
    obj1_field1.set(100);

    BOOST_CHECK(field1.isSet());
    BOOST_CHECK_EQUAL(field1.count(),1);

    WiredT wired;
    auto expectedSize=unit1.size();
    auto packedSize=unit1.serialize(wired);
    BOOST_REQUIRE(packedSize>0);
    BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));
    BOOST_REQUIRE(wired.mainContainer()!=nullptr);

    typename traits::type unit2;
    bool parsed=unit2.parse(wired);
    BOOST_REQUIRE(parsed);

    const auto& field2=unit2.field(fields.f0);
    BOOST_CHECK(field2.isSet());
    BOOST_CHECK_EQUAL(field2.count(),1);
HATN_DATAUNIT_NAMESPACE_END

BOOST_FIXTURE_TEST_CASE(TestSerializeRepeatedSimpleUnit,Env)
{
    checkSerializeRepeatedSimpleUnit<wire_simple_unit_repeated::traits,hatn::dataunit::WireDataSingle>();
    checkSerializeRepeatedSimpleUnit<wire_simple_unit_repeated::traits,hatn::dataunit::WireDataSingleShared>();
    checkSerializeRepeatedSimpleUnit<wire_simple_unit_repeated::traits,hatn::dataunit::WireDataChained>();
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

template <typename T> void checkUnitFields(const T& allTypes, int n)
{
    // check field position
    const auto& int8Pos=allTypes.fieldPos(all_types::type_int8);
    BOOST_CHECK_EQUAL(int8Pos,1);

    const auto& f1_=allTypes.field(all_types::type_bool);
    BOOST_CHECK(f1_.get());
    BOOST_CHECK(f1_.isSet());

    int8_t val_int8=-10+n;
    const auto& f2_=allTypes.field(all_types::type_int8);
    BOOST_CHECK(f2_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f2_.get()),static_cast<int>(val_int8));
    // check field by position
    const auto& f2__=allTypes.template field<1>();
    BOOST_CHECK(f2__.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f2__.get()),static_cast<int>(val_int8));

    int8_t val_int8_required=123+n;
    const auto& f2_2=allTypes.field(all_types::type_int8_required);
    BOOST_CHECK(f2_2.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f2_2.get()),static_cast<int>(val_int8_required));

    int16_t val_int16=0xF810+n;
    const auto& f3_=allTypes.field(all_types::type_int16);
    BOOST_CHECK(f3_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f3_.get()),static_cast<int>(val_int16));

    int32_t val_int32=0x1F810110+n;
    const auto& f4_=allTypes.field(all_types::type_int32);
    BOOST_CHECK(f4_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f4_.get()),static_cast<int>(val_int32));

    uint8_t val_uint8=10+n;
    const auto& f5_=allTypes.field(all_types::type_uint8);
    BOOST_CHECK(f5_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f5_.get()),static_cast<int>(val_uint8));

    uint16_t val_uint16=0xF810+n;
    const auto& f6_=allTypes.field(all_types::type_uint16);
    BOOST_CHECK(f6_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f6_.get()),static_cast<int>(val_uint16));

    uint32_t val_uint32=0x1F810110+n;
    const auto& f7_=allTypes.field(all_types::type_uint32);
    BOOST_CHECK(f7_.isSet());
    BOOST_CHECK_EQUAL(static_cast<int>(f7_.get()),static_cast<int>(val_uint32));

    float val_float=253245.7686f+static_cast<float>(n);
    const auto& f8_=allTypes.field(all_types::type_float);
    BOOST_CHECK_CLOSE(f8_.get(),val_float,0.0001);
    BOOST_CHECK(f8_.isSet());

    float val_double=253245.7686f+static_cast<float>(n);
    const auto& f9_=allTypes.field(all_types::type_double);
    BOOST_CHECK_CLOSE(f9_.get(),val_double,0.0001);
    BOOST_CHECK(f9_.isSet());
}

template <typename traits>
void checkSerializeSubUnit()
{
    const auto& fields=traits::fields;

    typename traits::type unit1;

    auto& field1=unit1.field(fields.f0);
    BOOST_CHECK(!field1.isSet());

    auto* obj1=field1.mutableValue();
    BOOST_REQUIRE(field1.isSet());
    BOOST_REQUIRE(obj1!=nullptr);
    fillUnitFields(*obj1,0);
    hatn::dataunit::WireDataSingle wired;
    auto expectedSize=unit1.size();
    auto packedSize=unit1.serialize(wired);
    BOOST_REQUIRE(packedSize>0);
    BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));
    BOOST_REQUIRE(wired.mainContainer()!=nullptr);

    typename traits::type unit2;
    bool parsed=unit2.parse(wired);
    BOOST_REQUIRE(parsed);

    const auto& field2=unit2.field(fields.f0);
    BOOST_REQUIRE(field2.isSet());

    const auto& obj2=field2.value();
    checkUnitFields(obj2,0);

    typename traits::type unit3;
    auto& field3=unit3.field(fields.f0);
    BOOST_CHECK(!field3.isSet());
    auto* obj3=field3.mutableValue();
    BOOST_REQUIRE(field3.isSet());
    BOOST_REQUIRE(obj3!=nullptr);
    fillUnitFields(*obj3,0);
    auto subWired3_1=hatn::common::makeShared<hatn::dataunit::WireDataSingle>();
    auto expectedSizeSub=obj3->size();
    auto packedSizeSub=obj3->serialize(*subWired3_1);
    BOOST_REQUIRE(packedSizeSub>0);
    BOOST_CHECK(static_cast<int>(expectedSizeSub)>=static_cast<int>(packedSizeSub));
    obj3->keepWireData(subWired3_1);
    hatn::dataunit::WireDataSingle wired3;
    auto packedSize3=unit3.serialize(wired3);
    BOOST_REQUIRE(packedSize3>0);
    BOOST_REQUIRE(wired3.mainContainer()!=nullptr);
    BOOST_CHECK_EQUAL(packedSize,packedSize3);

    typename traits::type unit3_2;
    parsed=unit3_2.parse(wired3);
    BOOST_REQUIRE(parsed);

    const auto& field3_2=unit3_2.field(fields.f0);
    BOOST_REQUIRE(field3_2.isSet());

    const auto& obj3_2=field3_2.value();
    checkUnitFields(obj3_2,0);
HATN_DATAUNIT_NAMESPACE_END

BOOST_FIXTURE_TEST_CASE(TestSerializeSubunit,Env)
{
    with_unit3::traits::type unit1;
    with_unit3::shared_traits::type unit2;

    with_unit4::traits::type unit3;
    with_unit4::shared_traits::type unit4;

    std::ignore=unit1;
    std::ignore=unit2;
    std::ignore=unit3;
    std::ignore=unit4;

    checkSerializeSubUnit<embedded_unit::traits>();
    checkSerializeSubUnit<shared_unit::traits>();

    checkSerializeSubUnit<with_unit::traits>();
    checkSerializeSubUnit<with_unit::shared_traits>();
}

namespace {

template <typename traits, typename WiredT, typename subWiredT>
void checkSerializePreparedSubUnit()
{
    const auto& fields=traits::fields;
    typename traits::type unit1;

    auto& field1=unit1.field(fields.f0);
    BOOST_CHECK(!field1.isSet());

    auto* obj1=field1.mutableValue();
    BOOST_REQUIRE(field1.isSet());
    BOOST_REQUIRE(obj1!=nullptr);
    fillUnitFields(*obj1,0);
    WiredT wired;
    auto expectedSize=unit1.size();
    auto packedSize=unit1.serialize(wired);
    BOOST_REQUIRE(packedSize>0);
    BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));

    hatn::dataunit::WireDataSingle wiredSingle1;
    auto packedSizeSingle=unit1.serialize(wiredSingle1);
    BOOST_CHECK_EQUAL(packedSize,packedSizeSingle);
    BOOST_REQUIRE(wiredSingle1.mainContainer());
    hatn::dataunit::WireDataSingle wiredSingle2=wired.toSingleWireData();
    BOOST_REQUIRE(wiredSingle2.mainContainer());
    BOOST_CHECK(*wiredSingle1.mainContainer()==*wiredSingle2.mainContainer());

    typename traits::type unit2;
    bool parsed2=unit2.parse(wiredSingle2);
    BOOST_REQUIRE(parsed2);
    const auto& field2_2=unit2.field(fields.f0);
    BOOST_REQUIRE(field2_2.isSet());
    const auto& obj2_2=field2_2.value();
    checkUnitFields(obj2_2,0);

    typename traits::type unit3;
    auto& field3=unit3.field(fields.f0);
    BOOST_CHECK(!field3.isSet());
    auto* obj3=field3.mutableValue();
    BOOST_REQUIRE(field3.isSet());
    BOOST_REQUIRE(obj3!=nullptr);
    fillUnitFields(*obj3,0);
    auto subWired3_1=hatn::common::makeShared<subWiredT>();
    auto expectedSizeSub=obj3->size();
    auto packedSizeSub=obj3->serialize(*subWired3_1);
    BOOST_REQUIRE(packedSizeSub>0);
    BOOST_CHECK(static_cast<int>(expectedSizeSub)>=static_cast<int>(packedSizeSub));
    obj3->keepWireData(subWired3_1);
    WiredT wired3;
    auto packedSize3=unit3.serialize(wired3);
    BOOST_REQUIRE(packedSize3>0);
    BOOST_CHECK_EQUAL(packedSize,packedSize3);
    hatn::common::ByteArray barr3;
    copyToContainer(wired3,&barr3);
    BOOST_CHECK(*wiredSingle1.mainContainer()==barr3);
    if (*wiredSingle1.mainContainer()!=barr3)
    {
        BOOST_CHECK_EQUAL(wiredSingle1.mainContainer()->size(),barr3.size());
        for (size_t i=0;i<barr3.size();i++)
        {
            if (wiredSingle1.mainContainer()->at(i)!=barr3.at(i))
            {
                BOOST_ERROR(fmt::format("Mismatch at {}",i));
                break;
            }
        }
    }

    wired3.resetState();
    typename traits::type unit3_2;
    auto parsed=unit3_2.parse(wired3);
    BOOST_REQUIRE(parsed);

    const auto& field3_2=unit3_2.field(fields.f0);
    BOOST_REQUIRE(field3_2.isSet());

    const auto& obj3_2=field3_2.value();
    checkUnitFields(obj3_2,0);
}

template <typename WiredT>
void checkSerializePrepared()
{
    BOOST_TEST_CONTEXT("embedded_unit::traits,WiredT,hatn::dataunit::WireDataSingle"){checkSerializePreparedSubUnit<embedded_unit::traits,WiredT,hatn::dataunit::WireDataSingle>();}
    BOOST_TEST_CONTEXT("shared_unit::traits,WiredT,hatn::dataunit::WireDataSingle"){checkSerializePreparedSubUnit<shared_unit::traits,WiredT,hatn::dataunit::WireDataSingle>();}
    BOOST_TEST_CONTEXT("with_unit::traits,WiredT,hatn::dataunit::WireDataSingle"){checkSerializePreparedSubUnit<with_unit::traits,WiredT,hatn::dataunit::WireDataSingle>();}
    BOOST_TEST_CONTEXT("with_unit::shared_traits,WiredT,hatn::dataunit::WireDataSingle"){checkSerializePreparedSubUnit<with_unit::shared_traits,WiredT,hatn::dataunit::WireDataSingle>();}

    BOOST_TEST_CONTEXT("embedded_unit::traits,WiredT,hatn::dataunit::WireDataSingleShared"){checkSerializePreparedSubUnit<embedded_unit::traits,WiredT,hatn::dataunit::WireDataSingleShared>();}
    BOOST_TEST_CONTEXT("shared_unit::traits,WiredT,hatn::dataunit::WireDataSingleShared"){checkSerializePreparedSubUnit<shared_unit::traits,WiredT,hatn::dataunit::WireDataSingleShared>();}
    BOOST_TEST_CONTEXT("with_unit::traits,WiredT,hatn::dataunit::WireDataSingleShared"){checkSerializePreparedSubUnit<with_unit::traits,WiredT,hatn::dataunit::WireDataSingleShared>();}
    BOOST_TEST_CONTEXT("with_unit::shared_traits,WiredT,hatn::dataunit::WireDataSingleShared"){checkSerializePreparedSubUnit<with_unit::shared_traits,WiredT,hatn::dataunit::WireDataSingleShared>();}

    BOOST_TEST_CONTEXT("embedded_unit::traits,WiredT,hatn::dataunit::WireDataChained"){checkSerializePreparedSubUnit<embedded_unit::traits,WiredT,hatn::dataunit::WireDataChained>();}
    BOOST_TEST_CONTEXT("shared_unit::traits,WiredT,hatn::dataunit::WireDataChained"){checkSerializePreparedSubUnit<shared_unit::traits,WiredT,hatn::dataunit::WireDataChained>();}
    BOOST_TEST_CONTEXT("with_unit::traits,WiredT,hatn::dataunit::WireDataChained"){checkSerializePreparedSubUnit<with_unit::traits,WiredT,hatn::dataunit::WireDataChained>();}
    BOOST_TEST_CONTEXT("with_unit::shared_traits,WiredT,hatn::dataunit::WireDataChained"){checkSerializePreparedSubUnit<with_unit::shared_traits,WiredT,hatn::dataunit::WireDataChained>();}
}
}

BOOST_FIXTURE_TEST_CASE(TestSerializePreparedSubunit,Env)
{
    BOOST_TEST_CONTEXT("hatn::dataunit::WireDataSingle"){checkSerializePrepared<hatn::dataunit::WireDataSingle>();}
    BOOST_TEST_CONTEXT("hatn::dataunit::WireDataSingleShared"){checkSerializePrepared<hatn::dataunit::WireDataSingleShared>();}
    BOOST_TEST_CONTEXT("hatn::dataunit::WireDataChained"){checkSerializePrepared<hatn::dataunit::WireDataChained>();}
}

namespace {
template <typename traits> void checkSerializeRepeatedUnit()
{
    const auto& fields=traits::fields;
    typename traits::type unit1;
    auto& field1=unit1.field(fields.f0);

    int n=1;

    for (int i=0;i<n;i++)
    {
        auto& obj=field1.createAndAddValue();
        fillUnitFields(*obj.mutableValue(),i);
    }

    BOOST_REQUIRE(field1.isSet());
    BOOST_REQUIRE_EQUAL(field1.count(),n);

    hatn::dataunit::WireDataSingle wired;
    auto expectedSize=unit1.size();
    auto packedSize=unit1.serialize(wired);
    BOOST_REQUIRE(packedSize>0);
    BOOST_CHECK(static_cast<int>(expectedSize)>=static_cast<int>(packedSize));
    BOOST_REQUIRE(wired.mainContainer()!=nullptr);

    typename traits::type unit2;
    bool parsed=unit2.parse(wired);
    BOOST_REQUIRE(parsed);

    const auto& field2=unit2.field(fields.f0);
    BOOST_REQUIRE(field2.isSet());
    BOOST_REQUIRE_EQUAL(field2.count(),n);

    for (int i=0;i<n;i++)
    {
        const auto& obj=field2.value(i);
        checkUnitFields(obj.value(),i);
    HATN_DATAUNIT_NAMESPACE_END
}

BOOST_FIXTURE_TEST_CASE(TestSerializeRepeatedUnit,Env)
{
    checkSerializeRepeatedUnit<wire_unit_repeated::traits>();
    checkSerializeRepeatedUnit<wire_embedded_all_repeated::traits>();
    checkSerializeRepeatedUnit<wire_embedded_all_repeated_protobuf::traits>();
    checkSerializeRepeatedUnit<wire_external_all_repeated_protobuf::traits>();

    checkSerializeRepeatedUnit<wire_unit_repeated1::traits>();
    checkSerializeRepeatedUnit<wire_unit_repeated1::shared_traits>();
//    checkSerializeRepeatedUnit<wire_unit_repeated_protobuf1::traits>();
    checkSerializeRepeatedUnit<wire_unit_repeated_protobuf1::shared_traits>();
}

BOOST_FIXTURE_TEST_CASE(TestUnitCasting,::hatn::test::MultiThreadFixture)
{
    auto factory=hatn::dataunit::AllocatorFactory::getDefault();
    auto unit1=factory->createObject<all_types::managed>(factory);
    auto unit2=unit1.staticCast<hatn::dataunit::Unit>();

    auto ptr=unit1->castToUnit(unit2.get());
    BOOST_CHECK_EQUAL(ptr,unit1.get());
}

BOOST_FIXTURE_TEST_CASE(TestUnitTree,::hatn::test::MultiThreadFixture)
{
    auto factory=hatn::dataunit::AllocatorFactory::getDefault();
    auto unit=factory->createObject<tree::managed>(factory);

    auto unit1=factory->createObject<tree::managed>(factory);
    auto unit2=factory->createObject<tree::managed>(factory);
    auto unit3=factory->createObject<tree::managed>(factory);
    auto unit4=factory->createObject<tree::managed>(factory);

    unit2->field(tree::children).addValues(2);
    unit3->field(tree::children).addValues(3);
    unit4->field(tree::children).addValues(4);

    unit2->field(tree::f0).set(15);
    unit3->field(tree::f0).set(25);
    unit4->field(tree::f0).set(35);

    unit->field(tree::children).addValue(unit1);
    unit->field(tree::children).addValue(unit2);
    unit->field(tree::children).addValue(unit3);
    unit->field(tree::children).addValue(unit4);

    size_t count=0;
    size_t levelCount=0;
    size_t contentCount=0;

    auto handler=[&count,&levelCount,&contentCount](const tree::type* obj, const tree::type* parent, size_t level)
    {
        std::ignore=obj;
        std::ignore=parent;
        std::ignore=level;

        contentCount+=obj->field(tree::f0).get();
        levelCount+=level;
        ++count;

        return true;
    };

    auto res=unit->iterateUnitTree(handler,unit.get(),tree::children);
    BOOST_CHECK(res);

    BOOST_CHECK_EQUAL(count,14);
    BOOST_CHECK_EQUAL(levelCount,22);
    BOOST_CHECK_EQUAL(contentCount,5*count+10+20+30);
}

BOOST_FIXTURE_TEST_CASE(TestFreeFactory,::hatn::test::MultiThreadFixture)
{
    ::hatn::dataunit::AllocatorFactory::resetDefault();
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
