/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/test/testvoicerecorder.cpp
  *
  *  Tests of the VoiceRecorder state machine and what it produces.
  *
  */

/****************************************************************************/

#include <atomic>
#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <hatn/media/mediaerror.h>
#include <hatn/media/voicerecorder.h>
#include <hatn/media/waveformextractor.h>

#include "testmediautils.h"

HATN_MEDIA_USING

BOOST_AUTO_TEST_SUITE(TestVoiceRecorder)

BOOST_AUTO_TEST_CASE(NewRecorderIsIdle)
{
    VoiceRecorder recorder;
    BOOST_CHECK(recorder.state()==RecorderState::Idle);
    BOOST_CHECK_EQUAL(recorder.elapsedMs(),uint32_t{0});
    BOOST_CHECK(!recorder.limitReached());
    BOOST_CHECK_EQUAL(recorder.overruns(),uint64_t{0});

    // nothing is accepted before start()
    const auto pcm=test::makeSine(480);
    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),pcm.size()),size_t{0});

    VoiceRecording recording;
    BOOST_CHECK(test::isMediaError(recorder.pause(),MediaError::INVALID_STATE));
    BOOST_CHECK(test::isMediaError(recorder.resume(),MediaError::INVALID_STATE));
    BOOST_CHECK(test::isMediaError(recorder.finish(recording),MediaError::INVALID_STATE));
}

#ifndef HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_CASE(StartFailsCleanlyWithoutTheCodec)
{
    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder;
    BOOST_CHECK(test::isMediaError(recorder.start(file),MediaError::CODEC_UNAVAILABLE));
    BOOST_CHECK(recorder.state()==RecorderState::Idle);
}

#else

BOOST_AUTO_TEST_CASE(HappyPathProducesAPlayableFile)
{
    const auto pcm=test::makeSine(96000);   // exactly 2 s
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    BOOST_CHECK_EQUAL(recording.durationMs,uint32_t{2000});
    BOOST_CHECK_EQUAL(recording.sampleRate,VoiceSampleRate);
    BOOST_CHECK_EQUAL(recording.channels,VoiceChannels);
    BOOST_CHECK_EQUAL(recording.waveform.size(),WaveformExtractor::WaveformBuckets);
    BOOST_CHECK_EQUAL(recording.fileBytes,static_cast<uint64_t>(file.bytes().size()));

    // a 440 Hz sine at amplitude 10000 is about -13 dBFS, so the bars are well up the scale
    for (auto bar : recording.waveform)
    {
        BOOST_CHECK_GT(static_cast<int>(bar),150);
    }

    // roughly 3 KB/s at 32 kbps: 2 s is well under 20 KB and well over 2 KB
    BOOST_CHECK_GT(recording.fileBytes,uint64_t{2000});
    BOOST_CHECK_LT(recording.fileBytes,uint64_t{20000});

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(file,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),pcm.size());
}

BOOST_AUTO_TEST_CASE(PartialLastFrameIsTrimmedExactly)
{
    // 1.5 s plus 100 frames: not a multiple of a 20 ms frame
    const auto pcm=test::makeSine(72100);
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    BOOST_CHECK_EQUAL(recording.durationMs,uint32_t{voiceFramesToMs(72100)});

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(file,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),pcm.size());
}

BOOST_AUTO_TEST_CASE(TooShortRecordingIsRefused)
{
    const auto pcm=test::makeSine(14400);   // 300 ms, under the default 1000 ms minimum
    test::MemoryFile file;
    VoiceRecording recording;
    const auto ec=test::recordPcm(file,pcm,recording);
    BOOST_CHECK(test::isMediaError(ec,MediaError::RECORDING_TOO_SHORT));
}

BOOST_AUTO_TEST_CASE(TooShortLeavesTheRecorderCancelled)
{
    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder;
    BOOST_REQUIRE(!recorder.start(file));

    const auto pcm=test::makeSine(4800);
    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),pcm.size()),pcm.size());

    VoiceRecording recording;
    BOOST_CHECK(test::isMediaError(recorder.finish(recording),MediaError::RECORDING_TOO_SHORT));
    BOOST_CHECK(recorder.state()==RecorderState::Cancelled);

    // and it stays dead
    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),pcm.size()),size_t{0});
    BOOST_CHECK(test::isMediaError(recorder.resume(),MediaError::INVALID_STATE));
}

BOOST_AUTO_TEST_CASE(MinimumIsConfigurable)
{
    VoiceRecorderConfig config;
    config.minDurationMs=0;

    const auto pcm=test::makeSine(4800);    // 100 ms
    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_CHECK(!test::recordPcm(file,pcm,recording,480,config));
    BOOST_CHECK_EQUAL(recording.durationMs,uint32_t{100});
}

BOOST_AUTO_TEST_CASE(StateMachineTransitions)
{
    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder;
    const auto pcm=test::makeSine(48000);

    BOOST_REQUIRE(!recorder.start(file));
    BOOST_CHECK(recorder.state()==RecorderState::Recording);

    // starting twice
    BOOST_CHECK(test::isMediaError(recorder.start(file),MediaError::INVALID_STATE));
    // resuming what is not paused
    BOOST_CHECK(test::isMediaError(recorder.resume(),MediaError::INVALID_STATE));

    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),24000),size_t{24000});
    BOOST_REQUIRE(!recorder.pause());
    BOOST_CHECK(recorder.state()==RecorderState::Paused);

    // paused audio is dropped, not queued
    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),24000),size_t{0});
    BOOST_CHECK(test::isMediaError(recorder.pause(),MediaError::INVALID_STATE));

    BOOST_REQUIRE(!recorder.resume());
    BOOST_CHECK(recorder.state()==RecorderState::Recording);
    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),24000),size_t{24000});

    // 24000 + 24000 frames were accepted; the paused stretch does not count
    BOOST_CHECK_EQUAL(recorder.elapsedMs(),uint32_t{1000});

    VoiceRecording recording;
    BOOST_REQUIRE(!recorder.finish(recording));
    BOOST_CHECK(recorder.state()==RecorderState::Finished);
    BOOST_CHECK_EQUAL(recording.durationMs,uint32_t{1000});

    // a finished recorder is done
    BOOST_CHECK(test::isMediaError(recorder.finish(recording),MediaError::INVALID_STATE));
    BOOST_CHECK(test::isMediaError(recorder.pause(),MediaError::INVALID_STATE));
}

BOOST_AUTO_TEST_CASE(FinishWorksFromPaused)
{
    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder;
    const auto pcm=test::makeSine(48000);

    BOOST_REQUIRE(!recorder.start(file));
    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),pcm.size()),pcm.size());
    BOOST_REQUIRE(!recorder.pause());

    VoiceRecording recording;
    BOOST_REQUIRE(!recorder.finish(recording));
    BOOST_CHECK_EQUAL(recording.durationMs,uint32_t{1000});

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(file,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),pcm.size());
}

BOOST_AUTO_TEST_CASE(PauseKeepsBothStretchesInOneFile)
{
    // Record 1 s, pause, record 1 s more: the file is one continuous 2 s stream.
    const auto first=test::makeSine(48000,440.0,10000.0,0);
    const auto second=test::makeSine(48000,440.0,10000.0,48000);

    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder;
    BOOST_REQUIRE(!recorder.start(file));

    BOOST_CHECK_EQUAL(recorder.pushPcm(first.data(),first.size()),first.size());
    BOOST_REQUIRE(!recorder.pause());
    BOOST_REQUIRE(!recorder.resume());
    BOOST_CHECK_EQUAL(recorder.pushPcm(second.data(),second.size()),second.size());

    VoiceRecording recording;
    BOOST_REQUIRE(!recorder.finish(recording));
    BOOST_CHECK_EQUAL(recording.durationMs,uint32_t{2000});

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(file,decoded));
    BOOST_REQUIRE_EQUAL(decoded.size(),size_t{96000});

    // the second stretch continues the sine exactly where the first ended, so the whole thing
    // still correlates with an unbroken 2 s sine
    const auto whole=test::makeSine(96000);
    BOOST_CHECK_GT(test::correlation(whole.data()+2000,decoded.data()+2000,96000-4000),0.95);
}

BOOST_AUTO_TEST_CASE(DurationCapStopsAcceptingAudio)
{
    VoiceRecorderConfig config;
    config.maxDurationMs=1000;
    config.minDurationMs=0;

    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder(config);
    BOOST_REQUIRE(!recorder.start(file));

    const auto pcm=test::makeSine(72000);   // 1.5 s offered
    // only the first 48000 frames fit under the cap, the rest of the call is refused
    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),pcm.size()),size_t{48000});
    BOOST_CHECK(recorder.limitReached());
    BOOST_CHECK_EQUAL(recorder.elapsedMs(),uint32_t{1000});
    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),480),size_t{0});

    // a paused recorder at the cap cannot be resumed to record nothing
    BOOST_REQUIRE(!recorder.pause());
    BOOST_CHECK(test::isMediaError(recorder.resume(),MediaError::INVALID_STATE));

    VoiceRecording recording;
    BOOST_REQUIRE(!recorder.finish(recording));
    BOOST_CHECK_EQUAL(recording.durationMs,uint32_t{1000});
}

BOOST_AUTO_TEST_CASE(RingOverrunIsCountedNotSilent)
{
    VoiceRecorderConfig config;
    config.ringMs=100;      // 4800 frames, rounded up to 8192

    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder(config);
    BOOST_REQUIRE(!recorder.start(file));

    // one second in a single burst with no process() in between overflows a 100 ms ring
    const auto pcm=test::makeSine(48000);
    const auto accepted=recorder.pushPcm(pcm.data(),pcm.size());
    BOOST_CHECK_LT(accepted,pcm.size());
    BOOST_CHECK_EQUAL(recorder.overruns(),static_cast<uint64_t>(pcm.size()-accepted));
    recorder.cancel();
}

BOOST_AUTO_TEST_CASE(CancelEndsTheRecording)
{
    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder;
    BOOST_REQUIRE(!recorder.start(file));

    const auto pcm=test::makeSine(48000);
    recorder.pushPcm(pcm.data(),pcm.size());
    recorder.cancel();

    BOOST_CHECK(recorder.state()==RecorderState::Cancelled);
    BOOST_CHECK_EQUAL(recorder.pushPcm(pcm.data(),pcm.size()),size_t{0});

    VoiceRecording recording;
    BOOST_CHECK(test::isMediaError(recorder.finish(recording),MediaError::INVALID_STATE));

    // cancelling again, or after the fact, is harmless
    recorder.cancel();
    BOOST_CHECK(recorder.state()==RecorderState::Cancelled);
}

BOOST_AUTO_TEST_CASE(LevelFollowsTheAudio)
{
    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder;
    BOOST_REQUIRE(!recorder.start(file));
    BOOST_CHECK_EQUAL(recorder.levelDb(),static_cast<float>(WaveformExtractor::FloorDb));

    const auto loud=test::makeSine(4800,440.0,20000.0);
    recorder.pushPcm(loud.data(),loud.size());
    BOOST_REQUIRE(!recorder.process());
    // amplitude 20000 sine: RMS 14142, about -7.3 dBFS
    BOOST_CHECK_GT(recorder.levelDb(),-10.0f);
    BOOST_CHECK_LT(recorder.levelDb(),-5.0f);

    const std::vector<int16_t> quiet(4800,0);
    recorder.pushPcm(quiet.data(),quiet.size());
    BOOST_REQUIRE(!recorder.process());
    BOOST_CHECK_EQUAL(recorder.levelDb(),static_cast<float>(WaveformExtractor::FloorDb));

    recorder.cancel();
}

BOOST_AUTO_TEST_CASE(WaveformFollowsTheRecordedAudio)
{
    // loud first half, silent second half: the stored bars must show exactly that shape
    std::vector<int16_t> pcm=test::makeSine(48000,440.0,20000.0);
    pcm.resize(96000,0);

    test::MemoryFile file;
    VoiceRecording recording;
    BOOST_REQUIRE(!test::recordPcm(file,pcm,recording));

    BOOST_REQUIRE_EQUAL(recording.waveform.size(),WaveformExtractor::WaveformBuckets);
    for (size_t i=0;i<45;i++)
    {
        BOOST_CHECK_GT(static_cast<int>(recording.waveform[i]),150);
    }
    for (size_t i=55;i<100;i++)
    {
        BOOST_CHECK_EQUAL(static_cast<int>(recording.waveform[i]),0);
    }
}

BOOST_AUTO_TEST_CASE(AudioThreadAndWorkerThreadRunConcurrently)
{
    // The recorder's contract: an audio thread that only calls pushPcm(), a worker that only calls
    // process(), and a control thread that finishes. Run under ThreadSanitizer this checks the
    // wait-free push path and the mutex hand-off.
    const auto pcm=test::makeSine(144000);   // 3 s

    test::MemoryFile file;
    test::openNew(file);
    VoiceRecorder recorder;
    BOOST_REQUIRE(!recorder.start(file));

    std::atomic<bool> audioDone{false};
    std::atomic<bool> workerFailed{false};
    std::atomic<uint64_t> accepted{0};

    std::thread audio([&]()
    {
        // a capture callback: fixed-size pieces, no retry of anything that was refused
        for (size_t offset=0;offset<pcm.size();offset+=480)
        {
            accepted+=recorder.pushPcm(pcm.data()+offset,480);
            std::this_thread::yield();
        }
        audioDone=true;
    });

    std::thread worker([&]()
    {
        while (!audioDone.load())
        {
            if (recorder.process())
            {
                workerFailed=true;
                return;
            }
            std::this_thread::yield();
        }
    });

    audio.join();
    worker.join();
    BOOST_CHECK(!workerFailed.load());

    VoiceRecording recording;
    BOOST_REQUIRE(!recorder.finish(recording));

    // pushPcm() reports what it wrote, so every offered frame is either accepted or counted as an
    // overrun; and whatever the scheduler did, everything ACCEPTED is in the file, exactly.
    BOOST_CHECK_EQUAL(accepted.load()+recorder.overruns(),uint64_t{pcm.size()});
    BOOST_CHECK_EQUAL(recording.durationMs,static_cast<uint32_t>(voiceFramesToMs(accepted.load())));

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(file,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),static_cast<size_t>(accepted.load()));
}

#endif // HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_SUITE_END()
