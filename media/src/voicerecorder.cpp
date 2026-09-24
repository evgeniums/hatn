/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/voicerecorder.cpp
  *
  *      Contains implementation of VoiceRecorder.
  *
  */

#include <atomic>
#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>

#include <hatn/media/mediaerror.h>
#include <hatn/media/pcmring.h>
#include <hatn/media/waveformextractor.h>
#include <hatn/media/oggopuswriter.h>
#include <hatn/media/voicerecorder.h>

HATN_MEDIA_NAMESPACE_BEGIN

/********************** VoiceRecorder **************************/

class VoiceRecorder_p
{
    public:

        explicit VoiceRecorder_p(const VoiceRecorderConfig& cfg)
            : config(cfg),
              ring(static_cast<size_t>(voiceMsToFrames(std::max<uint32_t>(cfg.ringMs,100)))),
              maxFrames(voiceMsToFrames(cfg.maxDurationMs))
        {}

        RecorderState getState() const noexcept
        {
            return static_cast<RecorderState>(state.load(std::memory_order_acquire));
        }

        void setState(RecorderState value) noexcept
        {
            state.store(static_cast<uint8_t>(value),std::memory_order_release);
        }

        //! Mark the recording as failed and hand the error back.
        Error fail(Error ec)
        {
            writer.abort();
            setState(RecorderState::Failed);
            return ec;
        }

        Error encodeFrame()
        {
            std::array<uint8_t,OpusMaxPacketBytes> packet;
            size_t bytes=0;
            auto ec=encoder.encode(frame.data(),packet.data(),packet.size(),bytes);
            if (ec)
            {
                return ec;
            }
            ec=writer.writePacket(packet.data(),bytes,VoiceFrameSamples);
            if (ec)
            {
                return ec;
            }
            encodedFrames+=VoiceFrameSamples;
            return OK;
        }

        //! Take `count` PCM frames through waveform, level and encoder.
        Error consume(const int16_t* samples, size_t count)
        {
            waveform.add(samples,count);
            level.store(WaveformExtractor::rmsDb(samples,count),std::memory_order_relaxed);
            processedFrames+=count;

            size_t offset=0;
            while (offset<count)
            {
                const auto take=std::min(count-offset,static_cast<size_t>(VoiceFrameSamples)-frameFill);
                std::memcpy(frame.data()+frameFill,samples+offset,take*sizeof(int16_t));
                frameFill+=take;
                offset+=take;

                if (frameFill==VoiceFrameSamples)
                {
                    auto ec=encodeFrame();
                    if (ec)
                    {
                        return ec;
                    }
                    frameFill=0;
                }
            }
            return OK;
        }

        //! Empty the ring through consume(). Caller holds the mutex.
        Error drain()
        {
            std::array<int16_t,2*VoiceFrameSamples> chunk;
            for (;;)
            {
                const auto n=ring.read(chunk.data(),chunk.size());
                if (n==0)
                {
                    return OK;
                }
                auto ec=consume(chunk.data(),n);
                if (ec)
                {
                    return fail(ec);
                }
            }
        }

        static bool isActive(RecorderState value) noexcept
        {
            return value==RecorderState::Recording || value==RecorderState::Paused;
        }

        VoiceRecorderConfig config;
        PcmRing ring;
        const uint64_t maxFrames;

        std::atomic<uint8_t> state{static_cast<uint8_t>(RecorderState::Idle)};
        std::atomic<uint64_t> acceptedFrames{0};
        std::atomic<uint64_t> overrunFrames{0};
        std::atomic<bool> limit{false};
        std::atomic<float> level{static_cast<float>(WaveformExtractor::FloorDb)};

        // Everything below is touched only under `mutex`.
        std::mutex mutex;
        OpusFrameEncoder encoder;
        OggOpusWriter writer;
        WaveformExtractor waveform;
        std::array<int16_t,VoiceFrameSamples> frame;
        size_t frameFill=0;
        uint64_t processedFrames=0;     //!< recorded PCM frames taken from the ring
        uint64_t encodedFrames=0;       //!< PCM frames covered by packets given to the writer
        uint32_t preSkip=0;             //!< encoder delay written to the stream header
};

//---------------------------------------------------------------
VoiceRecorder::VoiceRecorder(const VoiceRecorderConfig& config)
    : d(std::make_unique<VoiceRecorder_p>(config))
{}

//---------------------------------------------------------------
VoiceRecorder::~VoiceRecorder()=default;

//---------------------------------------------------------------
Error VoiceRecorder::start(common::File& file)
{
    std::lock_guard<std::mutex> lock(d->mutex);

    if (d->getState()!=RecorderState::Idle)
    {
        return mediaError(MediaError::INVALID_STATE);
    }

    auto ec=d->encoder.init(d->config.encoder);
    if (ec)
    {
        return ec;
    }

    // The encoder's own delay becomes the pre-skip, so that a decoder starts playing at the
    // first sample that was recorded.
    const auto preSkip=static_cast<uint16_t>(std::min<uint32_t>(d->encoder.lookahead(),0xFFFF));
    ec=d->writer.open(file,preSkip);
    if (ec)
    {
        return ec;
    }

    d->waveform.reset();
    d->frameFill=0;
    d->processedFrames=0;
    d->encodedFrames=0;
    d->preSkip=preSkip;
    d->setState(RecorderState::Recording);
    return OK;
}

//---------------------------------------------------------------
size_t VoiceRecorder::pushPcm(const int16_t* samples, size_t frames) noexcept
{
    if (d->getState()!=RecorderState::Recording || samples==nullptr || frames==0)
    {
        return 0;
    }

    // Only this thread writes acceptedFrames, so a plain load/store pair is enough.
    const auto accepted=d->acceptedFrames.load(std::memory_order_relaxed);
    if (accepted>=d->maxFrames)
    {
        d->limit.store(true,std::memory_order_release);
        return 0;
    }

    const auto allowed=static_cast<size_t>(std::min<uint64_t>(frames,d->maxFrames-accepted));
    const auto written=d->ring.write(samples,allowed);
    if (written<allowed)
    {
        d->overrunFrames.fetch_add(allowed-written,std::memory_order_relaxed);
    }

    const auto total=accepted+written;
    d->acceptedFrames.store(total,std::memory_order_release);
    if (total>=d->maxFrames)
    {
        d->limit.store(true,std::memory_order_release);
    }
    return written;
}

//---------------------------------------------------------------
Error VoiceRecorder::process()
{
    // A control call may be running; it drains the ring itself, so there is nothing to wait for.
    std::unique_lock<std::mutex> lock(d->mutex,std::try_to_lock);
    if (!lock.owns_lock() || !VoiceRecorder_p::isActive(d->getState()))
    {
        return OK;
    }
    return d->drain();
}

//---------------------------------------------------------------
Error VoiceRecorder::pause()
{
    std::lock_guard<std::mutex> lock(d->mutex);

    if (d->getState()!=RecorderState::Recording)
    {
        return mediaError(MediaError::INVALID_STATE);
    }

    // stop accepting first, then take what was already accepted
    d->setState(RecorderState::Paused);
    auto ec=d->drain();
    if (ec)
    {
        return ec;
    }

    // a pause is where the user may walk away or the app be killed, so put it on disk now
    ec=d->writer.flush();
    if (ec)
    {
        return d->fail(ec);
    }
    return OK;
}

//---------------------------------------------------------------
Error VoiceRecorder::resume()
{
    std::lock_guard<std::mutex> lock(d->mutex);

    if (d->getState()!=RecorderState::Paused)
    {
        return mediaError(MediaError::INVALID_STATE);
    }
    if (d->limit.load(std::memory_order_acquire))
    {
        // nothing more can be recorded, resuming would only accept zero frames
        return mediaError(MediaError::INVALID_STATE);
    }

    d->setState(RecorderState::Recording);
    return OK;
}

//---------------------------------------------------------------
Error VoiceRecorder::finish(VoiceRecording& recording)
{
    std::lock_guard<std::mutex> lock(d->mutex);

    if (!VoiceRecorder_p::isActive(d->getState()))
    {
        return mediaError(MediaError::INVALID_STATE);
    }

    // no more audio in; then take everything still queued
    d->setState(RecorderState::Paused);
    auto ec=d->drain();
    if (ec)
    {
        return ec;
    }

    const auto durationMs=voiceFramesToMs(d->processedFrames);
    if (durationMs<d->config.minDurationMs)
    {
        d->writer.abort();
        d->setState(RecorderState::Cancelled);
        return mediaError(MediaError::RECORDING_TOO_SHORT);
    }

    // The decoder drops the first `preSkip` samples it produces, and those are the encoder's own
    // delay: the recorded audio comes out `preSkip` samples LATER than it went in. So for the last
    // recorded sample to be played, the decoder has to produce processedFrames + preSkip samples
    // in all. Whatever real audio is left in an incomplete frame is padded with silence to a
    // whole frame (Opus needs one), and more silent frames follow until the delay line is
    // flushed. The excess is then trimmed off again through the end-of-stream granule position,
    // so the decoder plays exactly what was recorded.
    const auto needed=d->processedFrames+d->preSkip;
    if (d->frameFill!=0)
    {
        std::fill(d->frame.begin()+static_cast<std::ptrdiff_t>(d->frameFill),d->frame.end(),int16_t{0});
        ec=d->encodeFrame();
        if (ec)
        {
            return d->fail(ec);
        }
        d->frameFill=0;
    }
    std::fill(d->frame.begin(),d->frame.end(),int16_t{0});
    while (d->encodedFrames<needed)
    {
        ec=d->encodeFrame();
        if (ec)
        {
            return d->fail(ec);
        }
    }
    const auto trim=static_cast<uint32_t>(d->encodedFrames-needed);

    ec=d->writer.finish(trim);
    if (ec)
    {
        return d->fail(ec);
    }

    recording.durationMs=static_cast<uint32_t>(durationMs);
    recording.waveform=d->waveform.buckets();
    recording.fileBytes=d->writer.bytesWritten();
    recording.sampleRate=VoiceSampleRate;
    recording.channels=VoiceChannels;

    d->setState(RecorderState::Finished);
    return OK;
}

//---------------------------------------------------------------
void VoiceRecorder::cancel()
{
    std::lock_guard<std::mutex> lock(d->mutex);

    const auto current=d->getState();
    if (current==RecorderState::Finished || current==RecorderState::Cancelled || current==RecorderState::Failed)
    {
        return;
    }

    d->setState(RecorderState::Cancelled);
    d->writer.abort();

    // Holding the mutex makes this thread the ring's consumer, so it may drop the queued audio.
    d->ring.discardUpTo(d->ring.writeIndex());
}

//---------------------------------------------------------------
RecorderState VoiceRecorder::state() const noexcept
{
    return d->getState();
}

//---------------------------------------------------------------
uint32_t VoiceRecorder::elapsedMs() const noexcept
{
    return static_cast<uint32_t>(voiceFramesToMs(d->acceptedFrames.load(std::memory_order_acquire)));
}

//---------------------------------------------------------------
float VoiceRecorder::levelDb() const noexcept
{
    return d->level.load(std::memory_order_relaxed);
}

//---------------------------------------------------------------
bool VoiceRecorder::limitReached() const noexcept
{
    return d->limit.load(std::memory_order_acquire);
}

//---------------------------------------------------------------
uint64_t VoiceRecorder::overruns() const noexcept
{
    return d->overrunFrames.load(std::memory_order_relaxed);
}

//---------------------------------------------------------------

HATN_MEDIA_NAMESPACE_END
