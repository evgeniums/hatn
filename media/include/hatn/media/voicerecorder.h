/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/voicerecorder.h
  *
  *  Voice message recorder: PCM in, Ogg Opus file and waveform out.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAVOICERECORDER_H
#define HATNMEDIAVOICERECORDER_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <hatn/common/error.h>
#include <hatn/common/file.h>

#include <hatn/media/media.h>
#include <hatn/media/audioformat.h>
#include <hatn/media/opuscodec.h>

HATN_MEDIA_NAMESPACE_BEGIN

//! Recorder settings.
struct VoiceRecorderConfig
{
    /**
     * Recordings shorter than this are refused by finish() with MediaError::RECORDING_TOO_SHORT,
     * so a 200 ms fat-finger on the microphone button never becomes a message.
     */
    uint32_t minDurationMs=1000;

    /**
     * Hard cap. Once this much audio was accepted pushPcm() takes no more and limitReached()
     * turns true; the host is expected to finish() the recording.
     */
    uint32_t maxDurationMs=300000;

    OpusEncoderConfig encoder;

    /**
     * Capacity of the buffer between the audio thread and the worker, in milliseconds of audio.
     * It only has to cover how late the worker may be in calling process().
     */
    uint32_t ringMs=2000;
};

//! What a finished recording gives the message.
struct VoiceRecording
{
    uint32_t durationMs=0;

    //! WaveformExtractor::WaveformBuckets bytes, ready to go into the message metadata.
    std::vector<uint8_t> waveform;

    //! Size of the Ogg Opus stream that was written to the file.
    uint64_t fileBytes=0;

    uint32_t sampleRate=VoiceSampleRate;
    uint32_t channels=VoiceChannels;
};

enum class RecorderState : uint8_t
{
    Idle,       //!< created, start() not called
    Recording,  //!< pushPcm() accepts audio
    Paused,     //!< pushPcm() ignores audio, resume() continues
    Finished,   //!< finish() succeeded, the file is complete
    Cancelled,  //!< cancel(), or finish() refused a too-short recording
    Failed      //!< an encode or file error stopped the recording
};

class VoiceRecorder_p;

/**
 * @brief The voice message record state machine.
 *
 *  Idle -> Recording <-> Paused -> Finished
 *                 \______\______-> Cancelled / Failed
 *
 * A recorder records ONE message: create a new one for the next, and also after a start() that
 * failed, since the encoder it created is not torn down until the recorder is destroyed.
 *
 * THREADS. This class does no threading of its own. There are three roles:
 *  - The AUDIO thread (a platform capture callback, possibly realtime) calls only pushPcm().
 *    That call is wait-free: it copies into a lock-free ring buffer and touches atomics. It never
 *    allocates, blocks or takes a lock, so it is safe in a realtime callback. There must be one
 *    audio thread.
 *  - A WORKER thread calls process() regularly (every 20-50 ms is plenty; the ring holds
 *    ringMs). It drains the ring, encodes, writes the file and updates the waveform and level.
 *    Encoding and file I/O, including encrypted writes, happen here and only here.
 *  - The CONTROL thread (UI) calls start/pause/resume/finish/cancel and reads the getters.
 * Control calls and process() serialize on an internal mutex, so the worker and control threads
 * may be the same or different. Scheduling the worker (a hatn thread pool, a timer) is the
 * caller's job so that this library depends on no application framework.
 *
 * Without HATN_MEDIA_HAS_OGG_OPUS start() fails with MediaError::CODEC_UNAVAILABLE.
 */
class HATN_MEDIA_EXPORT VoiceRecorder
{
    public:

        explicit VoiceRecorder(const VoiceRecorderConfig& config=VoiceRecorderConfig{});
        ~VoiceRecorder();

        VoiceRecorder(const VoiceRecorder&)=delete;
        VoiceRecorder(VoiceRecorder&&)=delete;
        VoiceRecorder& operator=(const VoiceRecorder&)=delete;
        VoiceRecorder& operator=(VoiceRecorder&&)=delete;

        /**
         * @brief Begin recording into `file`.
         * @param file Open for writing and positioned at 0; it must outlive the recorder. Open a
         *        crypt::CryptFile to record encrypted straight to disk. The recorder never closes it.
         */
        Error start(common::File& file);

        /**
         * @brief Audio thread: hand over captured mono 48 kHz PCM.
         * @return Frames accepted. Fewer than `frames` when the recorder is not Recording, when the
         *         duration cap is reached (limitReached()), or when the ring is full because the
         *         worker fell behind (counted by overruns()).
         */
        size_t pushPcm(const int16_t* samples, size_t frames) noexcept;

        //! Worker: drain the ring, encode and write. Cheap when there is nothing to do.
        Error process();

        //! Stop accepting audio and flush what is queued. Recording -> Paused.
        Error pause();

        //! Paused -> Recording.
        Error resume();

        /**
         * @brief Finish and close the stream. Recording|Paused -> Finished.
         *
         * If the audio is shorter than VoiceRecorderConfig::minDurationMs this returns
         * MediaError::RECORDING_TOO_SHORT and the state becomes Cancelled; the file is then
         * incomplete and the caller should delete it.
         */
        Error finish(VoiceRecording& recording);

        //! Abandon the recording. The file is left as it is; deleting it is the caller's job.
        void cancel();

        RecorderState state() const noexcept;

        //! Audio accepted so far, in milliseconds. Cheap, for a duration label.
        uint32_t elapsedMs() const noexcept;

        //! RMS level of the most recently processed audio in dBFS, [WaveformExtractor::FloorDb, 0].
        float levelDb() const noexcept;

        //! The maxDurationMs cap was reached; call finish().
        bool limitReached() const noexcept;

        //! Frames dropped because the worker did not keep up. Non-zero means the file has a gap.
        uint64_t overruns() const noexcept;

    private:

        std::unique_ptr<VoiceRecorder_p> d;
};

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAVOICERECORDER_H
