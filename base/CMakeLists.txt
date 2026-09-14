PROJECT(hatnbase)

# task-spellcheck.md: optional hunspell-backed spell checking. Non-REQUIRED, same shape as
# common/CMakeLists.txt's own FIND_PACKAGE(utf8proc) -- consulted TWICE, because
# BASE_CONFIG_H_TEXT must be set before BUILD_HATN_MODULE() (which calls CREATE_CONFIG_FILE) while
# TARGET_LINK_LIBRARIES below needs the target, which BUILD_HATN_MODULE() is what creates.
FIND_PACKAGE(hunspell)

# A build script can state that it REQUIRES the spell checking backend:
#
#     export HATN_USE_HUNSPELL=1
#
# This does not (and cannot) define the macro by itself -- HATN_USE_HUNSPELL is written into
# base/config.h below only when the library is actually found, because the backend also needs
# hunspell's headers on the include path and libhunspell on the link line. What the variable does
# is turn "not found" from a silent STATUS into a hard error, so a build that was supposed to have
# spell checking never quietly ships without it. Point the find at the library with HUNSPELL_ROOT
# (see cmake/Findhunspell.cmake).
IF(NOT hunspell_FOUND AND DEFINED ENV{HATN_USE_HUNSPELL} AND NOT "$ENV{HATN_USE_HUNSPELL}" STREQUAL "0")
    MESSAGE(FATAL_ERROR
        "HATN_USE_HUNSPELL is set in the environment, but hunspell was not found.\n"
        "  Build it:   dep_libs=\"hunspell\" <hatn>/build/deps/desktop/build-<platform>.sh\n"
        "              (run it from the directory whose ./deps tree this build uses)\n"
        "  Or point at an existing one:  export HUNSPELL_ROOT=/path/to/deps/root-<toolchain>\n"
        "Unset HATN_USE_HUNSPELL to build without the spell checking backend.")
ENDIF()

IF(hunspell_FOUND)
    SET (BASE_CONFIG_H_TEXT "${BASE_CONFIG_H_TEXT}\n#ifndef HATN_USE_HUNSPELL \n    #define HATN_USE_HUNSPELL 1\n#endif")
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
    include/hatn/base/hunspellchecker.h
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
    src/hunspellchecker.cpp
)

BUILD_HATN_MODULE()

# See the FIND_PACKAGE(hunspell) block above -- the target this links against exists only after
# BUILD_HATN_MODULE().
FIND_PACKAGE(hunspell)

IF(hunspell_FOUND)
    MESSAGE(STATUS "hunspell found, enabling spell checking backend")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE hunspell::hunspell)
ELSE()
    # STATUS, not WARNING: hunspell is opt-in at the dependency level (not in the default
    # dep_libs list, see build/deps/desktop/config.sh), so a warning here would fire on every
    # default build and train people to ignore warnings.
    MESSAGE(STATUS "hunspell NOT found, spell checking backend disabled")
ENDIF()
