/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/opuscodec.cpp
  *
  *      Contains implementation of OpusFrameEncoder and OpusFrameDecoder.
  *
  */

#include <hatn/media/mediaerror.h>
#include <hatn/media/opuscodec.h>

#ifdef HATN_MEDIA_HAS_OGG_OPUS
#include <opus.h>
#endif

HATN_MEDIA_NAMESPACE_BEGIN

#ifdef HATN_MEDIA_HAS_OGG_OPUS

/********************** OpusFrameEncoder **************************/

class OpusFrameEncoder_p
{
    public:

        ~OpusFrameEncoder_p()
        {
            if (encoder!=nullptr)
            {
                opus_encoder_destroy(encoder);
            }
        }

        OpusEncoder* encoder=nullptr;
        uint32_t lookahead=0;
};

//---------------------------------------------------------------
OpusFrameEncoder::OpusFrameEncoder() : d(std::make_unique<OpusFrameEncoder_p>())
{}

//---------------------------------------------------------------
OpusFrameEncoder::~OpusFrameEncoder()=default;

//---------------------------------------------------------------
Error OpusFrameEncoder::init(const OpusEncoderConfig& config)
{
    if (d->encoder!=nullptr)
    {
        return mediaError(MediaError::INVALID_STATE);
    }

    int err=OPUS_OK;
    auto* encoder=opus_encoder_create(static_cast<opus_int32>(VoiceSampleRate),static_cast<int>(VoiceChannels),OPUS_APPLICATION_VOIP,&err);
    if (encoder==nullptr || err!=OPUS_OK)
    {
        return mediaError(MediaError::ENCODER_INIT_FAILED);
    }

    // Any failing ctl means a rejected setting; report it rather than encode with a surprise.
    auto ok=opus_encoder_ctl(encoder,OPUS_SET_BITRATE(static_cast<opus_int32>(config.bitrate)))==OPUS_OK;
    ok=ok && opus_encoder_ctl(encoder,OPUS_SET_COMPLEXITY(static_cast<opus_int32>(config.complexity)))==OPUS_OK;
    ok=ok && opus_encoder_ctl(encoder,OPUS_SET_VBR(static_cast<opus_int32>(config.vbr?1:0)))==OPUS_OK;
    ok=ok && opus_encoder_ctl(encoder,OPUS_SET_INBAND_FEC(static_cast<opus_int32>(config.inbandFec?1:0)))==OPUS_OK;
    ok=ok && opus_encoder_ctl(encoder,OPUS_SET_DTX(static_cast<opus_int32>(config.dtx?1:0)))==OPUS_OK;

    opus_int32 lookahead=0;
    ok=ok && opus_encoder_ctl(encoder,OPUS_GET_LOOKAHEAD(&lookahead))==OPUS_OK;

    if (!ok)
    {
        opus_encoder_destroy(encoder);
        return mediaError(MediaError::ENCODER_INIT_FAILED);
    }

    d->encoder=encoder;
    d->lookahead=lookahead>0?static_cast<uint32_t>(lookahead):0;
    return OK;
}

//---------------------------------------------------------------
bool OpusFrameEncoder::isInitialized() const noexcept
{
    return d->encoder!=nullptr;
}

//---------------------------------------------------------------
uint32_t OpusFrameEncoder::lookahead() const noexcept
{
    return d->lookahead;
}

//---------------------------------------------------------------
Error OpusFrameEncoder::encode(const int16_t* pcm, uint8_t* out, size_t maxOutBytes, size_t& outBytes)
{
    outBytes=0;
    if (d->encoder==nullptr)
    {
        return mediaError(MediaError::INVALID_STATE);
    }
    if (pcm==nullptr || out==nullptr || maxOutBytes==0)
    {
        return mediaError(MediaError::INVALID_ARGUMENT);
    }

    const auto result=opus_encode(
        d->encoder,
        reinterpret_cast<const opus_int16*>(pcm),
        static_cast<int>(VoiceFrameSamples),
        reinterpret_cast<unsigned char*>(out),
        static_cast<opus_int32>(maxOutBytes)
    );
    if (result<0)
    {
        return mediaError(MediaError::ENCODE_FAILED);
    }

    outBytes=static_cast<size_t>(result);
    return OK;
}

/********************** OpusFrameDecoder **************************/

class OpusFrameDecoder_p
{
    public:

        ~OpusFrameDecoder_p()
        {
            if (decoder!=nullptr)
            {
                opus_decoder_destroy(decoder);
            }
        }

        OpusDecoder* decoder=nullptr;
};

//---------------------------------------------------------------
OpusFrameDecoder::OpusFrameDecoder() : d(std::make_unique<OpusFrameDecoder_p>())
{}

//---------------------------------------------------------------
OpusFrameDecoder::~OpusFrameDecoder()=default;

//---------------------------------------------------------------
Error OpusFrameDecoder::init(int16_t outputGainQ8)
{
    if (d->decoder!=nullptr)
    {
        return mediaError(MediaError::INVALID_STATE);
    }

    int err=OPUS_OK;
    auto* decoder=opus_decoder_create(static_cast<opus_int32>(VoiceSampleRate),static_cast<int>(VoiceChannels),&err);
    if (decoder==nullptr || err!=OPUS_OK)
    {
        return mediaError(MediaError::DECODER_INIT_FAILED);
    }

    if (outputGainQ8!=0 && opus_decoder_ctl(decoder,OPUS_SET_GAIN(static_cast<opus_int32>(outputGainQ8)))!=OPUS_OK)
    {
        opus_decoder_destroy(decoder);
        return mediaError(MediaError::DECODER_INIT_FAILED);
    }

    d->decoder=decoder;
    return OK;
}

//---------------------------------------------------------------
bool OpusFrameDecoder::isInitialized() const noexcept
{
    return d->decoder!=nullptr;
}

//---------------------------------------------------------------
Error OpusFrameDecoder::decode(const uint8_t* data, size_t bytes, int16_t* pcm, size_t maxFrames, size_t& outFrames)
{
    outFrames=0;
    if (d->decoder==nullptr)
    {
        return mediaError(MediaError::INVALID_STATE);
    }
    if (data==nullptr || pcm==nullptr || bytes==0 || maxFrames==0)
    {
        return mediaError(MediaError::INVALID_ARGUMENT);
    }

    const auto result=opus_decode(
        d->decoder,
        reinterpret_cast<const unsigned char*>(data),
        static_cast<opus_int32>(bytes),
        reinterpret_cast<opus_int16*>(pcm),
        static_cast<int>(maxFrames),
        0
    );
    if (result<0)
    {
        return mediaError(MediaError::DECODE_FAILED);
    }

    outFrames=static_cast<size_t>(result);
    return OK;
}

//---------------------------------------------------------------
Error OpusFrameDecoder::reset()
{
    if (d->decoder==nullptr)
    {
        return mediaError(MediaError::INVALID_STATE);
    }
    if (opus_decoder_ctl(d->decoder,OPUS_RESET_STATE)!=OPUS_OK)
    {
        return mediaError(MediaError::DECODER_INIT_FAILED);
    }
    return OK;
}

#else // HATN_MEDIA_HAS_OGG_OPUS

/********************** stubs: no codec in this build **************************/

class OpusFrameEncoder_p
{};

class OpusFrameDecoder_p
{};

OpusFrameEncoder::OpusFrameEncoder() : d(std::make_unique<OpusFrameEncoder_p>()) {}
OpusFrameEncoder::~OpusFrameEncoder()=default;
Error OpusFrameEncoder::init(const OpusEncoderConfig&) { return mediaError(MediaError::CODEC_UNAVAILABLE); }
bool OpusFrameEncoder::isInitialized() const noexcept { return false; }
uint32_t OpusFrameEncoder::lookahead() const noexcept { return 0; }
Error OpusFrameEncoder::encode(const int16_t*, uint8_t*, size_t, size_t& outBytes)
{
    outBytes=0;
    return mediaError(MediaError::CODEC_UNAVAILABLE);
}

OpusFrameDecoder::OpusFrameDecoder() : d(std::make_unique<OpusFrameDecoder_p>()) {}
OpusFrameDecoder::~OpusFrameDecoder()=default;
Error OpusFrameDecoder::init(int16_t) { return mediaError(MediaError::CODEC_UNAVAILABLE); }
bool OpusFrameDecoder::isInitialized() const noexcept { return false; }
Error OpusFrameDecoder::decode(const uint8_t*, size_t, int16_t*, size_t, size_t& outFrames)
{
    outFrames=0;
    return mediaError(MediaError::CODEC_UNAVAILABLE);
}
Error OpusFrameDecoder::reset() { return mediaError(MediaError::CODEC_UNAVAILABLE); }

#endif // HATN_MEDIA_HAS_OGG_OPUS

//---------------------------------------------------------------

HATN_MEDIA_NAMESPACE_END
