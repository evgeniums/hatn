/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/waveformextractor.cpp
  *
  *      Contains implementation of WaveformExtractor.
  *
  */

#include <cmath>
#include <algorithm>

#include <hatn/media/waveformextractor.h>

HATN_MEDIA_NAMESPACE_BEGIN

namespace {

constexpr double FullScale=32768.0;

}

/********************** WaveformExtractor **************************/

//---------------------------------------------------------------
void WaveformExtractor::reset()
{
    m_windows.clear();
    m_partialSumSq=0;
    m_partialFrames=0;
    m_totalFrames=0;
}

//---------------------------------------------------------------
void WaveformExtractor::add(const int16_t* samples, size_t frames)
{
    for (size_t i=0;i<frames;i++)
    {
        // exact integer accumulation: |sample|^2 <= 2^30 and a window holds 240 of them, so this
        // cannot overflow uint64_t, and the result does not depend on how add() calls are chunked
        const auto s=static_cast<int64_t>(samples[i]);
        m_partialSumSq+=static_cast<uint64_t>(s*s);
        m_partialFrames++;
        if (m_partialFrames==WindowFrames)
        {
            m_windows.push_back(static_cast<float>(static_cast<double>(m_partialSumSq)/static_cast<double>(WindowFrames)));
            m_partialSumSq=0;
            m_partialFrames=0;
        }
    }
    m_totalFrames+=frames;
}

//---------------------------------------------------------------
uint8_t WaveformExtractor::meanSquareToByte(double meanSquare) noexcept
{
    if (!(meanSquare>0.0))
    {
        return 0;
    }

    const auto rms=std::sqrt(meanSquare)/FullScale;
    const auto db=20.0*std::log10(rms);
    if (db<=static_cast<double>(FloorDb))
    {
        return 0;
    }
    if (db>=0.0)
    {
        return 255;
    }

    const auto scaled=(db-static_cast<double>(FloorDb))/(-static_cast<double>(FloorDb))*255.0;
    return static_cast<uint8_t>(std::lround(scaled));
}

//---------------------------------------------------------------
std::vector<uint8_t> WaveformExtractor::buckets() const
{
    std::vector<uint8_t> result(WaveformBuckets,0);

    // completed windows plus, if any, the trailing partial one as a window of its own
    const auto windowCount=m_windows.size()+(m_partialFrames!=0?1:0);
    if (windowCount==0)
    {
        return result;
    }

    const auto windowValue=[this](size_t index)
    {
        if (index<m_windows.size())
        {
            return static_cast<double>(m_windows[index]);
        }
        return static_cast<double>(m_partialSumSq)/static_cast<double>(m_partialFrames);
    };

    for (size_t i=0;i<WaveformBuckets;i++)
    {
        // Integer partition of [0,windowCount) into WaveformBuckets stretches. With fewer windows
        // than buckets a stretch would be empty, so it is widened to the one window it falls on,
        // which stretches a very short recording across the whole bar instead of leaving gaps.
        const auto begin=static_cast<size_t>((static_cast<uint64_t>(i)*windowCount)/WaveformBuckets);
        auto end=static_cast<size_t>((static_cast<uint64_t>(i+1)*windowCount)/WaveformBuckets);
        if (end<=begin)
        {
            end=begin+1;
        }

        double sum=0.0;
        for (size_t j=begin;j<end;j++)
        {
            sum+=windowValue(j);
        }
        result[i]=meanSquareToByte(sum/static_cast<double>(end-begin));
    }

    return result;
}

//---------------------------------------------------------------
std::vector<uint8_t> WaveformExtractor::compute(const int16_t* samples, size_t frames)
{
    WaveformExtractor extractor;
    extractor.add(samples,frames);
    return extractor.buckets();
}

//---------------------------------------------------------------
float WaveformExtractor::rmsDb(const int16_t* samples, size_t frames) noexcept
{
    if (frames==0)
    {
        return static_cast<float>(FloorDb);
    }

    uint64_t sumSq=0;
    for (size_t i=0;i<frames;i++)
    {
        const auto s=static_cast<int64_t>(samples[i]);
        sumSq+=static_cast<uint64_t>(s*s);
    }
    if (sumSq==0)
    {
        return static_cast<float>(FloorDb);
    }

    const auto rms=std::sqrt(static_cast<double>(sumSq)/static_cast<double>(frames))/FullScale;
    const auto db=20.0*std::log10(rms);
    return static_cast<float>(std::clamp(db,static_cast<double>(FloorDb),0.0));
}

//---------------------------------------------------------------

HATN_MEDIA_NAMESPACE_END
