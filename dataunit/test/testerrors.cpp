#include <boost/test/unit_test.hpp>

#include <hatn/test/multithreadfixture.h>

#include <hatn/dataunit/datauniterror.h>
#include <hatn/dataunit/unitmacros.h>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

using namespace HATN_TEST_NAMESPACE;
using namespace HATN_COMMON_NAMESPACE;
using namespace HATN_DATAUNIT_NAMESPACE;
using namespace HATN_DATAUNIT_NAMESPACE::types;
using namespace HATN_DATAUNIT_NAMESPACE::meta;

namespace {

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

BOOST_AUTO_TEST_SUITE_END()
