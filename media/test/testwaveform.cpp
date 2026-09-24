/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/test/testwaveform.cpp
  *
  *  Tests of WaveformExtractor. The waveform is part of the message format, so these pin the
  *  constants and the mapping, not just "it produces something".
  *
  */

/****************************************************************************/

#include <random>

#include <boost/test/unit_test.hpp>

#include <hatn/media/waveformextractor.h>

#include "testmediautils.h"

HATN_MEDIA_USING

BOOST_AUTO_TEST_SUITE(TestWaveform)

BOOST_AUTO_TEST_CASE(FormatConstants)
{
    // Changing any of these changes how every already-sent message renders. If this test has to
    // change, the message format is changing with it.
    BOOST_CHECK_EQUAL(WaveformExtractor::WaveformBuckets,size_t{100});
    BOOST_CHECK_EQUAL(WaveformExtractor::FloorDb,-60);
    BOOST_CHECK_EQUAL(WaveformExtractor::WindowFrames,size_t{240});
}

BOOST_AUTO_TEST_CASE(BucketCountIsFixed)
{
    for (size_t frames : {size_t{0},size_t{1},size_t{239},size_t{240},size_t{241},size_t{24000},size_t{480000}})
    {
        std::vector<int16_t> pcm(frames,1000);
        const auto buckets=WaveformExtractor::compute(pcm.data(),pcm.size());
        BOOST_CHECK_EQUAL(buckets.size(),WaveformExtractor::WaveformBuckets);
    }
}

BOOST_AUTO_TEST_CASE(SilenceIsZero)
{
    std::vector<int16_t> pcm(48000,0);
    const auto buckets=WaveformExtractor::compute(pcm.data(),pcm.size());
    for (auto b : buckets)
    {
        BOOST_CHECK_EQUAL(static_cast<int>(b),0);
    }

    // and no input at all is the same, not an error
    WaveformExtractor empty;
    const auto none=empty.buckets();
    BOOST_REQUIRE_EQUAL(none.size(),WaveformExtractor::WaveformBuckets);
    for (auto b : none)
    {
        BOOST_CHECK_EQUAL(static_cast<int>(b),0);
    }
}

BOOST_AUTO_TEST_CASE(FullScaleIsMax)
{
    std::vector<int16_t> pcm(48000,32767);
    const auto buckets=WaveformExtractor::compute(pcm.data(),pcm.size());
    for (auto b : buckets)
    {
        BOOST_CHECK_EQUAL(static_cast<int>(b),255);
    }
}

BOOST_AUTO_TEST_CASE(LogScaleMapping)
{
    // A constant signal of amplitude A has RMS A. -20 dBFS is 3276.8 and must land a third of the
    // way up the 60 dB range: (60-20)/60*255 = 170. -40 dBFS is 327.68 -> 85. -6 dBFS -> 229.5.
    struct Case { int16_t amplitude; int expected; };
    const Case cases[]={{3277,170},{328,85},{16384,230}};

    for (const auto& c : cases)
    {
        std::vector<int16_t> pcm(24000,c.amplitude);
        const auto buckets=WaveformExtractor::compute(pcm.data(),pcm.size());
        for (auto b : buckets)
        {
            BOOST_CHECK_LE(std::abs(static_cast<int>(b)-c.expected),1);
        }
    }
}

BOOST_AUTO_TEST_CASE(BelowFloorIsZero)
{
    // -70 dBFS is under the floor: 32768 * 10^(-70/20) = 10.4
    std::vector<int16_t> pcm(24000,10);
    const auto buckets=WaveformExtractor::compute(pcm.data(),pcm.size());
    for (auto b : buckets)
    {
        BOOST_CHECK_EQUAL(static_cast<int>(b),0);
    }
}

BOOST_AUTO_TEST_CASE(LouderIsNeverLower)
{
    int previous=-1;
    for (int amplitude : {5,50,500,2000,8000,20000,32767})
    {
        std::vector<int16_t> pcm(24000,static_cast<int16_t>(amplitude));
        const auto buckets=WaveformExtractor::compute(pcm.data(),pcm.size());
        BOOST_CHECK_GE(static_cast<int>(buckets[0]),previous);
        previous=static_cast<int>(buckets[0]);
    }
}

BOOST_AUTO_TEST_CASE(ChunkingDoesNotMatter)
{
    // Whatever way a platform slices its capture buffers must give the same waveform.
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(-20000,20000);
    std::vector<int16_t> pcm(100003);
    for (auto& s : pcm)
    {
        s=static_cast<int16_t>(dist(rng));
    }

    const auto reference=WaveformExtractor::compute(pcm.data(),pcm.size());

    for (size_t chunk : {size_t{1},size_t{7},size_t{239},size_t{240},size_t{241},size_t{960},size_t{4096}})
    {
        WaveformExtractor extractor;
        for (size_t offset=0;offset<pcm.size();offset+=chunk)
        {
            extractor.add(pcm.data()+offset,std::min(chunk,pcm.size()-offset));
        }
        BOOST_CHECK_EQUAL(extractor.totalFrames(),pcm.size());
        BOOST_CHECK(extractor.buckets()==reference);
    }
}

BOOST_AUTO_TEST_CASE(ShortClipFillsTheWholeBar)
{
    // 10 windows for 100 buckets: each window must be stretched, not leave gaps of zeros.
    std::vector<int16_t> pcm(10*WaveformExtractor::WindowFrames,16384);
    const auto buckets=WaveformExtractor::compute(pcm.data(),pcm.size());
    for (auto b : buckets)
    {
        BOOST_CHECK_GT(static_cast<int>(b),200);
    }
}

BOOST_AUTO_TEST_CASE(LoudThenSilentKeepsItsShape)
{
    // 10 s = 2000 windows, first half loud. Boundaries fall exactly on windows, so the split is clean.
    const size_t half=240000;
    std::vector<int16_t> pcm(2*half,0);
    std::fill(pcm.begin(),pcm.begin()+static_cast<std::ptrdiff_t>(half),int16_t{16384});

    const auto buckets=WaveformExtractor::compute(pcm.data(),pcm.size());
    for (size_t i=0;i<50;i++)
    {
        BOOST_CHECK_GT(static_cast<int>(buckets[i]),200);
    }
    for (size_t i=50;i<100;i++)
    {
        BOOST_CHECK_EQUAL(static_cast<int>(buckets[i]),0);
    }
}

BOOST_AUTO_TEST_CASE(BucketsIsRepeatableMidRecording)
{
    // buckets() must not consume state: a live preview may call it and then keep adding.
    auto pcm=test::makeSine(48000);
    WaveformExtractor extractor;
    extractor.add(pcm.data(),24000);
    const auto first=extractor.buckets();
    BOOST_CHECK(extractor.buckets()==first);
    extractor.add(pcm.data()+24000,24000);
    BOOST_CHECK(extractor.buckets()==WaveformExtractor::compute(pcm.data(),pcm.size()));
}

BOOST_AUTO_TEST_CASE(ResetForgetsEverything)
{
    auto pcm=test::makeSine(48000);
    WaveformExtractor extractor;
    extractor.add(pcm.data(),pcm.size());
    extractor.reset();
    BOOST_CHECK_EQUAL(extractor.totalFrames(),uint64_t{0});
    for (auto b : extractor.buckets())
    {
        BOOST_CHECK_EQUAL(static_cast<int>(b),0);
    }
}

BOOST_AUTO_TEST_CASE(RmsDbLevelMeter)
{
    BOOST_CHECK_EQUAL(WaveformExtractor::rmsDb(nullptr,0),static_cast<float>(WaveformExtractor::FloorDb));

    std::vector<int16_t> silence(480,0);
    BOOST_CHECK_EQUAL(WaveformExtractor::rmsDb(silence.data(),silence.size()),static_cast<float>(WaveformExtractor::FloorDb));

    std::vector<int16_t> full(480,32767);
    BOOST_CHECK_LE(std::abs(WaveformExtractor::rmsDb(full.data(),full.size())),0.01f);

    std::vector<int16_t> minus20(480,3277);
    BOOST_CHECK_LE(std::abs(WaveformExtractor::rmsDb(minus20.data(),minus20.size())+20.0f),0.1f);
}

BOOST_AUTO_TEST_SUITE_END()
