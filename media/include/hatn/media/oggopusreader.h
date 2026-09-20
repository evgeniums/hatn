/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/oggopusreader.h
  *
  *  Reads and decodes an Ogg Opus stream (RFC 7845) through common::File.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAOGGOPUSREADER_H
#define HATNMEDIAOGGOPUSREADER_H

#include <cstddef>
#include <cstdint>
#include <memory>

#include <hatn/common/error.h>
#include <hatn/common/file.h>

#include <hatn/media/media.h>
#include <hatn/media/audioformat.h>

HATN_MEDIA_NAMESPACE_BEGIN

class OggOpusReader_p;

/**
 * @brief Decodes a mono 48 kHz Ogg Opus file to PCM, with random-access seeking.
 *
 * Reads through common::File, so a plain file and a crypt::CryptFile are interchangeable and a
 * voice message can be played straight from encrypted storage. The caller opens the file for
 * reading; the reader never closes it and it must outlive the reader.
 *
 * open() reads only the two header packets and the tail of the file (to learn the length from
 * the last page's granule position). The page index seeking needs is built lazily on the first
 * seek(), so playing from the start costs no extra pass over the file.
 *
 * A stream cut short by a crash has no end-of-stream flag. It still opens and plays up to its
 * last complete page, and isComplete() reports false.
 *
 * Only mono streams with channel mapping family 0 are accepted; that is all this library writes.
 * Without HATN_MEDIA_HAS_OGG_OPUS open() fails with MediaError::CODEC_UNAVAILABLE.
 * Not thread-safe: one thread owns a reader.
 */
class HATN_MEDIA_EXPORT OggOpusReader
{
    public:

        OggOpusReader();
        ~OggOpusReader();

        OggOpusReader(const OggOpusReader&)=delete;
        OggOpusReader(OggOpusReader&&)=delete;
        OggOpusReader& operator=(const OggOpusReader&)=delete;
        OggOpusReader& operator=(OggOpusReader&&)=delete;

        Error open(common::File& file);

        bool isOpen() const noexcept;

        //! Forget the stream. Does not close the file.
        void close() noexcept;

        //! Length in PCM frames at 48 kHz, after pre-skip and end trimming.
        uint64_t totalFrames() const noexcept;

        uint64_t durationMs() const noexcept
        {
            return voiceFramesToMs(totalFrames());
        }

        //! Whether the stream ended with an end-of-stream page. False for a truncated recording.
        bool isComplete() const noexcept;

        //! PCM frame that the next read() will return first.
        uint64_t position() const noexcept;

        /**
         * @brief Decode up to `maxFrames` frames.
         * @param pcm Output buffer.
         * @param maxFrames Size of `pcm` in frames.
         * @param outFrames Receives the number of frames produced. Fewer than `maxFrames`, or 0,
         *        means the end of the stream was reached.
         */
        Error read(int16_t* pcm, size_t maxFrames, size_t& outFrames);

        /**
         * @brief Move to PCM frame `frame`. Clamped to totalFrames().
         *
         * Lands on the exact frame: decoding restarts about 80 ms earlier, as RFC 7845 asks so the
         * decoder has converged, and the excess is discarded.
         */
        Error seek(uint64_t frame);

    private:

        std::unique_ptr<OggOpusReader_p> d;
};

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAOGGOPUSREADER_H
