PROJECT(hatnbase)

# task-spellcheck.md: optional hunspell-backed spell checking. Non-REQUIRED, same shape as
# common/CMakeLists.txt's own FIND_PACKAGE(utf8proc) -- consulted TWICE, because
# BASE_CONFIG_H_TEXT and the hunspell SOURCES must be set before BUILD_HATN_MODULE() (which calls
# CREATE_CONFIG_FILE and ADD_LIBRARY) while TARGET_LINK_LIBRARIES below needs the target, which
# BUILD_HATN_MODULE() is what creates.
#
# HATN_BASE_WITH_HUNSPELL is the single decision that drives all three -- the HATN_USE_HUNSPELL
# macro in base/config.h, src/hunspellchecker.cpp in SOURCES and libhunspell on the link line. They
# must never disagree: the macro without the source makes spellchecker.cpp reference an undefined
# HunspellChecker, which is exactly how an iOS build broke when a host (Homebrew) hunspell was found.
SET(HATN_BASE_WITH_HUNSPELL FALSE)

IF(BUILD_ANDROID OR BUILD_IOS)

    # Spell checking is not used on mobile platforms. Do not even look for hunspell: Findhunspell
    # falls back to host prefixes (/opt/homebrew, /usr/local) and could pick up a macOS library.
    MESSAGE(STATUS "hunspell spell checker disabled for mobile platforms")

ELSE()

    FIND_PACKAGE(hunspell)

    # A build script can state that it REQUIRES the spell checking backend:
    #
    #     export HATN_USE_HUNSPELL=1
    #
    # This does not (and cannot) define the macro by itself -- HATN_USE_HUNSPELL is written into
    # base/config.h below only when the library is actually found, because the backend also needs
    # hunspell's headers on the include path and libhunspell on the link line. What the variable
    # does is turn "not found" from a silent STATUS into a hard error, so a build that was supposed
    # to have spell checking never quietly ships without it. Point the find at the library with
    # HUNSPELL_ROOT (see cmake/Findhunspell.cmake).
    IF(NOT hunspell_FOUND AND DEFINED ENV{HATN_USE_HUNSPELL} AND NOT "$ENV{HATN_USE_HUNSPELL}" STREQUAL "0")
        MESSAGE(FATAL_ERROR
            "HATN_USE_HUNSPELL is set in the environment, but hunspell was not found.\n"
            "  Build it:   dep_libs=\"hunspell\" <hatn>/build/deps/desktop/build-<platform>.sh\n"
            "              (run it from the directory whose ./deps tree this build uses)\n"
            "  Or point at an existing one:  export HUNSPELL_ROOT=/path/to/deps/root-<toolchain>\n"
            "Unset HATN_USE_HUNSPELL to build without the spell checking backend.")
    ENDIF()

    IF(hunspell_FOUND)
        SET(HATN_BASE_WITH_HUNSPELL TRUE)
        SET (BASE_CONFIG_H_TEXT "${BASE_CONFIG_H_TEXT}\n#ifndef HATN_USE_HUNSPELL \n    #define HATN_USE_HUNSPELL 1\n#endif")
    ELSE()
        # STATUS, not WARNING: hunspell is opt-in at the dependency level (not in the default
        # dep_libs list, see build/deps/desktop/config.sh), so a warning here would fire on every
        # default build and train people to ignore warnings.
        MESSAGE(STATUS "hunspell NOT found, spell checking backend disabled")
    ENDIF()

ENDIF()

SET (HEADERS
    include/hatn/base/base.h
    include/hatn/base/baseerror.h
    include/hatn/base/baseerrorcodes.h
    include/hatn/base/configtreevalue.h
    include/hatn/base/configtree.h
    include/hatn/base/configtreepath.h
    include/hatn/base/configtreeio.h
    include/hatn/base/configtreejson.h
    include/hatn/base/configtreeloader.h
    include/hatn/base/configobject.h
    include/hatn/base/spellchecker.h
    include/hatn/base/stemmer.h
    include/hatn/base/languagedetector.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/base/detail/configtreevalue.ipp
    include/hatn/base/detail/configobject.ipp
)

SET (SOURCES
    src/baseerror.cpp
    src/configtree.cpp
    src/configtreepath.cpp
    src/configtreeio.cpp
    src/configtreeloader.cpp
    src/configtreejson.cpp
    src/spellchecker.cpp
    src/stemmer.cpp
    src/languagedetector.cpp
)

# Must be appended before BUILD_HATN_MODULE(): its ADD_LIBRARY consumes SOURCES/HEADERS at the
# call, so anything added afterwards is silently never compiled.
IF(HATN_BASE_WITH_HUNSPELL)
    SET (HEADERS
        ${HEADERS}
        include/hatn/base/hunspellchecker.h
    )

    SET (SOURCES
        ${SOURCES}
        src/hunspellchecker.cpp
    )
ENDIF()

BUILD_HATN_MODULE()

# See the FIND_PACKAGE(hunspell) block above -- the target this links against exists only after
# BUILD_HATN_MODULE().
IF(HATN_BASE_WITH_HUNSPELL)
    FIND_PACKAGE(hunspell)
    MESSAGE(STATUS "hunspell found, enabling spell checking backend")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE hunspell::hunspell)
ENDIF()
