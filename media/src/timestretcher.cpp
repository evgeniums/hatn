/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/timestretcher.cpp
  *
  *  Pitch-preserving time stretching of mono 48 kHz speech (WSOLA).
  *
  */

/****************************************************************************/

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

#include <hatn/media/timestretcher.h>

HATN_MEDIA_NAMESPACE_BEGIN

namespace {

//! Synthesis hop, how far apart the output segments are laid: 10 ms.
constexpr const int64_t HopFrames=VoiceSampleRate/100;

//! Segment length: two hops, so that a periodic Hann window at 50% overlap adds up to exactly one.
constexpr const int64_t WindowFrames=2*HopFrames;

//! How far a segment may move from where the speed puts it: 6 ms either way. A range of 12 ms
//! spans a whole pitch period down to about 83 Hz, so every voice has a phase-aligned position.
constexpr const int64_t SearchFrames=VoiceSampleRate*6/1000;

//! Mean square (in units of full scale) below which the audio counts as silence: about 3 LSB rms.
constexpr const double SilenceMeanSquare=1e-8;

//! Below this a candidate segment is silent and cannot be compared.
constexpr const double MinEnergy=1e-12;

inline int16_t toPcm(float value) noexcept
{
    const long scaled=std::lround(value*32768.0f);
    return static_cast<int16_t>(std::min<long>(32767,std::max<long>(-32768,scaled)));
}

//! Dot product with four independent accumulators, so the compiler is free to pipeline it.
inline float dot(const float* a, const float* b, size_t count) noexcept
{
    float s0=0.0f;
    float s1=0.0f;
    float s2=0.0f;
    float s3=0.0f;
    size_t i=0;
    for (;i+4<=count;i+=4)
    {
        s0+=a[i]*b[i];
        s1+=a[i+1]*b[i+1];
        s2+=a[i+2]*b[i+2];
        s3+=a[i+3]*b[i+3];
    }
    for (;i<count;i++)
    {
        s0+=a[i]*b[i];
    }
    return (s0+s1)+(s2+s3);
}

}

/********************** TimeStretcher_p **************************/

/**
 * Notation. The input is a stream of samples with ABSOLUTE indices counted from the last reset();
 * `in` holds only the part that is still needed, starting at index `inBase`. Segment k is the
 * WindowFrames samples of input starting at index tau_k, and it is laid down at output position
 * k * HopFrames. Its first half is cross-faded with the second half of segment k-1, which is
 * the raw input after tau_{k-1}+HopFrames, so tau_k is chosen to make the first half of segment k
 * as similar as possible to exactly that "natural continuation" of the previous segment.
 *
 * tau_k is searched around `nominal`, the position that the speed asks for: nominal advances by
 * speed*HopFrames per segment and NOT by the position that was actually chosen, so the search never
 * drifts and output frame j always corresponds to input position about j*speed.
 *
 * The first segment is used as it is, without fading in; and at the end of the stream the input is
 * treated as silence and the output is cut to exactly round(inputFrames/speed) frames.
 */
class TimeStretcher_p
{
    public:

        TimeStretcher_p()
            : window(static_cast<size_t>(WindowFrames)),
              block(static_cast<size_t>(HopFrames)),
              ref(static_cast<size_t>(HopFrames))
        {
            const double twoPi=6.283185307179586;
            for (size_t i=0;i<window.size();i++)
            {
                window[i]=static_cast<float>(0.5-0.5*std::cos(twoPi*static_cast<double>(i)/static_cast<double>(WindowFrames)));
            }
        }

        void reset(float value)
        {
            if (std::isnan(value))
            {
                value=1.0f;
            }
            speed=std::min(std::max(value,MinPlaybackSpeed),MaxPlaybackSpeed);
            analysisHop=static_cast<double>(speed)*static_cast<double>(HopFrames);

            in.clear();
            inBase=0;
            out.clear();
            outPos=0;
            nominal=0.0;
            prevTau=0;
            first=true;
            eof=false;
            done=false;
            emitted=0;
        }

        void push(const int16_t* pcm, size_t frames)
        {
            if (eof || pcm==nullptr || frames==0)
            {
                return;
            }
            const float scale=1.0f/32768.0f;
            const auto old=in.size();
            in.resize(old+frames);
            for (size_t i=0;i<frames;i++)
            {
                in[old+i]=static_cast<float>(pcm[i])*scale;
            }
        }

        size_t pull(int16_t* dst, size_t maxFrames)
        {
            if (dst==nullptr)
            {
                return 0;
            }

            size_t produced=0;
            while (produced<maxFrames)
            {
                if (outPos<out.size())
                {
                    const auto n=std::min(maxFrames-produced,out.size()-outPos);
                    std::memcpy(dst+produced,out.data()+outPos,n*sizeof(int16_t));
                    outPos+=n;
                    produced+=n;
                    continue;
                }

                out.clear();
                outPos=0;
                if (!step())
                {
                    break;
                }
            }
            return produced;
        }

        bool finished() const noexcept
        {
            return done && outPos>=out.size();
        }

        float speed=1.0f;
        bool eof=false;

    private:

        //! Absolute index one past the last input sample received.
        int64_t inputEnd() const noexcept
        {
            return inBase+static_cast<int64_t>(in.size());
        }

        //! Input sample at an absolute index; silence outside what is held.
        float at(int64_t index) const noexcept
        {
            if (index<inBase || index>=inputEnd())
            {
                return 0.0f;
            }
            return in[static_cast<size_t>(index-inBase)];
        }

        //! Total output length once the input has ended.
        int64_t outputLimit() const noexcept
        {
            return static_cast<int64_t>(std::llround(static_cast<double>(inputEnd())/static_cast<double>(speed)));
        }

        /**
         * Produce one hop of output into `out`. False when more input is needed or the stream is
         * done; then nothing was produced.
         */
        bool step()
        {
            if (done)
            {
                return false;
            }

            const auto position=static_cast<int64_t>(std::llround(nominal));

            if (eof)
            {
                if (nominal>=static_cast<double>(inputEnd()) || emitted>=outputLimit())
                {
                    done=true;
                    return false;
                }
            }
            else if (inputEnd()<position+SearchFrames+WindowFrames)
            {
                // the search may need input up to position+SearchFrames, and the segment WindowFrames more
                return false;
            }

            int64_t tau=position;
            if (first)
            {
                for (int64_t n=0;n<HopFrames;n++)
                {
                    block[static_cast<size_t>(n)]=at(tau+n);
                }
            }
            else
            {
                tau=findBestPosition(position);
                const auto tail=prevTau+HopFrames;
                for (int64_t n=0;n<HopFrames;n++)
                {
                    const auto i=static_cast<size_t>(n);
                    block[i]=at(tail+n)*window[static_cast<size_t>(HopFrames)+i]+at(tau+n)*window[i];
                }
            }

            int64_t count=HopFrames;
            if (eof)
            {
                // emitted<outputLimit() was checked above
                count=std::min(count,outputLimit()-emitted);
            }
            for (int64_t i=0;i<count;i++)
            {
                out.push_back(toPcm(block[static_cast<size_t>(i)]));
            }
            emitted+=count;

            prevTau=tau;
            first=false;
            nominal+=analysisHop;

            // Drop the input that no later step reads: from the second half of the segment just
            // used (the next reference) and from the lowest position the next search can reach.
            const auto next=static_cast<int64_t>(std::llround(nominal));
            const auto keepFrom=std::min(prevTau+HopFrames,std::max<int64_t>(0,next-SearchFrames));
            if (keepFrom>inBase)
            {
                const auto drop=std::min<int64_t>(keepFrom-inBase,static_cast<int64_t>(in.size()));
                in.erase(in.begin(),in.begin()+static_cast<std::ptrdiff_t>(drop));
                inBase+=drop;
            }
            return true;
        }

        /**
         * The start of the next segment: within SearchFrames of `position`, the one whose first half
         * best matches the natural continuation of the previous segment. Ties go to the position
         * closest to `position`, so silence and perfectly periodic input do not wander.
         */
        int64_t findBestPosition(int64_t position)
        {
            // position>=0 and everything from max(0,position-SearchFrames) on is still held
            const int64_t lo=std::max<int64_t>(position-SearchFrames,std::max<int64_t>(0,inBase));
            const int64_t hi=position+SearchFrames;

            const auto refStart=prevTau+HopFrames;
            double refEnergy=0.0;
            for (int64_t n=0;n<HopFrames;n++)
            {
                const auto value=at(refStart+n);
                ref[static_cast<size_t>(n)]=value;
                refEnergy+=static_cast<double>(value)*static_cast<double>(value);
            }
            if (refEnergy<SilenceMeanSquare*static_cast<double>(HopFrames))
            {
                return position;
            }

            // the input from lo to hi+HopFrames, and running sums of squares to get the energy of
            // every candidate in O(1)
            const auto spanLength=static_cast<size_t>(hi-lo+HopFrames);
            span.resize(spanLength);
            prefix.resize(spanLength+1);
            prefix[0]=0.0;
            for (size_t i=0;i<spanLength;i++)
            {
                const auto value=at(lo+static_cast<int64_t>(i));
                span[i]=value;
                prefix[i+1]=prefix[i]+static_cast<double>(value)*static_cast<double>(value);
            }

            int64_t best=position;
            double bestScore=-std::numeric_limits<double>::max();
            const int64_t reach=std::max(hi-position,position-lo);
            for (int64_t distance=0;distance<=reach;distance++)
            {
                for (int side=0;side<2;side++)
                {
                    if (distance==0 && side==1)
                    {
                        break;
                    }
                    const int64_t candidate=(side==0)?position+distance:position-distance;
                    if (candidate<lo || candidate>hi)
                    {
                        continue;
                    }

                    const auto offset=static_cast<size_t>(candidate-lo);
                    const double energy=prefix[offset+static_cast<size_t>(HopFrames)]-prefix[offset];
                    double score=0.0;
                    if (energy>MinEnergy)
                    {
                        // normalised by the candidate's energy so that a loud segment does not win
                        // just for being loud; the reference's energy is the same for all candidates
                        score=static_cast<double>(dot(ref.data(),span.data()+offset,static_cast<size_t>(HopFrames)))/std::sqrt(energy);
                    }
                    if (score>bestScore)
                    {
                        bestScore=score;
                        best=candidate;
                    }
                }
            }
            return best;
        }

        std::vector<float> window;      //!< periodic Hann, WindowFrames long
        std::vector<float> in;          //!< input from absolute index inBase on, scaled to [-1,1)
        int64_t inBase=0;
        std::vector<int16_t> out;       //!< output of the last step, consumed from outPos
        size_t outPos=0;

        double nominal=0.0;             //!< where the next segment starts, before the search
        double analysisHop=static_cast<double>(HopFrames);
        int64_t prevTau=0;              //!< where the previous segment started
        bool first=true;
        bool done=false;
        int64_t emitted=0;              //!< output frames produced so far

        // scratch, kept to avoid an allocation per step
        std::vector<float> block;
        std::vector<float> ref;
        std::vector<float> span;
        std::vector<double> prefix;
};

/********************** TimeStretcher **************************/

//---------------------------------------------------------------
TimeStretcher::TimeStretcher()
    : d(std::make_unique<TimeStretcher_p>())
{}

//---------------------------------------------------------------
TimeStretcher::~TimeStretcher()=default;

//---------------------------------------------------------------
void TimeStretcher::reset(float speed)
{
    d->reset(speed);
}

//---------------------------------------------------------------
float TimeStretcher::speed() const noexcept
{
    return d->speed;
}

//---------------------------------------------------------------
void TimeStretcher::push(const int16_t* pcm, size_t frames)
{
    d->push(pcm,frames);
}

//---------------------------------------------------------------
void TimeStretcher::finish()
{
    d->eof=true;
}

//---------------------------------------------------------------
size_t TimeStretcher::pull(int16_t* out, size_t maxFrames)
{
    return d->pull(out,maxFrames);
}

//---------------------------------------------------------------
bool TimeStretcher::finished() const noexcept
{
    return d->finished();
}

//---------------------------------------------------------------

HATN_MEDIA_NAMESPACE_END
