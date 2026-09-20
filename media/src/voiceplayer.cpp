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

        //! Frame that the next pulled sample belongs to.
        std::atomic<uint64_t> playedFrames{0};
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
Error VoicePlayer::fill()
{
    if (!d->opened.load(std::memory_order_acquire))
    {
        return OK;
    }

    const auto request=d->seekRequest.exchange(-1,std::memory_order_acq_rel);
    if (request>=0)
    {
        auto ec=d->reader.seek(static_cast<uint64_t>(request));
        if (ec)
        {
            return ec;
        }

        // Order matters: forget "finished" first, so the audio thread cannot see the new epoch
        // together with a stale end-of-stream, then publish the marker and target, then the epoch.
        d->decoderFinished.store(false,std::memory_order_release);
        d->seekMarker.store(d->ring.writeIndex(),std::memory_order_relaxed);
        d->seekTarget.store(d->reader.position(),std::memory_order_relaxed);
        d->seekEpoch.fetch_add(1,std::memory_order_release);
    }

    if (d->decoderFinished.load(std::memory_order_acquire))
    {
        return OK;
    }

    std::array<int16_t,VoiceFrameSamples> buffer;
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
        d->playedFrames.store(d->seekTarget.load(std::memory_order_relaxed),std::memory_order_relaxed);
        d->seenEpoch=epoch;
        d->ended.store(false,std::memory_order_relaxed);
    }

    if (out==nullptr || frames==0 || !d->playing.load(std::memory_order_acquire))
    {
        return 0;
    }

    const auto n=d->ring.read(out,frames);
    d->playedFrames.fetch_add(n,std::memory_order_relaxed);

    if (n==0 && d->decoderFinished.load(std::memory_order_acquire) && d->ring.readable()==0)
    {
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
