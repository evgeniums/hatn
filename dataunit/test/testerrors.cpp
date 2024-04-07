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

    BOOST_CHECK_EQUAL(RawError::threadLocal().code,0);
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,-1);
    BOOST_CHECK(RawError::threadLocal().message.empty());

    auto task1=[]()
    {
        BOOST_CHECK_EQUAL(RawError::threadLocal().code,0);
        BOOST_CHECK_EQUAL(RawError::threadLocal().field,-1);
        BOOST_CHECK(RawError::threadLocal().message.empty());

        RawError::threadLocal().code=10;
        RawError::threadLocal().field=11;
        RawError::threadLocal().message="some message";

        BOOST_CHECK_EQUAL(RawError::threadLocal().code,10);
        BOOST_CHECK_EQUAL(RawError::threadLocal().field,11);
        BOOST_CHECK_EQUAL(RawError::threadLocal().message, "some message");

        return 0;
    };

    BOOST_TEST_CONTEXT("Before task2"){thread->execFuture<int>(task1);}

    BOOST_CHECK_EQUAL(RawError::threadLocal().code,0);
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,-1);
    BOOST_CHECK(RawError::threadLocal().message.empty());

    RawError::threadLocal().code=110;
    RawError::threadLocal().field=111;
    RawError::threadLocal().message="main thread message";

    BOOST_CHECK_EQUAL(RawError::threadLocal().code,110);
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,111);
    BOOST_CHECK_EQUAL(RawError::threadLocal().message, "main thread message");

    auto task2=[]()
    {
        BOOST_CHECK_EQUAL(RawError::threadLocal().code,10);
        BOOST_CHECK_EQUAL(RawError::threadLocal().field,11);
        BOOST_CHECK_EQUAL(RawError::threadLocal().message, "some message");

        RawError::threadLocal().reset();
        return 0;
    };

    thread->execFuture<int>(task2);

    BOOST_CHECK_EQUAL(RawError::threadLocal().code,110);
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,111);
    BOOST_CHECK_EQUAL(RawError::threadLocal().message, "main thread message");

    BOOST_TEST_CONTEXT("After task2"){thread->execFuture<int>(task1);}

    BOOST_CHECK_EQUAL(RawError::threadLocal().code,110);
    BOOST_CHECK_EQUAL(RawError::threadLocal().field,111);
    BOOST_CHECK_EQUAL(RawError::threadLocal().message, "main thread message");

    thread->stop();
}

BOOST_AUTO_TEST_SUITE_END()
