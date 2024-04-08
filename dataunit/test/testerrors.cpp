#include <boost/test/unit_test.hpp>

#include <hatn/test/multithreadfixture.h>

#define HDU_TEST_RELAX_MISSING_FEILD_SERIALIZING && !TestRelaxMissingFieldSerializing

#include <hatn/dataunit/datauniterror.h>
#include <hatn/dataunit/unitmacros.h>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

HATN_DATAUNIT_NAMESPACE_BEGIN
bool TestRelaxMissingFieldSerializing=false;
HATN_DATAUNIT_NAMESPACE_END

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

BOOST_AUTO_TEST_SUITE_END()
