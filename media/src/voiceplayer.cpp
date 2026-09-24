/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/voiceplayer.cpp
  *
  *      Contains implementation of VoicePlayer.
  *
  */

#include <atomic>
#include <algorithm>
#include <array>
#include <cmath>

#include <hatn/media/mediaerror.h>
#include <hatn/media/pcmring.h>
#include <hatn/media/oggopusreader.h>
#include <hatn/media/voiceplayer.h>

HATN_MEDIA_NAMESPACE_BEGIN

/********************** VoicePlayer **************************/

class VoicePlayer_p
{
    public:

        explicit VoicePlayer_p(uint32_t bufferMs)
            : ring(static_cast<size_t>(voiceMsToFrames(std::max<uint32_t>(bufferMs,100))))
        {}

        PcmRing ring;
        OggOpusReader reader;   // touched by the decode thread only, and by open()/close()

        std::atomic<bool> opened{false};
        std::atomic<bool> playing{false};
        std::atomic<bool> decoderFinished{false};
        std::atomic<bool> ended{false};

        std::atomic<uint64_t> totalFrames{0};

        //! Frame to seek to, or -1. Written by the control thread, taken by the decode thread.
        std::atomic<int64_t> seekRequest{-1};

        // A seek is announced to the audio thread as (marker, target) published by bumping the
        // epoch. `marker` is the ring's write index at that moment: everything before it is
        // audio from before the seek and is dropped; everything from it on is fresh. That keeps
        // the read side of the ring owned by the audio thread alone.
        std::atomic<uint32_t> seekEpoch{0};
        std::atomic<uint64_t> seekMarker{0};
        std::atomic<uint64_t> seekTarget{0};
        uint32_t seenEpoch=0;   // audio thread only

        //! Frame of the message that the listener has reached: time of the message, not of the
        //! listener (see epochSpeed).
        std::atomic<uint64_t> playedFrames{0};

        // ---- speed ------------------------------------------------------------------------
        //
        // The audio in the ring was made at one speed since the last EPOCH: the start, a seek or a
        // change of speed. An epoch begins at a frame of the message (epochStart) and at a write
        // index of the ring (epochMarker), so ring index i of the epoch holds the message frame
        // epochStart+(i-epochMarker)*speed. That is all the position bookkeeping there is; a speed
        // of 1 makes it the plain frame count it was before speed existed.

        TimeStretcher stretcher;                    // decode thread only, and open()

        std::atomic<float> speed{1.0f};             // as requested; the control thread writes it
        std::atomic<float> appliedSpeed{1.0f};      // of the audio in the ring; the decode thread writes it
        std::atomic<float> seekSpeed{1.0f};         // published with seekEpoch, like seekTarget

        uint64_t epochMarker=0;                     // decode thread only
        uint64_t epochStart=0;                      // decode thread only

        uint64_t epochBase=0;                       // audio thread only: message frame at the epoch's start
        uint64_t epochPulled=0;                     // audio thread only: frames pulled since then
        double epochSpeed=1.0;                      // audio thread only

        /**
         * Decode thread: the frame of the message the listener is at, worked out from the producer
         * side of the ring alone. What the audio thread has consumed is still below epochMarker while
         * it has not yet noticed the newest epoch, and then the listener is at the epoch's start.
         */
        uint64_t listenerFrame() const noexcept
        {
            const uint64_t written=ring.writeIndex();
            const uint64_t consumed=written-(ring.capacity()-ring.writable());

            uint64_t frame=epochStart;
            if (consumed>epochMarker)
            {
                const auto advanced=std::llround(static_cast<double>(consumed-epochMarker)
                                                 *static_cast<double>(appliedSpeed.load(std::memory_order_relaxed)));
                frame+=static_cast<uint64_t>(advanced);
            }
            return std::min<uint64_t>(frame,totalFrames.load(std::memory_order_relaxed));
        }
};

//---------------------------------------------------------------
VoicePlayer::VoicePlayer(uint32_t bufferMs)
    : d(std::make_unique<VoicePlayer_p>(bufferMs))
{}

//---------------------------------------------------------------
VoicePlayer::~VoicePlayer()=default;

//---------------------------------------------------------------
Error VoicePlayer::open(common::File& file)
{
    if (d->opened.load(std::memory_order_acquire))
    {
        return mediaError(MediaError::INVALID_STATE);
    }

    auto ec=d->reader.open(file);
    if (ec)
    {
        return ec;
    }

    // No other thread runs during open(), so this thread may act as both ends of the ring.
    d->ring.discardUpTo(d->ring.writeIndex());
    d->seekRequest.store(-1,std::memory_order_relaxed);
    d->seenEpoch=d->seekEpoch.load(std::memory_order_relaxed);
    d->playedFrames.store(0,std::memory_order_relaxed);
    d->totalFrames.store(d->reader.totalFrames(),std::memory_order_relaxed);
    d->decoderFinished.store(false,std::memory_order_relaxed);
    d->ended.store(false,std::memory_order_relaxed);
    d->playing.store(false,std::memory_order_relaxed);

    // The speed asked for so far is the speed of the first epoch, so nothing has to be re-applied.
    const auto initialSpeed=d->speed.load(std::memory_order_relaxed);
    d->stretcher.reset(initialSpeed);
    d->appliedSpeed.store(initialSpeed,std::memory_order_relaxed);
    d->seekSpeed.store(initialSpeed,std::memory_order_relaxed);
    d->epochMarker=d->ring.writeIndex();
    d->epochStart=0;
    d->epochBase=0;
    d->epochPulled=0;
    d->epochSpeed=static_cast<double>(initialSpeed);

    d->opened.store(true,std::memory_order_release);
    return OK;
}

//---------------------------------------------------------------
void VoicePlayer::close()
{
    d->opened.store(false,std::memory_order_release);
    d->playing.store(false,std::memory_order_relaxed);
    d->reader.close();
    d->ring.discardUpTo(d->ring.writeIndex());
    d->totalFrames.store(0,std::memory_order_relaxed);
    d->playedFrames.store(0,std::memory_order_relaxed);
    d->ended.store(false,std::memory_order_relaxed);
}

//---------------------------------------------------------------
void VoicePlayer::play() noexcept
{
    if (!d->opened.load(std::memory_order_acquire))
    {
        return;
    }
    // playing an ended message starts it over
    if (d->ended.load(std::memory_order_acquire))
    {
        d->seekRequest.store(0,std::memory_order_release);
        d->ended.store(false,std::memory_order_release);
    }
    d->playing.store(true,std::memory_order_release);
}

//---------------------------------------------------------------
void VoicePlayer::pause() noexcept
{
    d->playing.store(false,std::memory_order_release);
}

//---------------------------------------------------------------
void VoicePlayer::seekMs(uint64_t ms) noexcept
{
    if (!d->opened.load(std::memory_order_acquire))
    {
        return;
    }
    const auto frame=std::min<uint64_t>(voiceMsToFrames(ms),d->totalFrames.load(std::memory_order_relaxed));
    d->seekRequest.store(static_cast<int64_t>(frame),std::memory_order_release);
    d->ended.store(false,std::memory_order_release);
}

//---------------------------------------------------------------
void VoicePlayer::setSpeed(float speed) noexcept
{
    if (std::isnan(speed))
    {
        return;
    }
    d->speed.store(std::min(std::max(speed,MinPlaybackSpeed),MaxPlaybackSpeed),std::memory_order_release);
}

//---------------------------------------------------------------
float VoicePlayer::speed() const noexcept
{
    return d->speed.load(std::memory_order_acquire);
}

//---------------------------------------------------------------
Error VoicePlayer::fill()
{
    if (!d->opened.load(std::memory_order_acquire))
    {
        return OK;
    }

    auto request=d->seekRequest.exchange(-1,std::memory_order_acq_rel);

    // A change of speed is a seek to where the listener is: the buffered audio was made at the old
    // speed and would otherwise have to play out first. An ended player has nothing to re-time; its
    // next play() seeks to the start and picks the new speed up then.
    const auto wanted=d->speed.load(std::memory_order_acquire);
    if (request<0 && wanted!=d->appliedSpeed.load(std::memory_order_relaxed))
    {
        if (d->ended.load(std::memory_order_acquire))
        {
            d->stretcher.reset(wanted);
            d->appliedSpeed.store(wanted,std::memory_order_release);
        }
        else
        {
            request=static_cast<int64_t>(d->listenerFrame());
        }
    }

    if (request>=0)
    {
        auto ec=d->reader.seek(static_cast<uint64_t>(request));
        if (ec)
        {
            return ec;
        }

        d->stretcher.reset(wanted);
        d->appliedSpeed.store(wanted,std::memory_order_release);
        d->epochMarker=d->ring.writeIndex();
        d->epochStart=d->reader.position();

        // Order matters: forget "finished" first, so the audio thread cannot see the new epoch
        // together with a stale end-of-stream, then publish the marker, target and speed, then the
        // epoch.
        d->decoderFinished.store(false,std::memory_order_release);
        d->seekMarker.store(d->epochMarker,std::memory_order_relaxed);
        d->seekTarget.store(d->epochStart,std::memory_order_relaxed);
        d->seekSpeed.store(wanted,std::memory_order_relaxed);
        d->seekEpoch.fetch_add(1,std::memory_order_release);
    }

    if (d->decoderFinished.load(std::memory_order_acquire))
    {
        return OK;
    }

    std::array<int16_t,VoiceFrameSamples> buffer;

    if (d->appliedSpeed.load(std::memory_order_relaxed)==1.0f)
    {
        // normal speed: the decoder's output goes to the ring untouched
        while (d->ring.writable()>=buffer.size())
        {
            size_t got=0;
            auto ec=d->reader.read(buffer.data(),buffer.size(),got);
            if (ec)
            {
                return ec;
            }
            if (got==0)
            {
                d->decoderFinished.store(true,std::memory_order_release);
                break;
            }
            d->ring.write(buffer.data(),got);
        }
        return OK;
    }

    // Other speeds: the decoder feeds the stretcher, and the stretcher feeds the ring. The stretcher
    // holds back about 26 ms of audio, and after the decoder's end it has a tail left to give out
    // before the ring may be marked finished.
    std::array<int16_t,VoiceFrameSamples> stretched;
    while (d->ring.writable()>=stretched.size())
    {
        const auto produced=d->stretcher.pull(stretched.data(),stretched.size());
        if (produced!=0)
        {
            d->ring.write(stretched.data(),produced);
            continue;
        }
        if (d->stretcher.finished())
        {
            d->decoderFinished.store(true,std::memory_order_release);
            break;
        }

        // it wants more input
        size_t got=0;
        auto ec=d->reader.read(buffer.data(),buffer.size(),got);
        if (ec)
        {
            return ec;
        }
        if (got==0)
        {
            d->stretcher.finish();
        }
        else
        {
            d->stretcher.push(buffer.data(),got);
        }
    }
    return OK;
}

//---------------------------------------------------------------
bool VoicePlayer::needsFill() const noexcept
{
    if (!d->opened.load(std::memory_order_acquire))
    {
        return false;
    }
    if (d->seekRequest.load(std::memory_order_acquire)>=0)
    {
        return true;
    }
    if (d->speed.load(std::memory_order_acquire)!=d->appliedSpeed.load(std::memory_order_acquire))
    {
        return true;
    }
    return !d->decoderFinished.load(std::memory_order_acquire) && d->ring.writable()>=VoiceFrameSamples;
}

//---------------------------------------------------------------
size_t VoicePlayer::pull(int16_t* out, size_t frames) noexcept
{
    // A seek happened since the last call: drop the audio from before it and move the clock.
    const auto epoch=d->seekEpoch.load(std::memory_order_acquire);
    if (epoch!=d->seenEpoch)
    {
        d->ring.discardUpTo(d->seekMarker.load(std::memory_order_relaxed));
        d->epochBase=d->seekTarget.load(std::memory_order_relaxed);
        d->epochPulled=0;
        d->epochSpeed=static_cast<double>(d->seekSpeed.load(std::memory_order_relaxed));
        d->playedFrames.store(d->epochBase,std::memory_order_relaxed);
        d->seenEpoch=epoch;
        d->ended.store(false,std::memory_order_relaxed);
    }

    if (out==nullptr || frames==0 || !d->playing.load(std::memory_order_acquire))
    {
        return 0;
    }

    const auto n=d->ring.read(out,frames);
    if (n!=0)
    {
        // message time: at speed 1 exactly the frames pulled, at 2 twice as many. Rounding at
        // another speed can overshoot the end by a frame, which must not show as a position past
        // the duration.
        d->epochPulled+=n;
        const auto position=d->epochBase+static_cast<uint64_t>(std::llround(static_cast<double>(d->epochPulled)*d->epochSpeed));
        d->playedFrames.store(std::min<uint64_t>(position,d->totalFrames.load(std::memory_order_relaxed)),std::memory_order_relaxed);
    }

    if (n==0 && d->decoderFinished.load(std::memory_order_acquire) && d->ring.readable()==0)
    {
        if (d->epochSpeed!=1.0)
        {
            // the frames pulled at another speed do not add up to exactly the rest of the message
            d->playedFrames.store(d->totalFrames.load(std::memory_order_relaxed),std::memory_order_relaxed);
        }
        d->ended.store(true,std::memory_order_release);
        d->playing.store(false,std::memory_order_release);
    }
    return n;
}

//---------------------------------------------------------------
PlayerState VoicePlayer::state() const noexcept
{
    if (!d->opened.load(std::memory_order_acquire))
    {
        return PlayerState::Closed;
    }
    if (d->ended.load(std::memory_order_acquire))
    {
        return PlayerState::Ended;
    }
    return d->playing.load(std::memory_order_acquire)?PlayerState::Playing:PlayerState::Paused;
}

//---------------------------------------------------------------
uint64_t VoicePlayer::positionMs() const noexcept
{
    const auto request=d->seekRequest.load(std::memory_order_acquire);
    if (request>=0)
    {
        return voiceFramesToMs(static_cast<uint64_t>(request));
    }
    return voiceFramesToMs(d->playedFrames.load(std::memory_order_relaxed));
}

//---------------------------------------------------------------
uint64_t VoicePlayer::durationMs() const noexcept
{
    return voiceFramesToMs(d->totalFrames.load(std::memory_order_relaxed));
}

//---------------------------------------------------------------

HATN_MEDIA_NAMESPACE_END
