/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/test/testpcmring.cpp
  *
  *  Tests of PcmRing.
  *
  */

/****************************************************************************/

#include <atomic>
#include <chrono>
#include <thread>
#include <numeric>

#include <boost/test/unit_test.hpp>

#include <hatn/media/pcmring.h>

HATN_MEDIA_USING

BOOST_AUTO_TEST_SUITE(TestPcmRing)

BOOST_AUTO_TEST_CASE(CapacityIsPowerOfTwo)
{
    BOOST_CHECK_EQUAL(PcmRing{0}.capacity(),size_t{2});
    BOOST_CHECK_EQUAL(PcmRing{1}.capacity(),size_t{2});
    BOOST_CHECK_EQUAL(PcmRing{3}.capacity(),size_t{4});
    BOOST_CHECK_EQUAL(PcmRing{1000}.capacity(),size_t{1024});
    BOOST_CHECK_EQUAL(PcmRing{1024}.capacity(),size_t{1024});
}

BOOST_AUTO_TEST_CASE(WriteThenRead)
{
    PcmRing ring{16};
    std::vector<int16_t> in(10);
    std::iota(in.begin(),in.end(),int16_t{1});

    BOOST_CHECK_EQUAL(ring.write(in.data(),in.size()),size_t{10});
    BOOST_CHECK_EQUAL(ring.readable(),size_t{10});
    BOOST_CHECK_EQUAL(ring.writable(),size_t{6});

    std::vector<int16_t> out(10,0);
    BOOST_CHECK_EQUAL(ring.read(out.data(),out.size()),size_t{10});
    BOOST_CHECK(out==in);
    BOOST_CHECK_EQUAL(ring.readable(),size_t{0});
}

BOOST_AUTO_TEST_CASE(FullRingAcceptsOnlyWhatFits)
{
    PcmRing ring{8};
    std::vector<int16_t> in(20,7);

    BOOST_CHECK_EQUAL(ring.write(in.data(),in.size()),size_t{8});
    BOOST_CHECK_EQUAL(ring.write(in.data(),in.size()),size_t{0});
    BOOST_CHECK_EQUAL(ring.writable(),size_t{0});

    int16_t one=0;
    BOOST_CHECK_EQUAL(ring.read(&one,1),size_t{1});
    BOOST_CHECK_EQUAL(ring.writable(),size_t{1});
    BOOST_CHECK_EQUAL(ring.write(in.data(),in.size()),size_t{1});
}

BOOST_AUTO_TEST_CASE(WrapsAround)
{
    PcmRing ring{8};
    int16_t next=0;
    int16_t expected=0;

    // Many uneven writes and reads so the indices wrap the buffer several times.
    for (int round=0;round<200;round++)
    {
        std::vector<int16_t> in(5);
        for (auto& s : in)
        {
            s=next++;
        }
        BOOST_REQUIRE_EQUAL(ring.write(in.data(),in.size()),in.size());

        std::vector<int16_t> out(5);
        BOOST_REQUIRE_EQUAL(ring.read(out.data(),out.size()),out.size());
        for (auto s : out)
        {
            BOOST_REQUIRE_EQUAL(s,expected);
            expected++;
        }
    }
}

BOOST_AUTO_TEST_CASE(IndicesAreMonotonic)
{
    PcmRing ring{8};
    std::vector<int16_t> in(6,1);
    std::vector<int16_t> out(6);

    for (int i=0;i<10;i++)
    {
        ring.write(in.data(),in.size());
        ring.read(out.data(),out.size());
    }
    BOOST_CHECK_EQUAL(ring.writeIndex(),uint64_t{60});
    BOOST_CHECK_EQUAL(ring.readIndex(),uint64_t{60});
}

BOOST_AUTO_TEST_CASE(DiscardUpToIsClamped)
{
    PcmRing ring{16};
    std::vector<int16_t> in(10,3);
    ring.write(in.data(),in.size());

    // partway
    ring.discardUpTo(4);
    BOOST_CHECK_EQUAL(ring.readIndex(),uint64_t{4});
    BOOST_CHECK_EQUAL(ring.readable(),size_t{6});

    // never backwards
    ring.discardUpTo(1);
    BOOST_CHECK_EQUAL(ring.readIndex(),uint64_t{4});

    // never past what was written
    ring.discardUpTo(1000);
    BOOST_CHECK_EQUAL(ring.readIndex(),uint64_t{10});
    BOOST_CHECK_EQUAL(ring.readable(),size_t{0});
}

BOOST_AUTO_TEST_CASE(ProducerAndConsumerOnTwoThreads)
{
    // One million samples in strictly increasing order (mod 2^15) must come out in the same order
    // with nothing lost or duplicated, through a ring far smaller than the stream.
    constexpr size_t total=1000000;
    PcmRing ring{4096};
    std::atomic<bool> failed{false};

    std::thread producer([&ring]()
    {
        size_t sent=0;
        std::vector<int16_t> chunk(300);
        while (sent<total)
        {
            const auto n=std::min(chunk.size(),total-sent);
            for (size_t i=0;i<n;i++)
            {
                chunk[i]=static_cast<int16_t>((sent+i)&0x7FFF);
            }
            const auto written=ring.write(chunk.data(),n);
            sent+=written;
            if (written<n)
            {
                // ring full: this simple producer resends the rest of the chunk next time round
                // by re-deriving it from `sent`, so just let the consumer catch up
                std::this_thread::yield();
            }
        }
    });

    size_t received=0;
    std::vector<int16_t> out(500);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);
    while (received<total && std::chrono::steady_clock::now()<deadline)
    {
        const auto n=ring.read(out.data(),out.size());
        if (n==0)
        {
            std::this_thread::yield();
            continue;
        }
        for (size_t i=0;i<n;i++)
        {
            if (out[i]!=static_cast<int16_t>((received+i)&0x7FFF))
            {
                failed=true;
            }
        }
        received+=n;
    }

    producer.join();
    BOOST_CHECK(!failed.load());
    BOOST_CHECK_EQUAL(received,total);
}

BOOST_AUTO_TEST_SUITE_END()
