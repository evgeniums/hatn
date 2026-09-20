PROJECT(hatnmedia)

# Optional Opus/Ogg voice codec. Non-REQUIRED, same shape as base/CMakeLists.txt's hunspell block
# and common/CMakeLists.txt's utf8proc: consulted TWICE, because MEDIA_CONFIG_H_TEXT must be set
# before BUILD_HATN_MODULE() (which calls CREATE_CONFIG_FILE) while TARGET_LINK_LIBRARIES below
# needs the imported targets, which are only meaningful once the module target exists.
#
# Both libraries install a CMake package config (OpusConfig.cmake / OggConfig.cmake, targets
# Opus::opus and Ogg::ogg), so plain CONFIG lookups resolve them off CMAKE_PREFIX_PATH, which the
# hatn build scripts point at the deps root. Build them with:
#
#     dep_libs="ogg opus" <hatn>/build/deps/desktop/build-<platform>.sh
FIND_PACKAGE(Opus CONFIG)
FIND_PACKAGE(Ogg CONFIG)

# A build script can state that it REQUIRES the voice codec:
#
#     export HATN_USE_OPUS=1
#
# This does not (and cannot) define the macros by itself -- HATN_USE_OPUS and HATN_USE_OGG are
# written into media/config.h below only when the libraries are actually found, because the
# backend also needs their headers on the include path and them on the link line. What the
# variable does is turn "not found" from a silent STATUS into a hard error, so a build that was
# supposed to have voice messages never quietly ships without them.
IF(NOT (Opus_FOUND AND Ogg_FOUND) AND DEFINED ENV{HATN_USE_OPUS} AND NOT "$ENV{HATN_USE_OPUS}" STREQUAL "0")
    MESSAGE(FATAL_ERROR
        "HATN_USE_OPUS is set in the environment, but libopus and/or libogg were not found.\n"
        "  Build them: dep_libs=\"ogg opus\" <hatn>/build/deps/desktop/build-<platform>.sh\n"
        "              (run it from the directory whose ./deps tree this build uses)\n"
        "Unset HATN_USE_OPUS to build without the Ogg/Opus voice codec.")
ENDIF()

IF(Opus_FOUND)
    SET (MEDIA_CONFIG_H_TEXT "${MEDIA_CONFIG_H_TEXT}\n#ifndef HATN_USE_OPUS \n    #define HATN_USE_OPUS 1\n#endif")
ENDIF()
IF(Ogg_FOUND)
    SET (MEDIA_CONFIG_H_TEXT "${MEDIA_CONFIG_H_TEXT}\n#ifndef HATN_USE_OGG \n    #define HATN_USE_OGG 1\n#endif")
ENDIF()

SET (HEADERS
    include/hatn/media/media.h
    include/hatn/media/mediaerror.h
    include/hatn/media/mediaerrorcodes.h
    include/hatn/media/audioformat.h
    include/hatn/media/pcmring.h
    include/hatn/media/waveformextractor.h
    include/hatn/media/opuscodec.h
    include/hatn/media/oggopuswriter.h
    include/hatn/media/oggopusreader.h
    include/hatn/media/voicerecorder.h
    include/hatn/media/voiceplayer.h
    include/hatn/media/voicecrop.h
)

SET (SOURCES
    src/mediaerror.cpp
    src/waveformextractor.cpp
    src/opuscodec.cpp
    src/oggopuswriter.cpp
    src/oggopusreader.cpp
    src/voicerecorder.cpp
    src/voiceplayer.cpp
    src/voicecrop.cpp
)

BUILD_HATN_MODULE()

# See the FIND_PACKAGE calls above. PRIVATE: no consumer of hatnmedia includes an opus or ogg
# header (the codec classes hide them behind pimpls), so they are not propagated.
IF(Opus_FOUND AND Ogg_FOUND)
    MESSAGE(STATUS "libopus and libogg found, enabling Ogg/Opus voice codec")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE Opus::opus Ogg::ogg)
ELSE()
    # STATUS, not WARNING: opus and ogg are opt-in at the dependency level (not in the default
    # dep_libs list, see build/deps/desktop/config.sh), so a warning here would fire on every
    # default build and train people to ignore warnings.
    MESSAGE(STATUS "libopus and/or libogg NOT found, Ogg/Opus voice codec disabled (Opus: ${Opus_FOUND}, Ogg: ${Ogg_FOUND})")
ENDIF()
