/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/voicecrop.cpp
  *
  *      Contains implementation of cropVoice().
  *
  */

#include <algorithm>
#include <array>
#include <limits>

#include <hatn/media/mediaerror.h>
#include <hatn/media/oggopusreader.h>
#include <hatn/media/voicecrop.h>

HATN_MEDIA_NAMESPACE_BEGIN

//---------------------------------------------------------------
Error cropVoice(
        common::File& in,
        common::File& out,
        uint64_t startFrame,
        uint64_t endFrame,
        VoiceRecording& recording,
        const OpusEncoderConfig& encoder
    )
{
    if (&in==&out || endFrame<=startFrame)
    {
        return mediaError(MediaError::INVALID_ARGUMENT);
    }

    OggOpusReader reader;
    auto ec=reader.open(in);
    if (ec)
    {
        return ec;
    }

    const auto total=reader.totalFrames();
    if (startFrame>=total)
    {
        return mediaError(MediaError::INVALID_ARGUMENT);
    }
    endFrame=std::min(endFrame,total);
    const auto wanted=endFrame-startFrame;

    ec=reader.seek(startFrame);
    if (ec)
    {
        return ec;
    }

    // Re-encoding goes through a VoiceRecorder so the result is a normal voice message: it owns
    // the waveform, the exact-length trimming and the flush of the encoder's delay. The limits
    // that make sense for a live recording are switched off: this is a deliberate edit of
    // something that already exists.
    VoiceRecorderConfig config;
    config.encoder=encoder;
    config.minDurationMs=0;
    config.maxDurationMs=std::numeric_limits<uint32_t>::max();

    VoiceRecorder recorder(config);
    ec=recorder.start(out);
    if (ec)
    {
        return ec;
    }

    std::array<int16_t,VoiceFrameSamples> buffer;
    uint64_t copied=0;
    while (copied<wanted)
    {
        const auto want=static_cast<size_t>(std::min<uint64_t>(buffer.size(),wanted-copied));
        size_t got=0;
        ec=reader.read(buffer.data(),want,got);
        if (ec)
        {
            recorder.cancel();
            return ec;
        }
        if (got==0)
        {
            // the file ended before its own length said it would
            break;
        }

        // The ring is far larger than one chunk and process() empties it every time, so this
        // always accepts everything; anything else means the recorder is not recording.
        if (recorder.pushPcm(buffer.data(),got)!=got)
        {
            recorder.cancel();
            return mediaError(MediaError::INVALID_STATE);
        }
        ec=recorder.process();
        if (ec)
        {
            recorder.cancel();
            return ec;
        }
        copied+=got;
    }

    if (copied==0)
    {
        recorder.cancel();
        return mediaError(MediaError::TRUNCATED_STREAM);
    }

    return recorder.finish(recording);
}

//---------------------------------------------------------------

HATN_MEDIA_NAMESPACE_END
