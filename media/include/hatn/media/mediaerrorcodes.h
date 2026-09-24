/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/mediaerrorcodes.h
  *
  * Contains error codes for hatnmedia lib.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAERRORCODES_H
#define HATNMEDIAERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/media/media.h>

#define HATN_MEDIA_ERRORS(Do) \
    Do(MediaError,OK,_TR("OK")) \
    Do(MediaError,CODEC_UNAVAILABLE,_TR("Ogg/Opus codec support is not available in this build","media")) \
    Do(MediaError,INVALID_STATE,_TR("operation is not valid in the current state","media")) \
    Do(MediaError,INVALID_ARGUMENT,_TR("invalid argument","media")) \
    Do(MediaError,UNSUPPORTED_FORMAT,_TR("unsupported audio format","media")) \
    Do(MediaError,FILE_NOT_OPEN,_TR("file is not open","media")) \
    Do(MediaError,ENCODER_INIT_FAILED,_TR("failed to initialize Opus encoder","media")) \
    Do(MediaError,ENCODE_FAILED,_TR("failed to encode audio","media")) \
    Do(MediaError,DECODER_INIT_FAILED,_TR("failed to initialize Opus decoder","media")) \
    Do(MediaError,DECODE_FAILED,_TR("failed to decode audio","media")) \
    Do(MediaError,FILE_WRITE_FAILED,_TR("failed to write audio file","media")) \
    Do(MediaError,FILE_READ_FAILED,_TR("failed to read audio file","media")) \
    Do(MediaError,INVALID_OGG_STREAM,_TR("invalid Ogg stream","media")) \
    Do(MediaError,NOT_OPUS_STREAM,_TR("Ogg stream does not contain Opus audio","media")) \
    Do(MediaError,UNSUPPORTED_OPUS_MAPPING,_TR("unsupported Opus channel mapping","media")) \
    Do(MediaError,TRUNCATED_STREAM,_TR("audio stream is truncated","media")) \
    Do(MediaError,SEEK_FAILED,_TR("failed to seek in audio stream","media")) \
    Do(MediaError,RECORDING_TOO_SHORT,_TR("recording is too short","media"))

HATN_MEDIA_NAMESPACE_BEGIN

//! Error codes of hatnmedia lib.
enum class MediaError : int
{
    HATN_MEDIA_ERRORS(HATN_ERROR_CODE)
};

//! media errors codes as strings.
constexpr const char* const MediaErrorStrings[] = {
    HATN_MEDIA_ERRORS(HATN_ERROR_STR)
};

//! Media error code to string.
inline const char* mediaErrorString(MediaError code)
{
    return errorString(code,MediaErrorStrings);
}

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAERRORCODES_H
