/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/opuscodec.h
  *
  *  Thin RAII wrappers over libopus for voice: one frame in, one packet out and back.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAOPUSCODEC_H
#define HATNMEDIAOPUSCODEC_H

#include <cstddef>
#include <cstdint>
#include <memory>

#include <hatn/common/error.h>

#include <hatn/media/media.h>
#include <hatn/media/audioformat.h>

HATN_MEDIA_NAMESPACE_BEGIN

//! Encoder settings. Defaults are the voice-message starting point from todo-voice-messages.md.
struct OpusEncoderConfig
{
    //! Target bitrate, bits per second. 24000..32000 is ~3 KB/s, so a minute is ~180 KB.
    int bitrate=32000;

    //! 0..10. Higher costs CPU; 8 is fine on desktop, 5..8 on mobile.
    int complexity=8;

    bool vbr=true;

    //! Off on purpose: there is no packet loss when writing a file.
    bool inbandFec=false;

    //! Off on purpose: DTX would save bytes on silence at the cost of duration/waveform integrity.
    bool dtx=false;
};

//! Largest packet Opus can produce for one frame (RFC 6716), a safe output buffer size.
constexpr const size_t OpusMaxPacketBytes=1275;

//! Largest amount of audio one Opus packet can decode to, 120 ms at 48 kHz.
constexpr const size_t OpusMaxFrameSamples=5760;

class OpusFrameEncoder_p;
class OpusFrameDecoder_p;

/**
 * @brief Encodes 20 ms mono frames of 48 kHz PCM to Opus packets (OPUS_APPLICATION_VOIP).
 *
 * Without HATN_MEDIA_HAS_OGG_OPUS every method fails with MediaError::CODEC_UNAVAILABLE.
 * Not thread-safe.
 */
class HATN_MEDIA_EXPORT OpusFrameEncoder
{
    public:

        OpusFrameEncoder();
        ~OpusFrameEncoder();

        OpusFrameEncoder(const OpusFrameEncoder&)=delete;
        OpusFrameEncoder(OpusFrameEncoder&&)=delete;
        OpusFrameEncoder& operator=(const OpusFrameEncoder&)=delete;
        OpusFrameEncoder& operator=(OpusFrameEncoder&&)=delete;

        Error init(const OpusEncoderConfig& config=OpusEncoderConfig{});

        bool isInitialized() const noexcept;

        /**
         * @brief Encoder delay in 48 kHz samples, to be written as the pre-skip of the Ogg header.
         *
         * A decoder discards this many samples from the start so that the first sample it plays is
         * the first sample that was recorded. Valid after init().
         */
        uint32_t lookahead() const noexcept;

        /**
         * @brief Encode exactly VoiceFrameSamples frames.
         * @param pcm VoiceFrameSamples mono samples.
         * @param out Buffer for the packet, at least OpusMaxPacketBytes for a guaranteed fit.
         * @param maxOutBytes Size of `out`.
         * @param outBytes Receives the packet size.
         */
        Error encode(const int16_t* pcm, uint8_t* out, size_t maxOutBytes, size_t& outBytes);

    private:

        std::unique_ptr<OpusFrameEncoder_p> d;
};

/**
 * @brief Decodes Opus packets to 48 kHz mono PCM. Not thread-safe.
 */
class HATN_MEDIA_EXPORT OpusFrameDecoder
{
    public:

        OpusFrameDecoder();
        ~OpusFrameDecoder();

        OpusFrameDecoder(const OpusFrameDecoder&)=delete;
        OpusFrameDecoder(OpusFrameDecoder&&)=delete;
        OpusFrameDecoder& operator=(const OpusFrameDecoder&)=delete;
        OpusFrameDecoder& operator=(OpusFrameDecoder&&)=delete;

        /**
         * @brief Create the decoder.
         * @param outputGainQ8 The Ogg Opus header's output gain, Q7.8 dB. Applied to all output.
         */
        Error init(int16_t outputGainQ8=0);

        bool isInitialized() const noexcept;

        /**
         * @brief Decode one packet.
         * @param data Packet.
         * @param bytes Packet size.
         * @param pcm Output, room for at least maxFrames samples.
         * @param maxFrames Size of `pcm` in samples, OpusMaxFrameSamples is always enough.
         * @param outFrames Receives the number of samples decoded.
         */
        Error decode(const uint8_t* data, size_t bytes, int16_t* pcm, size_t maxFrames, size_t& outFrames);

        //! Forget all decoder history, for a seek. The next packets decode as from a fresh stream.
        Error reset();

    private:

        std::unique_ptr<OpusFrameDecoder_p> d;
};

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAOPUSCODEC_H
