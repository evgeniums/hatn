/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/voiceplayer.h
  *
  *  Voice message player: Ogg Opus file in, PCM out.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAVOICEPLAYER_H
#define HATNMEDIAVOICEPLAYER_H

#include <cstddef>
#include <cstdint>
#include <memory>

#include <hatn/common/error.h>
#include <hatn/common/file.h>

#include <hatn/media/media.h>
#include <hatn/media/audioformat.h>

HATN_MEDIA_NAMESPACE_BEGIN

enum class PlayerState : uint8_t
{
    Closed,     //!< no file open
    Paused,     //!< open, pull() returns nothing
    Playing,    //!< pull() returns audio
    Ended       //!< the last frame was pulled
};

class VoicePlayer_p;

/**
 * @brief Plays an Ogg Opus voice message: decodes ahead into a buffer that a realtime audio
 *        callback drains.
 *
 * THREADS. Like VoiceRecorder this class does no threading and depends on no application
 * framework. Three roles:
 *  - The AUDIO thread (a platform render callback, possibly realtime) calls only pull(). It is
 *    wait-free: it copies from a lock-free ring buffer and touches atomics, so it is safe in a
 *    realtime callback. There must be one audio thread.
 *  - A DECODE thread calls fill() whenever needsFill() says so, or on a timer (every 20-50 ms is
 *    plenty; the buffer holds bufferMs). Decoding, file reads and decryption happen here only,
 *    and the file and decoder are touched by no other thread.
 *  - The CONTROL thread (UI) calls play(), pause(), seekMs() and reads the getters. All of these
 *    are safe from any thread while the player is open.
 * open() and close() must not overlap with any other call.
 *
 * The audio thread never gets silence-padding from here: pull() returns fewer frames than asked
 * for, or none, and the platform layer fills the rest with zeros.
 *
 * Reads through common::File, so a plain file and a crypt::CryptFile are interchangeable.
 * Without HATN_MEDIA_HAS_OGG_OPUS open() fails with MediaError::CODEC_UNAVAILABLE.
 */
class HATN_MEDIA_EXPORT VoicePlayer
{
    public:

        /**
         * @param bufferMs Decoded audio kept ahead of the audio thread. It only has to cover how
         *        late the decode thread may be in calling fill().
         */
        explicit VoicePlayer(uint32_t bufferMs=1000);
        ~VoicePlayer();

        VoicePlayer(const VoicePlayer&)=delete;
        VoicePlayer(VoicePlayer&&)=delete;
        VoicePlayer& operator=(const VoicePlayer&)=delete;
        VoicePlayer& operator=(VoicePlayer&&)=delete;

        /**
         * @brief Open a voice message. Starts Paused at position 0.
         * @param file Open for reading; it must outlive the player. The player never closes it.
         */
        Error open(common::File& file);

        //! Forget the file. Does not close it.
        void close();

        // ---- control thread ---------------------------------------------------------------

        void play() noexcept;

        void pause() noexcept;

        //! Jump to a position. Applied by the next fill(); positionMs() reports it at once.
        void seekMs(uint64_t ms) noexcept;

        // ---- decode thread ----------------------------------------------------------------

        /**
         * @brief Decode ahead until the buffer is full or the file ends, applying a pending seek.
         *
         * A decode or read error is returned and decoding stops there. Audio already buffered can
         * still be pulled, after which pull() returns nothing without reaching Ended, so the
         * caller should pause() and report the error.
         */
        Error fill();

        //! Whether fill() has work to do: a seek is pending or there is room and audio left.
        bool needsFill() const noexcept;

        // ---- audio thread -----------------------------------------------------------------

        /**
         * @brief Copy up to `frames` mono 48 kHz frames into `out`.
         * @return Frames written; 0 when not Playing, when the decode thread has not kept up, or
         *         at the end. At the end the state becomes Ended.
         */
        size_t pull(int16_t* out, size_t frames) noexcept;

        // ---- any thread -------------------------------------------------------------------

        PlayerState state() const noexcept;

        //! Playback position. While a seek is pending it is the seek target.
        uint64_t positionMs() const noexcept;

        uint64_t durationMs() const noexcept;

    private:

        std::unique_ptr<VoicePlayer_p> d;
};

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAVOICEPLAYER_H
