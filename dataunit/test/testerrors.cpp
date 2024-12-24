#include <boost/test/unit_test.hpp>

#include <hatn/test/multithreadfixture.h>

bool TestRelaxMissingFieldSerializing=false;
#define HDU_TEST_RELAX_MISSING_FIELD_SERIALIZING && !TestRelaxMissingFieldSerializing

#include <hatn/common/utils.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/datauniterror.h>
#include <hatn/dataunit/unitmacros.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

using namespace HATN_TEST_NAMESPACE;
using namespace HATN_COMMON_NAMESPACE;
using namespace HATN_DATAUNIT_NAMESPACE;
using namespace HATN_DATAUNIT_NAMESPACE::types;
using namespace HATN_DATAUNIT_NAMESPACE::meta;

namespace {

HDU_V2_UNIT(u1,
    HDU_V2_FIELD(f1,TYPE_INT32,1)
    HDU_V2_REQUIRED_FIELD(f2,TYPE_INT32,2)
)

HDU_V2_UNIT(u2,
    HDU_V2_FIELD(f3,TYPE_INT32,3)
    HDU_V2_FIELD(f4,u1::TYPE,4,true)
)

HDU_V2_UNIT(u3,
    HDU_V2_FIELD(f1,TYPE_INT32,1)
    HDU_V2_FIELD(f2,TYPE_INT32,2)
    HDU_V2_FIELD(f3,TYPE_INT32,3)
    HDU_V2_FIELD(f4,TYPE_INT32,4)
    HDU_V2_REPEATED_FIELD(f5,TYPE_INT32,5,false,100)
)

HDU_V2_UNIT(u4,
    HDU_V2_FIELD(f10,TYPE_BOOL,10)
    HDU_V2_FIELD(f11,TYPE_INT8,11)
    HDU_V2_FIELD(f12,TYPE_INT16,12)
    HDU_V2_FIELD(f13,TYPE_INT32,13)
    HDU_V2_FIELD(f1,u3::TYPE,1)
    HDU_V2_FIELD(f14,TYPE_INT64,14)
    HDU_V2_FIELD(f15,TYPE_FIXED_INT32,15)
    HDU_V2_FIELD(f16,TYPE_FIXED_INT64,16)
    HDU_V2_FIELD(f26,HDU_V2_TYPE_FIXED_STRING(64),26)
    HDU_V2_FIELD(f17,TYPE_UINT8,17)
    HDU_V2_FIELD(f18,TYPE_UINT16,18)
    HDU_V2_FIELD(f19,TYPE_UINT32,19)
    HDU_V2_FIELD(f20,TYPE_UINT64,20)
    HDU_V2_FIELD(f21,TYPE_FIXED_UINT32,21)
    HDU_V2_FIELD(f22,TYPE_FIXED_UINT64,22)
    HDU_V2_FIELD(f27,TYPE_BYTES,27)
    HDU_V2_FIELD(f23,TYPE_FLOAT,23)
    HDU_V2_FIELD(f24,TYPE_DOUBLE,24)
    HDU_V2_FIELD(f25,TYPE_STRING,25)
)

uint32_t urand(uint32_t mn, uint32_t mx)
{
    return HATN_COMMON_NAMESPACE::Random::uniform(mn,mx);
}

}

BOOST_AUTO_TEST_SUITE(TestErrors)

BOOST_FIXTURE_TEST_CASE(ThreadLocalError,MultiThreadFixture)
{
    auto thread=std::make_shared<Thread>("test");
    thread->start();

    BOOST_CHECK_EQUAL(int(RawError::threadLocal().code),int(RawErrorCode::OK));
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,-1);
    BOOST_CHECK(RawError::threadLocal().message.empty());

    auto task1=[]()
    {
        BOOST_CHECK_EQUAL(int(RawError::threadLocal().code),int(RawErrorCode::OK));
        BOOST_CHECK_EQUAL(RawError::threadLocal().field,-1);
        BOOST_CHECK(RawError::threadLocal().message.empty());

        RawError::threadLocal().code=RawErrorCode::END_OF_STREAM;
        RawError::threadLocal().field=11;
        RawError::threadLocal().message="some message";

        BOOST_CHECK_EQUAL(int(RawError::threadLocal().code),int(RawErrorCode::END_OF_STREAM));
        BOOST_CHECK_EQUAL(RawError::threadLocal().field,11);
        BOOST_CHECK_EQUAL(RawError::threadLocal().message, "some message");

        return RawError::threadLocal().code;
    };

    auto c1=thread->execSync<RawErrorCode>(task1);
    BOOST_CHECK_EQUAL(int(c1),int(RawErrorCode::END_OF_STREAM));

    BOOST_CHECK_EQUAL(int(RawError::threadLocal().code),int(RawErrorCode::OK));
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,-1);
    BOOST_CHECK(RawError::threadLocal().message.empty());

    RawError::threadLocal().code=RawErrorCode::FIELD_TYPE_MISMATCH;
    RawError::threadLocal().field=111;
    RawError::threadLocal().message="main thread message";

    BOOST_CHECK_EQUAL(int(RawError::threadLocal().code),int(RawErrorCode::FIELD_TYPE_MISMATCH));
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,111);
    BOOST_CHECK_EQUAL(RawError::threadLocal().message, "main thread message");

    auto task2=[]()
    {
        BOOST_CHECK_EQUAL(int(RawError::threadLocal().code),int(RawErrorCode::END_OF_STREAM));
        BOOST_CHECK_EQUAL(RawError::threadLocal().field,11);
        BOOST_CHECK_EQUAL(RawError::threadLocal().message, "some message");

        RawError::threadLocal().reset();
        return RawError::threadLocal().code;;
    };

    auto c2=thread->execSync<RawErrorCode>(task2);
    BOOST_CHECK_EQUAL(int(c2),int(RawErrorCode::OK));

    BOOST_CHECK_EQUAL(int(RawError::threadLocal().code),int(RawErrorCode::FIELD_TYPE_MISMATCH));
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,111);
    BOOST_CHECK_EQUAL(RawError::threadLocal().message, "main thread message");

    auto c3=thread->execSync<RawErrorCode>(task1);
    BOOST_CHECK_EQUAL(int(c3),int(RawErrorCode::END_OF_STREAM));

    BOOST_CHECK_EQUAL(int(RawError::threadLocal().code),int(RawErrorCode::FIELD_TYPE_MISMATCH));
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,111);
    BOOST_CHECK_EQUAL(RawError::threadLocal().message, "main thread message");

    thread->stop();
}

BOOST_AUTO_TEST_CASE(RequiredField)
{
    int count=0;
    {
        HATN_SCOPE_GUARD([&count](){ BOOST_TEST_MESSAGE("Scope exit"); count++; })
    }
    BOOST_CHECK_EQUAL(count,1);

    u1::type v1;
    WireBufSolid buf1;
    Error ec;
    BOOST_CHECK_EQUAL(io::serialize(v1,buf1,ec),-1);
    BOOST_CHECK(ec);
    BOOST_TEST_MESSAGE(ec.message());
    BOOST_CHECK_EQUAL(ec.value(),int(UnitError::SERIALIZE_ERROR));
    BOOST_CHECK_EQUAL(ec.message(),"failed to serialize object: required field f2 is not set");
    auto nativeEc=ec.native();
    BOOST_REQUIRE(nativeEc!=nullptr);
    auto unitEc=dynamic_cast<const UnitNativeError*>(nativeEc);
    BOOST_REQUIRE(unitEc!=nullptr);
    BOOST_CHECK_EQUAL(unitEc->nativeCode(),int(RawErrorCode::REQUIRED_FIELD_MISSING));
    BOOST_CHECK_EQUAL(unitEc->fieldId(),2);
    BOOST_CHECK_EQUAL(unitEc->message(),"required field f2 is not set");

    u1::type v2;
    Error ec1;
    BOOST_CHECK(!io::deserialize(v2,buf1,ec1));
    BOOST_CHECK(ec1);
    BOOST_TEST_MESSAGE(ec1.message());
    BOOST_CHECK_EQUAL(ec1.value(),int(UnitError::PARSE_ERROR));
    BOOST_CHECK_EQUAL(ec1.message(),"failed to parse object: required field f2 is not set");
    auto nativeEc1=ec1.native();
    BOOST_REQUIRE(nativeEc1!=nullptr);
    auto unitEc1=dynamic_cast<const UnitNativeError*>(nativeEc1);
    BOOST_REQUIRE(unitEc1!=nullptr);
    BOOST_CHECK_EQUAL(unitEc1->nativeCode(),int(RawErrorCode::REQUIRED_FIELD_MISSING));
    BOOST_CHECK_EQUAL(unitEc1->fieldId(),2);
    BOOST_CHECK_EQUAL(unitEc1->message(),"required field f2 is not set");

    v1.field(u1::f2).set(100);
    Error ec2;
    BOOST_CHECK_GT(io::serialize(v1,buf1,ec2),0);
    BOOST_CHECK(!ec2);

    u1::type v3;
    Error ec3;
    BOOST_CHECK(io::deserialize(v3,buf1,ec3));
    BOOST_CHECK(!ec3);
    BOOST_CHECK_EQUAL(v3.field(u1::f2).value(),100);

    u2::type v4;
    Error ec4;
    BOOST_CHECK(v4.field(u2::f4).mutableValue()!=nullptr);
    BOOST_CHECK_EQUAL(io::serialize(v4,buf1,ec4),-1);
    BOOST_CHECK(ec4);
    BOOST_TEST_MESSAGE(ec4.message());
    BOOST_CHECK_EQUAL(ec4.value(),int(UnitError::SERIALIZE_ERROR));
    BOOST_CHECK_EQUAL(ec4.message(),"failed to serialize object: failed to serialize field f4: required field f2 is not set");
    auto nativeEc4=ec4.native();
    BOOST_REQUIRE(nativeEc4!=nullptr);
    auto unitEc4=dynamic_cast<const UnitNativeError*>(nativeEc4);
    BOOST_REQUIRE(unitEc4!=nullptr);
    BOOST_CHECK_EQUAL(unitEc4->nativeCode(),int(RawErrorCode::REQUIRED_FIELD_MISSING));
    BOOST_CHECK_EQUAL(unitEc4->fieldId(),4);
    BOOST_CHECK_EQUAL(unitEc4->message(),"failed to serialize field f4: required field f2 is not set");

    u2::type v5;
    Error ec5;
    BOOST_CHECK(!io::deserialize(v5,buf1,ec5));
    BOOST_CHECK(ec5);
    BOOST_TEST_MESSAGE(ec5.message());
    BOOST_CHECK_EQUAL(ec5.value(),int(UnitError::PARSE_ERROR));
    BOOST_CHECK_EQUAL(ec5.message(),"failed to parse object: failed to parse field f4 at offset 3: available data size is less than requested size");
    auto nativeEc5=ec5.native();
    BOOST_REQUIRE(nativeEc5!=nullptr);
    auto unitEc5=dynamic_cast<const UnitNativeError*>(nativeEc5);
    BOOST_REQUIRE(unitEc5!=nullptr);
    BOOST_CHECK_EQUAL(unitEc5->nativeCode(),int(RawErrorCode::END_OF_STREAM));
    BOOST_CHECK_EQUAL(unitEc5->fieldId(),4);
    BOOST_CHECK_EQUAL(unitEc5->message(),"failed to parse field f4 at offset 3: available data size is less than requested size");

    TestRelaxMissingFieldSerializing=true;
    u2::type v6;
    Error ec6;
    BOOST_CHECK(v6.field(u2::f4).mutableValue()!=nullptr);
    BOOST_CHECK_GT(io::serialize(v6,buf1,ec4),0);
    BOOST_CHECK(!ec6);

    u2::type v7;
    Error ec7;
    BOOST_CHECK(!io::deserialize(v7,buf1,ec7));
    BOOST_CHECK(ec7);
    BOOST_TEST_MESSAGE(ec7.message());
    BOOST_CHECK_EQUAL(ec7.value(),int(UnitError::PARSE_ERROR));
    BOOST_CHECK_EQUAL(ec7.message(),"failed to parse object: failed to parse field f4 at offset 6: required field f2 is not set");
    auto nativeEc7=ec7.native();
    BOOST_REQUIRE(nativeEc7!=nullptr);
    auto unitEc7=dynamic_cast<const UnitNativeError*>(nativeEc7);
    BOOST_REQUIRE(unitEc7!=nullptr);
    BOOST_CHECK_EQUAL(unitEc7->nativeCode(),int(RawErrorCode::REQUIRED_FIELD_MISSING));
    BOOST_CHECK_EQUAL(unitEc7->fieldId(),4);
    BOOST_CHECK_EQUAL(unitEc7->message(),"failed to parse field f4 at offset 6: required field f2 is not set");
}

BOOST_AUTO_TEST_CASE(UnitBufErrors)
{
    u3::type v1;
    WireBufSolid buf1;
    v1.field(u3::f1).set(101);
    v1.field(u3::f2).set(102);
    v1.field(u3::f3).set(103);
    v1.field(u3::f4).set(104);
    v1.field(u3::f5).appendValues(7);

    auto fillBuf=[&v1,&buf1]()
    {
        Error ec1;
        BOOST_CHECK_EQUAL(io::serialize(v1,buf1,ec1),28);
        BOOST_CHECK(!ec1);
    };

    Error ec;

    ec.reset();
    fillBuf();
    u3::type vOK;
    BOOST_CHECK(io::deserialize(vOK,buf1,ec));
    BOOST_CHECK(!ec);

    ec.reset();
    fillBuf();
    buf1.mainContainer()->resize(buf1.mainContainer()->size()-1);
    BOOST_CHECK_EQUAL(buf1.mainContainer()->size(),27);
    u3::type v2;
    BOOST_CHECK(!io::deserialize(v2,buf1,ec));
    BOOST_CHECK(ec);
    BOOST_TEST_MESSAGE(ec.message());
    BOOST_CHECK_EQUAL(ec.value(),int(UnitError::PARSE_ERROR));
    BOOST_CHECK_EQUAL(ec.message(),"failed to parse object: failed to parse field f5 at offset 26: unexpected end of buffer");
    auto nativeEc=ec.native();
    BOOST_REQUIRE(nativeEc!=nullptr);
    auto unitEc=dynamic_cast<const UnitNativeError*>(nativeEc);
    BOOST_REQUIRE(unitEc!=nullptr);
    BOOST_CHECK_EQUAL(unitEc->nativeCode(),int(RawErrorCode::END_OF_STREAM));
    BOOST_CHECK_EQUAL(unitEc->fieldId(),5);
    BOOST_CHECK_EQUAL(unitEc->message(),"failed to parse field f5 at offset 26: unexpected end of buffer");

    ec.reset();
    fillBuf();
    buf1.mainContainer()->resize(buf1.mainContainer()->size()-10);
    BOOST_CHECK_EQUAL(buf1.mainContainer()->size(),18);
    BOOST_CHECK(!io::deserialize(v2,buf1,ec));
    BOOST_CHECK(ec);
    BOOST_TEST_MESSAGE(ec.message());
    BOOST_CHECK_EQUAL(ec.value(),int(UnitError::PARSE_ERROR));
    BOOST_CHECK_EQUAL(ec.message(),"failed to parse object: failed to parse field f5 at offset 14: repeated count 7 exceeds available buffer size 4");
    nativeEc=ec.native();
    BOOST_REQUIRE(nativeEc!=nullptr);
    unitEc=dynamic_cast<const UnitNativeError*>(nativeEc);
    BOOST_REQUIRE(unitEc!=nullptr);
    BOOST_CHECK_EQUAL(unitEc->nativeCode(),int(RawErrorCode::END_OF_STREAM));
    BOOST_CHECK_EQUAL(unitEc->fieldId(),5);
    BOOST_CHECK_EQUAL(unitEc->message(),"failed to parse field f5 at offset 14: repeated count 7 exceeds available buffer size 4");

    ec.reset();
    fillBuf();
    buf1.mainContainer()->resize(buf1.mainContainer()->size()-21);
    buf1.incSize(-21);
    BOOST_CHECK_EQUAL(buf1.mainContainer()->size(),7);
    BOOST_CHECK(!io::deserialize(v2,buf1,ec));
    BOOST_CHECK(ec);
    BOOST_TEST_MESSAGE(ec.message());
    BOOST_CHECK_EQUAL(ec.value(),int(UnitError::PARSE_ERROR));
    BOOST_CHECK_EQUAL(ec.message(),"failed to parse object: failed to parse field f3 at offset 7: unexpected end of buffer");
    nativeEc=ec.native();
    BOOST_REQUIRE(nativeEc!=nullptr);
    unitEc=dynamic_cast<const UnitNativeError*>(nativeEc);
    BOOST_REQUIRE(unitEc!=nullptr);
    BOOST_CHECK_EQUAL(unitEc->nativeCode(),int(RawErrorCode::END_OF_STREAM));
    BOOST_CHECK_EQUAL(unitEc->fieldId(),3);
    BOOST_CHECK_EQUAL(unitEc->message(),"failed to parse field f3 at offset 7: unexpected end of buffer");

    ec.reset();
    fillBuf();
    auto i=buf1.mainContainer()->size();
    buf1.mainContainer()->resize(i+10);
    buf1.incSize(10);
    for (;i<buf1.mainContainer()->size();i++)
    {
        (*buf1.mainContainer())[i]=static_cast<char>(urand(1,255));
    }
    BOOST_CHECK_EQUAL(buf1.mainContainer()->size(),38);
    auto maybeOk=io::deserialize(v2,buf1,ec);
    if (!maybeOk)
    {
        BOOST_CHECK(!maybeOk);
        BOOST_CHECK(ec);
    }
    else
    {
        BOOST_TEST_MESSAGE("Parsed succesfully random buffer");
    }

    // fuzzy buffer content - mailform 25% of bytes
    BOOST_TEST_MESSAGE("Fuzzing buffer contents before parsing");
    int total=1500;
    int good=0;
    for (size_t i=0;i<total;i++)
    {
        ec.reset();
        fillBuf();
        for (size_t j=0;j<buf1.mainContainer()->size();j++)
        {
            if (urand(0,16)%4==0)
            {
                (*buf1.mainContainer())[j]=static_cast<char>(urand(0,0xFF));
            }
        }
        if (io::deserialize(v2,buf1,ec))
        {
            good++;
        }
        if (i%100==0)
        {
            BOOST_TEST_MESSAGE(fmt::format("processed {} of {}",i,total));
        }
    }
    BOOST_TEST_MESSAGE(fmt::format("fuzzing finished: parsed good {} of {}",good,total));
}

BOOST_AUTO_TEST_CASE(FuzzUnit)
{
    int runs=1500;

    u4::type obj;
    auto fillObj=[&obj]()
    {
        obj.field(u4::f1).mutableValue()->field(u3::f1).set(urand(0,0xFFFFFFFF));
        obj.field(u4::f1).mutableValue()->field(u3::f2).set(urand(0,0xFFFFFFFF));
        obj.field(u4::f1).mutableValue()->field(u3::f3).set(urand(0,0xFFFFFFFF));
        obj.field(u4::f1).mutableValue()->field(u3::f4).set(urand(0,0xFFFFFFFF));
        for (uint32_t i=0;i<urand(0,8);i++)
        {
            obj.field(u4::f1).mutableValue()->field(u3::f5).appendValue(urand(0,0xFFFFFFFF));
        }

        obj.field(u4::f10).set((urand(0,0xFFFFFFFF)&0x1) == 0x1);
        obj.field(u4::f11).set(int8_t(urand(0,0xFF)));
        obj.field(u4::f12).set(int16_t(urand(0,0xFFFF)));
        obj.field(u4::f13).set(urand(0,0xFFFFFFFF));
        auto f14=(int64_t(urand(0,0xFFFFFFFF))<<32) | urand(0,0xFFFFFFFF);
        obj.field(u4::f14).set(f14);
        obj.field(u4::f15).set(urand(0,0xFFFFFFFF));
        auto f16=(int64_t(urand(0,0xFFFFFFFF))<<32) | urand(0,0xFFFFFFFF);
        obj.field(u4::f16).set(f16);

        obj.field(u4::f17).set(uint8_t(urand(0,0xFF)));
        obj.field(u4::f18).set(uint16_t(urand(0,0xFFFF)));
        obj.field(u4::f19).set(urand(0,0xFFFFFFFF));
        auto f20=(uint64_t(urand(0,0xFFFFFFFF))<<32) | urand(0,0xFFFFFFFF);
        obj.field(u4::f20).set(f20);
        obj.field(u4::f21).set(urand(0,0xFFFFFFFF));
        auto f22=(uint64_t(urand(0,0xFFFFFFFF))<<32) | urand(0,0xFFFFFFFF);
        obj.field(u4::f22).set(f22);

        obj.field(u4::f23).set(float(urand(0,0xFFFFFFFF)));
        auto f24=(uint64_t(urand(0,0xFFFFFFFF))<<32) | urand(0,0xFFFFFFFF);
        obj.field(u4::f24).set(double(f24));

        obj.field(u4::f25).set(fmt::format("{:d}",f24));
        obj.field(u4::f26).set(fmt::format("{:x}",f24));

        obj.field(u4::f27).buf()->load(fmt::format("{:d}{:x}",f20,f24));
    };

    fillObj();
    BOOST_TEST_MESSAGE(obj.toString(true));

    u4::type tObj;

    WireBufSolid buf1;
    auto goodHandler=[&obj,&tObj,&fillObj,&buf1]
    {
        fillObj();

        BOOST_REQUIRE_GT(io::serialize(obj,buf1),0);
        BOOST_REQUIRE(io::deserialize(tObj,buf1));
        BOOST_REQUIRE_EQUAL(obj.toString(),tObj.toString());
    };

    BOOST_TEST_MESSAGE("Test normal operation");
    goodHandler();
    for (int i=0;i<runs;i++)
    {
        goodHandler();
        if (i%100==0)
        {
            BOOST_TEST_MESSAGE(fmt::format("processed {} of {}",i+1,runs));
        }
    }

    BOOST_TEST_MESSAGE("Test operation with mailformed buffer");
    int good=0;
    auto mailformedHandler=[&obj,&tObj,&fillObj,&buf1,&good]
    {
        fillObj();

        BOOST_REQUIRE_GT(io::serialize(obj,buf1),0);

        for (size_t i=0;i<urand(0,64);i++)
        {
            (*buf1.mainContainer())[static_cast<size_t>(urand(0,uint32_t(buf1.mainContainer()->size())-1))]=static_cast<char>(urand(0,0xFF));
        }

        if (io::deserialize(tObj,buf1))
        {
            good++;
        }
    };
    for (int i=0;i<runs;i++)
    {
        mailformedHandler();
        if (i%100==0)
        {
            BOOST_TEST_MESSAGE(fmt::format("processed {} of {}",i+1,runs));
        }
    }
    BOOST_TEST_MESSAGE(fmt::format("fuzzing finished: parsed good {} of {}",good,runs));
}

BOOST_AUTO_TEST_SUITE_END()
