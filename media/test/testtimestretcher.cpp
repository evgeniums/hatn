/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/test/testtimestretcher.cpp
  *
  *  Tests of TimeStretcher. It needs no codec, so all of it runs in every build of the library.
  *
  *  What is checked is what can be measured: the length of the output, the pitch of a tone and of
  *  a voiced signal (a time stretch that only resampled would fail these), the level, and that the
  *  result depends on nothing but the input. Whether it SOUNDS good at 2x is not something a test
  *  can say.
  *
  */

/****************************************************************************/

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <hatn/media/timestretcher.h>

#include "testmediautils.h"

HATN_MEDIA_USING

BOOST_AUTO_TEST_SUITE(TestTimeStretcher)

namespace {

//! Feed `input` in pieces of `pushChunk`, taking output in pieces of `pullChunk` as it becomes
//! available, then finish and take the rest. The stretcher must already have been reset.
std::vector<int16_t> stretchWith(TimeStretcher& stretcher, const std::vector<int16_t>& input, size_t pushChunk, size_t pullChunk)
{
    std::vector<int16_t> result;
    std::vector<int16_t> buffer(pullChunk);

    auto drain=[&]()
    {
        for (;;)
        {
            const auto n=stretcher.pull(buffer.data(),buffer.size());
            if (n==0)
            {
                break;
            }
            result.insert(result.end(),buffer.begin(),buffer.begin()+static_cast<std::ptrdiff_t>(n));
        }
    };

    for (size_t offset=0;offset<input.size();offset+=pushChunk)
    {
        const auto n=std::min(pushChunk,input.size()-offset);
        stretcher.push(input.data()+offset,n);
        drain();
    }
    stretcher.finish();
    drain();
    BOOST_CHECK(stretcher.finished());
    return result;
}

std::vector<int16_t> stretchAll(const std::vector<int16_t>& input, float speed, size_t pushChunk=960, size_t pullChunk=480)
{
    TimeStretcher stretcher;
    stretcher.reset(speed);
    return stretchWith(stretcher,input,pushChunk,pullChunk);
}

//! The output length the class promises.
size_t expectedLength(size_t inputFrames, float speed)
{
    return static_cast<size_t>(std::llround(static_cast<double>(inputFrames)/static_cast<double>(speed)));
}

//! Something voiced: the first eight harmonics of `f0` with the slow amplitude swell of syllables.
std::vector<int16_t> makeVoiced(size_t frames, double f0)
{
    std::vector<int16_t> result(frames);
    const double twoPi=6.283185307179586;
    for (size_t i=0;i<frames;i++)
    {
        const double t=static_cast<double>(i)/static_cast<double>(VoiceSampleRate);
        double sample=0.0;
        for (int k=1;k<=8;k++)
        {
            sample+=std::sin(twoPi*f0*static_cast<double>(k)*t)/static_cast<double>(k);
        }
        const double envelope=0.65+0.35*std::sin(twoPi*3.0*t);
        result[i]=static_cast<int16_t>(std::lround(6000.0*envelope*sample));
    }
    return result;
}

//! Deterministic white noise. It has no periodicity, so a segment matches only itself.
std::vector<int16_t> makeNoise(size_t frames)
{
    std::vector<int16_t> result(frames);
    uint32_t state=12345u;
    for (auto& sample : result)
    {
        state=state*1664525u+1013904223u;
        sample=static_cast<int16_t>(static_cast<int32_t>(state>>16)%16001-8000);
    }
    return result;
}

/**
 * Period of the fundamental, in frames, from the normalised autocorrelation of `windowFrames`
 * frames: the first local maximum that is nearly as high as the highest, so that a multiple of the
 * period is not taken for it. `pcm` must hold windowFrames+maxLag frames. 0 if there is none.
 */
size_t estimatePeriod(const int16_t* pcm, size_t windowFrames, size_t minLag, size_t maxLag)
{
    std::vector<double> correlationAt(maxLag+2,0.0);
    for (size_t lag=minLag;lag<=maxLag;lag++)
    {
        double ab=0.0;
        double aa=0.0;
        double bb=0.0;
        for (size_t i=0;i<windowFrames;i++)
        {
            const double a=pcm[i];
            const double b=pcm[i+lag];
            ab+=a*b;
            aa+=a*a;
            bb+=b*b;
        }
        correlationAt[lag]=(aa>0.0 && bb>0.0)?ab/std::sqrt(aa*bb):0.0;
    }

    double best=0.0;
    for (size_t lag=minLag;lag<=maxLag;lag++)
    {
        best=std::max(best,correlationAt[lag]);
    }
    for (size_t lag=minLag+1;lag<maxLag;lag++)
    {
        if (correlationAt[lag]>=0.9*best
            && correlationAt[lag]>=correlationAt[lag-1]
            && correlationAt[lag]>=correlationAt[lag+1])
        {
            return lag;
        }
    }
    return 0;
}

}

BOOST_AUTO_TEST_CASE(NewStretcherHasNothingToGive)
{
    TimeStretcher stretcher;
    BOOST_CHECK_EQUAL(stretcher.speed(),1.0f);

    int16_t out[16];
    BOOST_CHECK_EQUAL(stretcher.pull(out,16),size_t{0});
    BOOST_CHECK(!stretcher.finished());
    BOOST_CHECK_EQUAL(stretcher.pull(nullptr,16),size_t{0});

    // an empty stream simply finishes
    stretcher.finish();
    BOOST_CHECK_EQUAL(stretcher.pull(out,16),size_t{0});
    BOOST_CHECK(stretcher.finished());
}

BOOST_AUTO_TEST_CASE(SpeedIsClamped)
{
    TimeStretcher stretcher;

    stretcher.reset(3.0f);
    BOOST_CHECK_EQUAL(stretcher.speed(),MaxPlaybackSpeed);
    stretcher.reset(0.1f);
    BOOST_CHECK_EQUAL(stretcher.speed(),MinPlaybackSpeed);
    stretcher.reset(-2.0f);
    BOOST_CHECK_EQUAL(stretcher.speed(),MinPlaybackSpeed);
    stretcher.reset(std::numeric_limits<float>::quiet_NaN());
    BOOST_CHECK_EQUAL(stretcher.speed(),1.0f);
    stretcher.reset(1.25f);
    BOOST_CHECK_EQUAL(stretcher.speed(),1.25f);
}

BOOST_AUTO_TEST_CASE(NothingComesOutBeforeEnoughInput)
{
    // about 26 ms of input are needed for the first output (see the class comment)
    const auto pcm=test::makeSine(4000);
    TimeStretcher stretcher;
    stretcher.reset(1.5f);

    int16_t out[480];
    stretcher.push(pcm.data(),1200);
    BOOST_CHECK_EQUAL(stretcher.pull(out,480),size_t{0});
    BOOST_CHECK(!stretcher.finished());

    stretcher.push(pcm.data()+1200,2800);
    BOOST_CHECK_GT(stretcher.pull(out,480),size_t{0});
}

BOOST_AUTO_TEST_CASE(OutputLengthFollowsTheSpeed)
{
    for (const auto speed : {0.5f,0.75f,1.0f,1.25f,1.5f,2.0f})
    {
        for (const size_t frames : {size_t{47999},size_t{48000}})
        {
            const auto pcm=test::makeSine(frames,220.0);
            const auto out=stretchAll(pcm,speed);
            const auto expected=expectedLength(frames,speed);
            BOOST_CHECK_MESSAGE(out.size()==expected,
                                "speed " << speed << ", " << frames << " frames in: got " << out.size()
                                << " frames out, expected " << expected);
        }
    }
}

BOOST_AUTO_TEST_CASE(ShortInputsGiveTheRightLength)
{
    // shorter than one segment, around the size of the first hop and of the start-up latency
    for (const auto speed : {0.5f,1.0f,2.0f})
    {
        for (const size_t frames : {size_t{0},size_t{1},size_t{100},size_t{479},size_t{480},size_t{481},
                                    size_t{1247},size_t{1248},size_t{1249},size_t{2000}})
        {
            const auto pcm=test::makeSine(frames,300.0);
            const auto out=stretchAll(pcm,speed,300,480);
            const auto expected=expectedLength(frames,speed);
            BOOST_CHECK_MESSAGE(out.size()==expected,
                                "speed " << speed << ", " << frames << " frames in: got " << out.size()
                                << " frames out, expected " << expected);
        }
    }
}

BOOST_AUTO_TEST_CASE(SpeedOneReproducesTheInputExactly)
{
    // At speed 1 the best position of every segment is the one that simply continues the input, and
    // the two halves of the cross-fade then add up to the input itself. Noise has no other position
    // that fits, so this checks the whole chain: the input buffer, the indices, the window, the
    // overlap-add and the cut at the end.
    const auto pcm=makeNoise(20000);
    const auto out=stretchAll(pcm,1.0f);
    BOOST_REQUIRE_EQUAL(out.size(),pcm.size());

    size_t firstDifference=out.size();
    for (size_t i=0;i<out.size();i++)
    {
        if (out[i]!=pcm[i])
        {
            firstDifference=i;
            break;
        }
    }
    BOOST_CHECK_EQUAL(firstDifference,out.size());
}

BOOST_AUTO_TEST_CASE(PitchOfATonePreserved)
{
    // Playing a tone faster by resampling would raise its frequency by the same factor
    for (const double frequency : {150.0,600.0})
    {
        for (const auto speed : {0.5f,1.5f,2.0f})
        {
            const auto pcm=test::makeSine(48000,frequency);
            const auto out=stretchAll(pcm,speed);
            BOOST_REQUIRE_GT(out.size(),size_t{4000});

            const auto begin=out.size()/4;
            const auto window=out.size()/2;
            const auto measured=test::toneFrequency(out.data()+begin,window);
            BOOST_CHECK_MESSAGE(std::fabs(measured-frequency)<0.03*frequency,
                                "tone of " << frequency << " Hz at speed " << speed << " came out as " << measured << " Hz");

            const auto level=test::rmsLevel(out.data()+begin,window)/test::rmsLevel(pcm.data()+pcm.size()/4,pcm.size()/2);
            BOOST_CHECK_MESSAGE(level>0.9 && level<1.1,
                                "tone of " << frequency << " Hz at speed " << speed << " changed its level by a factor " << level);
        }
    }
}

BOOST_AUTO_TEST_CASE(PitchOfAVoicedSignalPreserved)
{
    // harmonics with a syllable-like swell, fundamental 120 Hz = a period of 400 frames
    const auto pcm=makeVoiced(96000,120.0);
    for (const auto speed : {0.5f,0.75f,1.5f,2.0f})
    {
        const auto out=stretchAll(pcm,speed);
        BOOST_REQUIRE_GT(out.size(),size_t{20000});

        const auto period=estimatePeriod(out.data()+out.size()/3,4096,96,900);
        BOOST_CHECK_MESSAGE(period>=384 && period<=416,
                            "voiced signal at speed " << speed << " has a period of " << period << " frames, expected about 400");
    }
}

BOOST_AUTO_TEST_CASE(SilenceStaysSilent)
{
    const std::vector<int16_t> silence(30000,0);
    for (const auto speed : {0.5f,1.5f})
    {
        const auto out=stretchAll(silence,speed);
        BOOST_CHECK_EQUAL(out.size(),expectedLength(silence.size(),speed));
        BOOST_CHECK(std::all_of(out.begin(),out.end(),[](int16_t sample){return sample==0;}));
    }
}

BOOST_AUTO_TEST_CASE(ResultDoesNotDependOnHowItIsCut)
{
    // The output is a function of the input alone: how the input is split into push() calls and how
    // the output is taken with pull() must not change a single sample.
    const auto pcm=makeVoiced(48000,120.0);
    const auto reference=stretchAll(pcm,1.5f,960,480);
    BOOST_REQUIRE_EQUAL(reference.size(),expectedLength(pcm.size(),1.5f));

    const size_t cuts[][2]={{1,480},{37,1},{5000,4096},{48000,480},{960,7}};
    for (const auto& cut : cuts)
    {
        const auto out=stretchAll(pcm,1.5f,cut[0],cut[1]);
        BOOST_CHECK_MESSAGE(out==reference,
                            "pushing " << cut[0] << " and pulling " << cut[1] << " frames at a time gives a different result");
    }
}

BOOST_AUTO_TEST_CASE(ResetStartsOver)
{
    const auto pcm=makeVoiced(30000,120.0);
    TimeStretcher stretcher;

    stretcher.reset(1.5f);
    const auto first=stretchWith(stretcher,pcm,960,480);
    stretcher.reset(1.5f);
    const auto second=stretchWith(stretcher,pcm,960,480);
    BOOST_CHECK(first==second);

    // and a different speed on the same object
    stretcher.reset(0.5f);
    const auto slower=stretchWith(stretcher,pcm,960,480);
    BOOST_CHECK_EQUAL(slower.size(),expectedLength(pcm.size(),0.5f));
}

BOOST_AUTO_TEST_CASE(InputAfterFinishIsIgnored)
{
    const auto pcm=makeVoiced(20000,120.0);
    TimeStretcher stretcher;
    stretcher.reset(2.0f);

    stretcher.push(pcm.data(),10000);
    stretcher.finish();
    stretcher.push(pcm.data()+10000,10000);

    std::vector<int16_t> buffer(480);
    size_t total=0;
    for (;;)
    {
        const auto n=stretcher.pull(buffer.data(),buffer.size());
        if (n==0)
        {
            break;
        }
        total+=n;
    }
    BOOST_CHECK(stretcher.finished());
    BOOST_CHECK_EQUAL(total,expectedLength(10000,2.0f));
}

BOOST_AUTO_TEST_SUITE_END()
