/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/audioformat.h
  *
  *  Fixed PCM format of voice messages and helpers to convert between frames and time.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAAUDIOFORMAT_H
#define HATNMEDIAAUDIOFORMAT_H

#include <cstdint>

#include <hatn/media/media.h>

HATN_MEDIA_NAMESPACE_BEGIN

/**
 * Voice messages are 48 kHz mono, signed 16-bit PCM, always.
 *
 * Opus is internally 48 kHz, so feeding it anything else only adds a resample, and a platform
 * audio layer can capture at 48 kHz mono everywhere. Fixing the format here keeps the codec, the
 * waveform and the record state machine free of a format matrix. A frame below means one sample
 * of every channel, which for mono is just one int16_t.
 */
constexpr const uint32_t VoiceSampleRate=48000;
constexpr const uint32_t VoiceChannels=1;

//! Opus frame duration used for voice. 20 ms is the codec's sweet spot for speech.
constexpr const uint32_t VoiceFrameMs=20;

//! PCM frames in one Opus voice frame (960 at 48 kHz).
constexpr const uint32_t VoiceFrameSamples=VoiceSampleRate*VoiceFrameMs/1000;

//! Convert a number of PCM frames at VoiceSampleRate to milliseconds, rounding down.
constexpr uint64_t voiceFramesToMs(uint64_t frames) noexcept
{
    return frames*1000/VoiceSampleRate;
}

//! Convert milliseconds to a number of PCM frames at VoiceSampleRate.
constexpr uint64_t voiceMsToFrames(uint64_t ms) noexcept
{
    return ms*VoiceSampleRate/1000;
}

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAAUDIOFORMAT_H
