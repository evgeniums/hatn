#include <boost/test/unit_test.hpp>

#include <hatn/common/memorylockeddata.h>
#include <hatn/common/logger.h>

HATN_USING
HATN_COMMON_USING
using namespace std;

class SetupMemoryLockedData
{
    public:

        SetupMemoryLockedData()
        {
            Logger::setDefaultVerbosity(LoggerVerbosity::NONE);
        }
};

BOOST_AUTO_TEST_SUITE(TestMemoryLockedDataContainers)

BOOST_AUTO_TEST_CASE(StringCtr)
{
    string s1 = "hello world";
    MemoryLockedDataString sds1 = "hello world";
    MemoryLockedDataString sds1_ = s1.c_str();
    BOOST_CHECK(sds1_ == sds1);

    string s2 = sds1.c_str();
    BOOST_CHECK(s1 == s2);
}

BOOST_AUTO_TEST_CASE(StringConcat)
{
    MemoryLockedDataString sds1 = "hello world";
    bool thrown = false;
    try {
        MemoryLockedDataString sds2;
        for (int i=0; i<1000; i++)
            sds2 += sds1;
    }
    catch (const std::runtime_error& e)
    {
        BOOST_TEST_MESSAGE(e.what());
        thrown = true;
    }
    catch (...)
    {
        thrown = true;
    }
    BOOST_CHECK(!thrown);
}

// macos and linux can provide huge memory pages
#if !defined(__APPLE__) && !defined(__linux__)
BOOST_AUTO_TEST_CASE(StringNotEnoughMemory)
{    
    MemoryLockedDataString sds1 = "hello world";
    bool thrown = false;
    try {
        MemoryLockedDataString sds2;
        for (int i=0; i<10000000; i++)
            sds2 += sds1;
    } catch (...) {
        thrown = true;
    }
    BOOST_CHECK(thrown);
}

BOOST_AUTO_TEST_CASE(SDSStreamConcatBad)
{
    MemoryLockedDataString sds1 = "hello world";
    MemoryLockedDataStringStream ss;

    for (int i=0; i<10000000; i++)
        ss << sds1;

    BOOST_CHECK(!ss.good());
}
#endif

BOOST_AUTO_TEST_CASE(SDSStreamConcatOk)
{
    MemoryLockedDataString sds1 = "hello world";
    MemoryLockedDataStringStream ss;

    for (int i=0; i<1000; i++)
        ss << sds1;

    BOOST_CHECK(ss.good());
}

BOOST_AUTO_TEST_SUITE_END()
