/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/test/testvoiceplayer.cpp
  *
  *  Tests of VoicePlayer. The player is driven the way its three roles would drive it, but from
  *  one thread, which is a legal way to use it and keeps the tests deterministic.
  *
  */

/****************************************************************************/

#include <atomic>
#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <hatn/media/mediaerror.h>
#include <hatn/media/voiceplayer.h>

#include "testmediautils.h"

HATN_MEDIA_USING

BOOST_AUTO_TEST_SUITE(TestVoicePlayer)

BOOST_AUTO_TEST_CASE(NewPlayerIsClosed)
{
    VoicePlayer player;
    BOOST_CHECK(player.state()==PlayerState::Closed);
    BOOST_CHECK_EQUAL(player.durationMs(),uint64_t{0});
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{0});
    BOOST_CHECK(!player.needsFill());

    // everything is a harmless no-op on a closed player
    player.play();
    player.pause();
    player.seekMs(1000);
    BOOST_CHECK(!player.fill());
    int16_t out[16];
    BOOST_CHECK_EQUAL(player.pull(out,16),size_t{0});
    BOOST_CHECK(player.state()==PlayerState::Closed);
}

#ifndef HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_CASE(OpenFailsCleanlyWithoutTheCodec)
{
    test::MemoryFile file;
    test::openNew(file);
    VoicePlayer player;
    BOOST_CHECK(test::isMediaError(player.open(file),MediaError::CODEC_UNAVAILABLE));
    BOOST_CHECK(player.state()==PlayerState::Closed);
}

#else

namespace {

//! Play to the end from one thread: fill, pull, repeat. Returns everything that came out.
std::vector<int16_t> playThrough(VoicePlayer& player, size_t pullChunk=480, size_t maxIterations=100000)
{
    std::vector<int16_t> result;
    std::vector<int16_t> chunk(pullChunk);
    for (size_t i=0;i<maxIterations && player.state()!=PlayerState::Ended;i++)
    {
        BOOST_REQUIRE(!player.fill());
        const auto n=player.pull(chunk.data(),chunk.size());
        result.insert(result.end(),chunk.begin(),chunk.begin()+static_cast<std::ptrdiff_t>(n));
    }
    return result;
}

}

BOOST_AUTO_TEST_CASE(OpensPausedAtTheStart)
{
    const auto pcm=test::makeSine(96000);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    BOOST_CHECK(player.state()==PlayerState::Paused);
    BOOST_CHECK_EQUAL(player.durationMs(),uint64_t{2000});
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{0});

    // opening an open player is a mistake
    BOOST_CHECK(test::isMediaError(player.open(file),MediaError::INVALID_STATE));

    // paused means no audio, however much is decoded
    BOOST_REQUIRE(!player.fill());
    int16_t out[480];
    BOOST_CHECK_EQUAL(player.pull(out,480),size_t{0});
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{0});

    player.close();
    BOOST_CHECK(player.state()==PlayerState::Closed);
}

BOOST_AUTO_TEST_CASE(PlaysEverythingAndEnds)
{
    const auto pcm=test::makeSine(120480);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    player.play();
    BOOST_CHECK(player.state()==PlayerState::Playing);

    const auto played=playThrough(player);
    BOOST_CHECK(player.state()==PlayerState::Ended);

    // exactly the recorded length, and the audio of the recording
    BOOST_REQUIRE_EQUAL(played.size(),pcm.size());
    BOOST_CHECK_GT(test::correlation(pcm.data()+2000,played.data()+2000,pcm.size()-4000),0.95);

    // the clock reached the end
    BOOST_CHECK_EQUAL(player.positionMs(),player.durationMs());
}

BOOST_AUTO_TEST_CASE(PositionAdvancesWithWhatIsPulled)
{
    const auto pcm=test::makeSine(96000);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    player.play();
    BOOST_REQUIRE(!player.fill());

    std::vector<int16_t> out(4800);
    BOOST_REQUIRE_EQUAL(player.pull(out.data(),out.size()),out.size());
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{100});
    BOOST_REQUIRE_EQUAL(player.pull(out.data(),out.size()),out.size());
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{200});
}

BOOST_AUTO_TEST_CASE(PauseStopsAndPlayResumesWhereItWas)
{
    const auto pcm=test::makeSine(96000);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    player.play();
    BOOST_REQUIRE(!player.fill());

    std::vector<int16_t> out(4800);
    BOOST_REQUIRE_EQUAL(player.pull(out.data(),out.size()),out.size());

    player.pause();
    BOOST_CHECK(player.state()==PlayerState::Paused);
    BOOST_CHECK_EQUAL(player.pull(out.data(),out.size()),size_t{0});
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{100});

    player.play();
    BOOST_REQUIRE_EQUAL(player.pull(out.data(),out.size()),out.size());
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{200});
}

BOOST_AUTO_TEST_CASE(SeekJumpsAndReportsTheTargetAtOnce)
{
    const auto pcm=test::makeSine(144000);   // 3 s
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    // the sequential decode is the reference for what should be heard after a seek
    std::vector<int16_t> full;
    BOOST_REQUIRE(!test::decodeAll(file,full));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    player.play();
    BOOST_REQUIRE(!player.fill());

    std::vector<int16_t> out(2400);
    BOOST_REQUIRE_EQUAL(player.pull(out.data(),out.size()),out.size());

    // The seek is pending until the decode thread applies it, but the UI must see the new
    // position immediately or a slider would snap back and forth.
    player.seekMs(2000);
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{2000});
    BOOST_CHECK(player.needsFill());

    // The decode thread applies the seek and decodes a little from the new position. Only then
    // does the audio thread's next pull() notice the seek and drop the old buffered audio, so
    // right after it just the fresh audio is left; the decode thread tops the buffer up again.
    BOOST_REQUIRE(!player.fill());
    std::vector<int16_t> first(480);
    BOOST_REQUIRE_EQUAL(player.pull(first.data(),first.size()),first.size());
    BOOST_REQUIRE(!player.fill());
    BOOST_REQUIRE_EQUAL(player.pull(out.data(),out.size()),out.size());

    // Audio from before the seek was dropped: what comes out is the audio at 2 s, not what was
    // buffered ahead of the old position, and it is continuous across the two pulls.
    const auto target=static_cast<size_t>(voiceMsToFrames(2000));
    BOOST_CHECK_GT(test::correlation(first.data(),full.data()+target,first.size()),0.99);
    BOOST_CHECK_GT(test::correlation(out.data(),full.data()+target+first.size(),out.size()),0.99);

    // the clock started from the seek target: 2000 ms + (480 + 2400) frames = 60 ms
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{2060});
}

BOOST_AUTO_TEST_CASE(SeekBackwardsAndBeyondTheEnd)
{
    const auto pcm=test::makeSine(96000);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    player.play();

    // beyond the end clamps to the end
    player.seekMs(999999);
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{2000});
    BOOST_REQUIRE(!player.fill());
    std::vector<int16_t> out(480);
    BOOST_CHECK_EQUAL(player.pull(out.data(),out.size()),size_t{0});
    BOOST_CHECK(player.state()==PlayerState::Ended);

    // and back to the start plays the whole message again
    player.seekMs(0);
    player.play();
    const auto played=playThrough(player);
    BOOST_CHECK_EQUAL(played.size(),pcm.size());
}

BOOST_AUTO_TEST_CASE(PlayingAnEndedMessageRestartsIt)
{
    const auto pcm=test::makeSine(48000);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    player.play();
    BOOST_CHECK_EQUAL(playThrough(player).size(),pcm.size());
    BOOST_CHECK(player.state()==PlayerState::Ended);

    player.play();
    BOOST_CHECK(player.state()==PlayerState::Playing);
    BOOST_CHECK_EQUAL(playThrough(player).size(),pcm.size());
}

BOOST_AUTO_TEST_CASE(SmallAudioBufferStillPlaysEverything)
{
    // A buffer far smaller than the message forces many fill()/pull() rounds and exercises the
    // ring's wrap-around and "full, wait for the consumer" path.
    const auto pcm=test::makeSine(120000);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player(100);
    BOOST_REQUIRE(!player.open(file));
    player.play();

    const auto played=playThrough(player,700);
    BOOST_REQUIRE_EQUAL(played.size(),pcm.size());
    BOOST_CHECK_GT(test::correlation(pcm.data()+2000,played.data()+2000,pcm.size()-4000),0.95);
}

BOOST_AUTO_TEST_CASE(ThreeThreadsWithSeeksInFlight)
{
    // The three roles on three real threads: an audio thread that only pulls, a decode thread that
    // only fills, and a control thread that plays, seeks around and finally lets it run to the end.
    // Run under ThreadSanitizer this is what checks the wait-free claim and the seek protocol.
    const auto pcm=test::makeSine(240000);   // 5 s
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player(200);
    BOOST_REQUIRE(!player.open(file));

    std::atomic<bool> stop{false};
    std::atomic<bool> decodeFailed{false};
    std::atomic<uint64_t> pulled{0};

    std::thread decoder([&]()
    {
        while (!stop.load())
        {
            if (player.needsFill())
            {
                if (player.fill())
                {
                    decodeFailed=true;
                    return;
                }
            }
            else
            {
                std::this_thread::yield();
            }
        }
    });

    std::thread audio([&]()
    {
        std::vector<int16_t> chunk(480);
        while (!stop.load())
        {
            const auto n=player.pull(chunk.data(),chunk.size());
            pulled+=n;
            if (n==0)
            {
                std::this_thread::yield();
            }
        }
    });

    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(60);
    auto waitFor=[&](auto&& condition)
    {
        while (!condition() && std::chrono::steady_clock::now()<deadline)
        {
            std::this_thread::yield();
        }
    };

    player.play();
    waitFor([&](){return pulled.load()>24000;});

    // seek around while audio is flowing
    for (uint64_t ms : {3000,500,4000,0,2500,100})
    {
        player.seekMs(ms);
        BOOST_CHECK_LE(player.positionMs(),player.durationMs());
        const auto before=pulled.load();
        waitFor([&](){return pulled.load()>before+4800;});
        BOOST_CHECK_LE(player.positionMs(),player.durationMs());
    }

    // and finally play it to the end
    player.seekMs(0);
    player.play();
    waitFor([&](){return player.state()==PlayerState::Ended;});

    stop=true;
    decoder.join();
    audio.join();

    BOOST_CHECK(!decodeFailed.load());
    BOOST_CHECK(player.state()==PlayerState::Ended);
    BOOST_CHECK_EQUAL(player.positionMs(),player.durationMs());
}

#endif // HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_SUITE_END()
