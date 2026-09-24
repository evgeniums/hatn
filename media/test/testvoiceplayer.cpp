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
#include <cmath>
#include <limits>
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

BOOST_AUTO_TEST_CASE(SpeedIsClampedAndKept)
{
    VoicePlayer player;
    BOOST_CHECK_EQUAL(player.speed(),1.0f);

    player.setSpeed(10.0f);
    BOOST_CHECK_EQUAL(player.speed(),MaxPlaybackSpeed);
    player.setSpeed(0.1f);
    BOOST_CHECK_EQUAL(player.speed(),MinPlaybackSpeed);

    // not a number: ignored
    player.setSpeed(std::numeric_limits<float>::quiet_NaN());
    BOOST_CHECK_EQUAL(player.speed(),MinPlaybackSpeed);

    player.setSpeed(1.25f);
    BOOST_CHECK_EQUAL(player.speed(),1.25f);

    // a closed player has nothing to re-time
    BOOST_CHECK(!player.needsFill());
    BOOST_CHECK(!player.fill());
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

namespace {

//! fill()/pull() rounds until `count` frames have come out, and no more than that.
std::vector<int16_t> pullFrames(VoicePlayer& player, size_t count, size_t pullChunk=480)
{
    std::vector<int16_t> result;
    std::vector<int16_t> chunk(pullChunk);
    for (size_t i=0;i<100000 && result.size()<count;i++)
    {
        BOOST_REQUIRE(!player.fill());
        const auto n=player.pull(chunk.data(),std::min(chunk.size(),count-result.size()));
        result.insert(result.end(),chunk.begin(),chunk.begin()+static_cast<std::ptrdiff_t>(n));
    }
    return result;
}

//! How many frames a stretch of `frames` comes out as at `speed`.
size_t stretchedLength(size_t frames, float speed)
{
    return static_cast<size_t>(std::llround(static_cast<double>(frames)/static_cast<double>(speed)));
}

}

BOOST_AUTO_TEST_CASE(SpeedChangesTheLengthNotThePitch)
{
    const auto pcm=test::makeSine(144000,300.0);   // 3 s
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    for (const auto speed : {2.0f,0.5f,1.5f})
    {
        // set before open(): the speed is kept
        VoicePlayer player;
        player.setSpeed(speed);
        BOOST_REQUIRE(!player.open(file));
        BOOST_CHECK_EQUAL(player.speed(),speed);
        player.play();

        const auto played=playThrough(player);
        BOOST_CHECK(player.state()==PlayerState::Ended);

        const auto expected=stretchedLength(pcm.size(),speed);
        BOOST_CHECK_MESSAGE(played.size()==expected,
                            "speed " << speed << ": " << played.size() << " frames came out, expected " << expected);

        // the pitch is the pitch of the message, not of the speed
        BOOST_REQUIRE_GT(played.size(),size_t{8000});
        const auto frequency=test::toneFrequency(played.data()+played.size()/4,played.size()/2);
        BOOST_CHECK_MESSAGE(std::fabs(frequency-300.0)<9.0,
                            "a 300 Hz tone at speed " << speed << " came out as " << frequency << " Hz");

        // the clock is in time of the message: it ends on the end, and the duration did not change
        BOOST_CHECK_EQUAL(player.positionMs(),player.durationMs());
        BOOST_CHECK_EQUAL(player.durationMs(),uint64_t{3000});
    }
}

BOOST_AUTO_TEST_CASE(PositionIsInTimeOfTheMessage)
{
    const auto pcm=test::makeSine(144000,300.0);   // 3 s
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    player.setSpeed(2.0f);
    BOOST_REQUIRE(!player.open(file));
    player.play();
    BOOST_REQUIRE(!player.fill());

    // 100 ms of audio at twice the speed is 200 ms of the message
    std::vector<int16_t> out(4800);
    BOOST_REQUIRE_EQUAL(player.pull(out.data(),out.size()),out.size());
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{200});
    BOOST_REQUIRE_EQUAL(player.pull(out.data(),out.size()),out.size());
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{400});
    BOOST_CHECK_EQUAL(player.durationMs(),uint64_t{3000});
}

BOOST_AUTO_TEST_CASE(ChangingTheSpeedWhilePlayingContinuesFromTheSamePlace)
{
    const auto pcm=test::makeSine(144000,300.0);   // 3 s
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    player.play();
    BOOST_REQUIRE(!player.fill());

    // half a second in at normal speed, with about a second of audio decoded ahead
    std::vector<int16_t> out(24000);
    BOOST_REQUIRE_EQUAL(player.pull(out.data(),out.size()),out.size());
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{500});

    // The change is applied by the decode thread like a seek to where the listener is, so it does
    // not wait for the buffered audio to play out.
    player.setSpeed(2.0f);
    BOOST_CHECK(player.needsFill());
    BOOST_REQUIRE(!player.fill());

    // The next pull() drops what was buffered at the old speed, and the clock continues from 500 ms
    // at twice the speed: 480 frames are 20 ms of audio and 20 ms x 2 = 40 ms of the message.
    std::vector<int16_t> first(480);
    BOOST_REQUIRE_EQUAL(player.pull(first.data(),first.size()),first.size());
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{520});

    // and it is the same tone, faster, not a higher one
    const auto rest=pullFrames(player,24000);
    BOOST_REQUIRE_EQUAL(rest.size(),size_t{24000});
    const auto frequency=test::toneFrequency(rest.data(),rest.size());
    BOOST_CHECK_MESSAGE(std::fabs(frequency-300.0)<9.0,"a 300 Hz tone came out as " << frequency << " Hz after the change");

    BOOST_CHECK_EQUAL(player.positionMs(),voiceFramesToMs(24000+2*(480+24000)));

    // back to normal speed from there, and to the end
    player.setSpeed(1.0f);
    BOOST_REQUIRE(!player.fill());
    const auto position=player.positionMs();
    const auto tail=playThrough(player);
    BOOST_CHECK(player.state()==PlayerState::Ended);
    BOOST_CHECK_EQUAL(player.positionMs(),player.durationMs());
    BOOST_CHECK_GT(position,uint64_t{1400});

    // what is left of the message plays at normal speed: about its length in frames
    const auto remaining=voiceMsToFrames(3000-position);
    BOOST_CHECK_LE(std::abs(static_cast<long long>(tail.size())-static_cast<long long>(remaining)),static_cast<long long>(2000));
}

BOOST_AUTO_TEST_CASE(SpeedChangeWhilePausedKeepsItPausedAndSurvivesReopening)
{
    const auto pcm=test::makeSine(96000,300.0);   // 2 s
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    player.setSpeed(1.5f);
    BOOST_CHECK(player.needsFill());
    BOOST_REQUIRE(!player.fill());
    BOOST_CHECK(player.state()==PlayerState::Paused);
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{0});

    player.play();
    const auto expected=stretchedLength(pcm.size(),1.5f);
    BOOST_CHECK_EQUAL(playThrough(player).size(),expected);

    // close() and open() keep the speed, and it is applied from the start
    player.close();
    BOOST_CHECK_EQUAL(player.speed(),1.5f);
    BOOST_REQUIRE(!player.open(file));
    BOOST_CHECK(player.state()==PlayerState::Paused);
    BOOST_CHECK_EQUAL(player.speed(),1.5f);
    player.play();
    BOOST_CHECK_EQUAL(playThrough(player).size(),expected);
}

BOOST_AUTO_TEST_CASE(SpeedChangeAfterTheEndAppliesToTheNextPlay)
{
    const auto pcm=test::makeSine(48000,300.0);   // 1 s
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    BOOST_REQUIRE(!player.open(file));
    player.play();
    BOOST_CHECK_EQUAL(playThrough(player).size(),pcm.size());
    BOOST_CHECK(player.state()==PlayerState::Ended);

    // an ended player has nothing to re-time: it stays ended and does not start playing
    player.setSpeed(2.0f);
    BOOST_CHECK(player.needsFill());
    BOOST_REQUIRE(!player.fill());
    BOOST_CHECK(player.state()==PlayerState::Ended);
    BOOST_CHECK(!player.needsFill());

    // play() starts the message over, at the new speed
    player.play();
    const auto played=playThrough(player);
    BOOST_CHECK_EQUAL(played.size(),stretchedLength(pcm.size(),2.0f));
    BOOST_CHECK_EQUAL(player.positionMs(),player.durationMs());
}

BOOST_AUTO_TEST_CASE(SeekWorksAtAnySpeed)
{
    const auto pcm=test::makeSine(144000,300.0);   // 3 s
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    VoicePlayer player;
    player.setSpeed(2.0f);
    BOOST_REQUIRE(!player.open(file));
    player.play();

    player.seekMs(2000);
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{2000});
    BOOST_CHECK(player.needsFill());
    BOOST_REQUIRE(!player.fill());

    // 480 frames of audio at twice the speed are 960 frames of the message, 20 ms
    std::vector<int16_t> first(480);
    BOOST_REQUIRE_EQUAL(player.pull(first.data(),first.size()),first.size());
    BOOST_CHECK_EQUAL(player.positionMs(),uint64_t{2020});

    // the last second of the message is half a second of audio
    const auto rest=playThrough(player);
    BOOST_CHECK(player.state()==PlayerState::Ended);
    BOOST_CHECK_EQUAL(first.size()+rest.size(),stretchedLength(48000,2.0f));
    BOOST_REQUIRE_GT(rest.size(),size_t{8000});
    const auto frequency=test::toneFrequency(rest.data(),rest.size());
    BOOST_CHECK_MESSAGE(std::fabs(frequency-300.0)<9.0,"a 300 Hz tone came out as " << frequency << " Hz after a seek");
    BOOST_CHECK_EQUAL(player.positionMs(),player.durationMs());
}

BOOST_AUTO_TEST_CASE(ThreeThreadsWithSpeedChangesInFlight)
{
    // As ThreeThreadsWithSeeksInFlight, with the control thread also changing the speed. Under
    // ThreadSanitizer this is what checks the speed's part of the seek protocol.
    const auto pcm=test::makeSine(240000,300.0);   // 5 s
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

    // change the speed, sometimes together with a seek, while audio is flowing
    size_t index=0;
    for (const auto speed : {2.0f,0.5f,1.0f,1.5f,2.0f,0.75f,1.25f})
    {
        player.setSpeed(speed);
        if (index%2==1)
        {
            player.seekMs(index*600);
        }
        index++;
        BOOST_CHECK_LE(player.positionMs(),player.durationMs());
        const auto before=pulled.load();
        waitFor([&](){return pulled.load()>before+4800;});
        BOOST_CHECK_LE(player.positionMs(),player.durationMs());
    }

    // and finally play it to the end at normal speed
    player.setSpeed(1.0f);
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
