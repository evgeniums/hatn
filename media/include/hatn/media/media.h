/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/media.h
  *
  *  Hatn media library: voice recording and playback over Ogg/Opus.
  *
  *  The library never opens an audio device. PCM comes in and goes out through plain calls
  *  (see AudioSink, VoiceRecorder::pushPcm(), VoicePlayer::pull()), so a platform layer owns the
  *  microphone/speaker and the audio session, and this library owns everything that must not
  *  differ between platforms: the codec, the waveform, the record state machine.
  *
  *  Files are read and written through common::File, so a plain file and an encrypted one
  *  (crypt::CryptFile) are interchangeable and this library does not depend on hatn crypt.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIADEF_H
#define HATNMEDIADEF_H

#include <hatn/media/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_MEDIA_EXPORT
#   ifdef BUILD_HATN_MEDIA
#       define HATN_MEDIA_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_MEDIA_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_MEDIA_NAMESPACE_BEGIN namespace hatn { namespace media {
#define HATN_MEDIA_NAMESPACE_END }}

#define HATN_MEDIA_NAMESPACE hatn::media
#define HATN_MEDIA_NS media
#define HATN_MEDIA_USING using namespace hatn::media;

/**
 * HATN_USE_OPUS and HATN_USE_OGG are written into media/config.h by CMake only when the
 * corresponding library was found at configure time (see media/CMakeLists.txt). Voice messages
 * need BOTH: Opus for the codec and Ogg for the container. This is the single place that
 * combines them; source files test HATN_MEDIA_HAS_OGG_OPUS rather than the two macros.
 *
 * Without it the library still builds and everything that does not touch the codec keeps working
 * (waveform extraction, PCM ring, format helpers); anything that needs the codec returns
 * MediaError::CODEC_UNAVAILABLE.
 */
#if defined(HATN_USE_OPUS) && defined(HATN_USE_OGG)
#   define HATN_MEDIA_HAS_OGG_OPUS 1
#endif

HATN_MEDIA_NAMESPACE_BEGIN

//! Whether this build of the library can encode and decode Ogg/Opus.
HATN_MEDIA_EXPORT bool isOggOpusAvailable() noexcept;

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIADEF_H
