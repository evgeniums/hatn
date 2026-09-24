/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/voicecrop.h
  *
  *  Cropping a recorded voice message to a part of itself.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAVOICECROP_H
#define HATNMEDIAVOICECROP_H

#include <cstdint>

#include <hatn/common/error.h>
#include <hatn/common/file.h>

#include <hatn/media/media.h>
#include <hatn/media/audioformat.h>
#include <hatn/media/opuscodec.h>
#include <hatn/media/voicerecorder.h>

HATN_MEDIA_NAMESPACE_BEGIN

/**
 * @brief Write the part [startFrame, endFrame) of an Ogg Opus voice message as a new message.
 *
 * The range is decoded and encoded again through a VoiceRecorder, so the new file is a normal
 * voice message in every respect: exact length, correct pre-skip, and a waveform computed from
 * the cropped audio itself, ready to go into the message metadata (`recording.waveform`).
 *
 * This costs one more lossy generation. At voice bitrates a single re-encode is not audible in
 * practice; cutting Ogg packets without re-encoding would avoid it but needs page and granule
 * bookkeeping for a few percent of a small file, which is not worth the complexity here.
 *
 * @param in Open for reading. Must be an Ogg Opus stream written by this library.
 * @param out Open for writing and positioned at 0, as a VoiceRecorder expects. It may be an
 *        encrypted crypt::CryptFile. Neither file is closed. `in` and `out` must be different files.
 * @param startFrame First frame to keep, in 48 kHz frames from the start of the message.
 * @param endFrame One past the last frame to keep. Values past the end of the message are clamped to it.
 * @param recording Receives the length, waveform and size of the new message.
 * @param encoder Encoder settings for the new stream.
 *
 * MediaError::INVALID_ARGUMENT if the range is empty or starts at or after the end of the
 * message. There is no minimum length here, unlike a live recording: cropping is a deliberate
 * act and the caller applies whatever minimum it wants. On any error `out` holds an incomplete
 * stream that the caller should discard.
 *
 * Without HATN_MEDIA_HAS_OGG_OPUS this fails with MediaError::CODEC_UNAVAILABLE.
 */
HATN_MEDIA_EXPORT Error cropVoice(
        common::File& in,
        common::File& out,
        uint64_t startFrame,
        uint64_t endFrame,
        VoiceRecording& recording,
        const OpusEncoderConfig& encoder=OpusEncoderConfig{}
    );

//! The same, with the range in milliseconds.
inline Error cropVoiceMs(
        common::File& in,
        common::File& out,
        uint64_t startMs,
        uint64_t endMs,
        VoiceRecording& recording,
        const OpusEncoderConfig& encoder=OpusEncoderConfig{}
    )
{
    return cropVoice(in,out,voiceMsToFrames(startMs),voiceMsToFrames(endMs),recording,encoder);
}

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAVOICECROP_H
