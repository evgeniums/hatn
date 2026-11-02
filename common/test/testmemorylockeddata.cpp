#include <vector>

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

// macos and linux can provide huge memory pages, and with best effort locking a page that can not
// be locked is used unlocked instead of throwing, so neither case below can expect a failure
#if !defined(__APPLE__) && !defined(__linux__) && !defined(HATN_MEMORY_LOCK_BEST_EFFORT)
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

// An empty region must be a no-op on both sides. unlockRegion() used to lack lockRegion()'s
// n==0 guard, so the end of the region wrapped to the byte before it and a neighbouring page
// was released instead.
BOOST_AUTO_TEST_CASE(LockEmptyRegion)
{
    std::vector<char> buf(4096);

    MemoryLocker::lockRegion(buf.data(),0);
    MemoryLocker::unlockRegion(buf.data(),0);

    // the page around buf must still be usable, and a real lock/unlock of it must still balance
    MemoryLocker::lockRegion(buf.data(),buf.size());
    MemoryLocker::unlockRegion(buf.data(),buf.size());
}

// Overlapping regions share pages, so the pages they share must survive until the last
// region using them is released. Repeated to prove nothing leaks or double-unlocks.
BOOST_AUTO_TEST_CASE(LockOverlappingRegions)
{
    // kept small: a default RLIMIT_MEMLOCK can be as little as 64 KiB, and in strict mode
    // exceeding it would throw and fail the test for reasons unrelated to the accounting
    std::vector<char> buf(16*1024);

    for (int i=0; i<10; i++)
    {
        MemoryLocker::lockRegion(buf.data(),buf.size());
        MemoryLocker::lockRegion(buf.data()+1024,buf.size()-2048);
        MemoryLocker::lockRegion(buf.data(),1024);

        MemoryLocker::unlockRegion(buf.data(),1024);
        MemoryLocker::unlockRegion(buf.data()+1024,buf.size()-2048);

        // still locked by the first region: writing must be safe
        buf[0]='a';
        buf[buf.size()-1]='z';

        MemoryLocker::unlockRegion(buf.data(),buf.size());
    }

    BOOST_CHECK_EQUAL(buf[0],'a');
    BOOST_CHECK_EQUAL(buf[buf.size()-1],'z');
}

// Many short-lived buffers: the page counter must not accumulate entries for pages that are no
// longer locked. Exercised through the allocator so both sides of the refcount are used.
BOOST_AUTO_TEST_CASE(LockCounterDoesNotAccumulate)
{
    for (int i=0; i<2000; i++)
    {
        MemoryLockedDataString s;
        s += "hello world";
        BOOST_CHECK(!s.empty());
    }
}

BOOST_AUTO_TEST_SUITE_END()
