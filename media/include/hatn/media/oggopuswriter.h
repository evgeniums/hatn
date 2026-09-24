/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/oggopuswriter.h
  *
  *  Writes Opus packets into an Ogg stream (RFC 7845) through common::File.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAOGGOPUSWRITER_H
#define HATNMEDIAOGGOPUSWRITER_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <hatn/common/error.h>
#include <hatn/common/file.h>

#include <hatn/media/media.h>
#include <hatn/media/audioformat.h>

HATN_MEDIA_NAMESPACE_BEGIN

class OggOpusWriter_p;

/**
 * @brief Writes a mono 48 kHz Opus stream in an Ogg container.
 *
 * Output goes to the file as pages are completed, roughly every 4 KB of audio (about 1.3 s of
 * voice), never into a memory buffer, so a long recording is not a heap spike and a crash loses
 * at most the last unflushed page. flush() forces the queued audio out (call it on pause).
 * A stream cut short by a crash has no end-of-stream flag but is still playable, see
 * OggOpusReader.
 *
 * The file is a common::File the CALLER has opened for writing and positioned at 0, so the same
 * writer serves a plain file and a crypt::CryptFile whose keys the caller has configured.
 * The writer does not close it.
 *
 * Without HATN_MEDIA_HAS_OGG_OPUS open() fails with MediaError::CODEC_UNAVAILABLE.
 * Not thread-safe.
 */
class HATN_MEDIA_EXPORT OggOpusWriter
{
    public:

        OggOpusWriter();
        ~OggOpusWriter();

        OggOpusWriter(const OggOpusWriter&)=delete;
        OggOpusWriter(OggOpusWriter&&)=delete;
        OggOpusWriter& operator=(const OggOpusWriter&)=delete;
        OggOpusWriter& operator=(OggOpusWriter&&)=delete;

        /**
         * @brief Start a stream: writes the OpusHead and OpusTags pages.
         * @param file Open for writing, positioned at 0. Must outlive the writer.
         * @param preSkip Encoder delay in 48 kHz samples, see OpusFrameEncoder::lookahead().
         * @param vendor Vendor string for the OpusTags header.
         */
        Error open(common::File& file, uint16_t preSkip, const std::string& vendor="hatn");

        bool isOpen() const noexcept;

        /**
         * @brief Append one encoded packet.
         * @param data Packet.
         * @param bytes Packet size.
         * @param frames PCM frames the packet covers, VoiceFrameSamples for voice.
         *
         * The LAST packet is held back until the next call or finish(), because only then is it
         * known that it is the last and must carry the end-of-stream flag.
         */
        Error writePacket(const uint8_t* data, size_t bytes, uint32_t frames);

        //! Push everything queued out to the file now. The held-back last packet stays held.
        Error flush();

        /**
         * @brief End the stream.
         * @param trimFrames Frames at the end of the LAST packet that are not part of the
         *        recording: the silence that pads an incomplete final frame to a whole one and
         *        flushes the encoder's delay. They are excluded from the stream's length, so a
         *        decoder plays exactly what was recorded. Must be less than the last packet's frames.
         *
         * The stream's playable length is (framesWritten() - trimFrames) minus the pre-skip, so to
         * end up with exactly N recorded frames the caller writes packets until framesWritten() is
         * at least N plus the pre-skip and passes the excess as `trimFrames`. VoiceRecorder does that.
         */
        Error finish(uint32_t trimFrames=0);

        //! Drop the stream without an end-of-stream marker. For a cancelled recording.
        void abort() noexcept;

        //! Bytes handed to the file so far.
        uint64_t bytesWritten() const noexcept;

        //! PCM frames covered by all packets written so far, including the held-back one.
        uint64_t framesWritten() const noexcept;

    private:

        std::unique_ptr<OggOpusWriter_p> d;
};

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAOGGOPUSWRITER_H
