/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/test/testvoicecrop.cpp
  *
  *  Tests of cropVoice().
  *
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/media/mediaerror.h>
#include <hatn/media/voicecrop.h>
#include <hatn/media/waveformextractor.h>

#include "testmediautils.h"

HATN_MEDIA_USING

BOOST_AUTO_TEST_SUITE(TestVoiceCrop)

#ifndef HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_CASE(FailsCleanlyWithoutTheCodec)
{
    test::MemoryFile in;
    test::MemoryFile out;
    test::openNew(in);
    test::openNew(out);
    VoiceRecording recording;
    BOOST_CHECK(test::isMediaError(cropVoice(in,out,0,48000,recording),MediaError::CODEC_UNAVAILABLE));
}

#else

namespace {

//! 3 s made of three different tones, so a crop can be told apart by its CONTENT, not just its length.
std::vector<int16_t> threeTones()
{
    auto pcm=test::makeSine(48000,440.0,10000.0,0);
    const auto second=test::makeSine(48000,880.0,10000.0,48000);
    const auto third=test::makeSine(48000,1320.0,10000.0,96000);
    pcm.insert(pcm.end(),second.begin(),second.end());
    pcm.insert(pcm.end(),third.begin(),third.end());
    return pcm;
}

}

BOOST_AUTO_TEST_CASE(CropKeepsExactlyTheRequestedRange)
{
    const auto pcm=threeTones();
    test::MemoryFile in;
    VoiceRecording original;
    BOOST_REQUIRE(!test::recordPcm(in,pcm,original));

    // the reference: the original decoded, which is what a listener would have heard
    std::vector<int16_t> full;
    BOOST_REQUIRE(!test::decodeAll(in,full));
    BOOST_REQUIRE_EQUAL(full.size(),pcm.size());

    // the second tone only
    test::MemoryFile out;
    test::openNew(out);
    VoiceRecording cropped;
    BOOST_REQUIRE(!cropVoice(in,out,48000,96000,cropped));

    BOOST_CHECK_EQUAL(cropped.durationMs,uint32_t{1000});
    BOOST_CHECK_EQUAL(cropped.fileBytes,static_cast<uint64_t>(out.bytes().size()));
    BOOST_CHECK_EQUAL(cropped.waveform.size(),WaveformExtractor::WaveformBuckets);

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(out,decoded));

    // exact length
    BOOST_REQUIRE_EQUAL(decoded.size(),size_t{48000});

    // the right audio, in the right place: it matches the second tone, and clearly not the first
    const size_t margin=2000;
    BOOST_CHECK_GT(test::correlation(full.data()+48000+margin,decoded.data()+margin,48000-2*margin),0.9);
    BOOST_CHECK_LT(std::abs(test::correlation(full.data()+margin,decoded.data()+margin,48000-2*margin)),0.5);
}

BOOST_AUTO_TEST_CASE(CropWithOddBoundariesIsExact)
{
    // boundaries that fall inside Opus frames and inside waveform windows
    const auto pcm=threeTones();
    test::MemoryFile in;
    VoiceRecording original;
    BOOST_REQUIRE(!test::recordPcm(in,pcm,original));

    const uint64_t start=12345;
    const uint64_t end=98765;

    test::MemoryFile out;
    test::openNew(out);
    VoiceRecording cropped;
    BOOST_REQUIRE(!cropVoice(in,out,start,end,cropped));
    BOOST_CHECK_EQUAL(cropped.durationMs,static_cast<uint32_t>(voiceFramesToMs(end-start)));

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(out,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),static_cast<size_t>(end-start));
}

BOOST_AUTO_TEST_CASE(WaveformIsRecomputedFromTheCroppedAudio)
{
    // loud second, silent second; crop the middle 1 s: half loud then half silent
    auto pcm=test::makeSine(48000,440.0,20000.0);
    pcm.resize(96000,0);

    test::MemoryFile in;
    VoiceRecording original;
    BOOST_REQUIRE(!test::recordPcm(in,pcm,original));

    test::MemoryFile out;
    test::openNew(out);
    VoiceRecording cropped;
    BOOST_REQUIRE(!cropVoiceMs(in,out,500,1500,cropped));

    BOOST_REQUIRE_EQUAL(cropped.waveform.size(),WaveformExtractor::WaveformBuckets);
    // The loud half is decoded audio and the silent half is decoded silence, but the stored
    // waveform comes from what was RE-ENCODED, so allow a little pre-echo around the seam.
    for (size_t i=0;i<45;i++)
    {
        BOOST_CHECK_GT(static_cast<int>(cropped.waveform[i]),150);
    }
    for (size_t i=60;i<100;i++)
    {
        BOOST_CHECK_LT(static_cast<int>(cropped.waveform[i]),40);
    }
}

BOOST_AUTO_TEST_CASE(EndPastTheMessageIsClamped)
{
    const auto pcm=test::makeSine(96000);
    test::MemoryFile in;
    VoiceRecording original;
    BOOST_REQUIRE(!test::recordPcm(in,pcm,original));

    test::MemoryFile out;
    test::openNew(out);
    VoiceRecording cropped;
    BOOST_REQUIRE(!cropVoice(in,out,48000,9999999,cropped));
    BOOST_CHECK_EQUAL(cropped.durationMs,uint32_t{1000});

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(out,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),size_t{48000});
}

BOOST_AUTO_TEST_CASE(WholeRangeKeepsTheWholeMessage)
{
    const auto pcm=test::makeSine(120480);
    test::MemoryFile in;
    VoiceRecording original;
    BOOST_REQUIRE(!test::recordPcm(in,pcm,original));

    test::MemoryFile out;
    test::openNew(out);
    VoiceRecording cropped;
    BOOST_REQUIRE(!cropVoice(in,out,0,pcm.size(),cropped));
    BOOST_CHECK_EQUAL(cropped.durationMs,original.durationMs);

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(out,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),pcm.size());
}

BOOST_AUTO_TEST_CASE(ShortCropIsAllowed)
{
    // no minimum here, unlike a live recording
    const auto pcm=test::makeSine(96000);
    test::MemoryFile in;
    VoiceRecording original;
    BOOST_REQUIRE(!test::recordPcm(in,pcm,original));

    test::MemoryFile out;
    test::openNew(out);
    VoiceRecording cropped;
    BOOST_REQUIRE(!cropVoice(in,out,10000,10480,cropped));   // 10 ms

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(out,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),size_t{480});
}

BOOST_AUTO_TEST_CASE(RejectsBadRanges)
{
    const auto pcm=test::makeSine(96000);
    test::MemoryFile in;
    VoiceRecording original;
    BOOST_REQUIRE(!test::recordPcm(in,pcm,original));

    test::MemoryFile out;
    test::openNew(out);
    VoiceRecording cropped;

    // empty and backwards
    BOOST_CHECK(test::isMediaError(cropVoice(in,out,1000,1000,cropped),MediaError::INVALID_ARGUMENT));
    BOOST_CHECK(test::isMediaError(cropVoice(in,out,2000,1000,cropped),MediaError::INVALID_ARGUMENT));
    // starts at or past the end
    BOOST_CHECK(test::isMediaError(cropVoice(in,out,96000,100000,cropped),MediaError::INVALID_ARGUMENT));
    BOOST_CHECK(test::isMediaError(cropVoice(in,out,500000,600000,cropped),MediaError::INVALID_ARGUMENT));
    // the same file as source and destination
    BOOST_CHECK(test::isMediaError(cropVoice(in,in,0,1000,cropped),MediaError::INVALID_ARGUMENT));

    // not a voice message at all
    test::MemoryFile garbage;
    test::openNew(garbage);
    const char text[]="not an ogg file, just some text long enough to be read as a chunk of garbage";
    garbage.write(text,sizeof(text));
    BOOST_CHECK(cropVoice(garbage,out,0,1000,cropped));
}

BOOST_AUTO_TEST_CASE(CropOfACropWorks)
{
    // trimming twice, as a user adjusting the handles again and again would
    const auto pcm=threeTones();
    test::MemoryFile in;
    VoiceRecording original;
    BOOST_REQUIRE(!test::recordPcm(in,pcm,original));

    test::MemoryFile first;
    test::openNew(first);
    VoiceRecording firstRecording;
    BOOST_REQUIRE(!cropVoice(in,first,24000,120000,firstRecording));   // 2 s

    test::MemoryFile second;
    test::openNew(second);
    VoiceRecording secondRecording;
    BOOST_REQUIRE(!cropVoice(first,second,24000,72000,secondRecording));   // 1 s from the middle

    std::vector<int16_t> decoded;
    BOOST_REQUIRE(!test::decodeAll(second,decoded));
    BOOST_CHECK_EQUAL(decoded.size(),size_t{48000});
    BOOST_CHECK_EQUAL(secondRecording.durationMs,uint32_t{1000});
}

#endif // HATN_MEDIA_HAS_OGG_OPUS

BOOST_AUTO_TEST_SUITE_END()
