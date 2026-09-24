#[[
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
]]

#[[
    Findhunspell.cmake -- task-spellcheck.md.

    Upstream hunspell installs a pkg-config file (hunspell.pc) but NO CMake package config, so a
    bare FIND_PACKAGE(hunspell) fails even against a correctly built/installed hunspell -- unlike
    e.g. utf8proc, which ships its own <name>Config.cmake and needs no Find module at all. This
    module fills that one gap: find_path/find_library against the usual locations, then an
    IMPORTED target so callers (base/CMakeLists.txt) never see the raw variables.

    Searched, in order: HUNSPELL_ROOT (cache variable or ENVIRONMENT variable -- see below),
    DEPS_ROOT (this project's own dependency build -- build/deps/desktop/libs/hunspell.sh installs
    there), then the common system/package-manager prefixes, so a locally `brew install hunspell`
    also resolves for a developer who has not run the dependency script.

    HUNSPELL_ROOT exists so a build script can point at a hunspell that was built into a DIFFERENT
    dependency prefix than the one this build otherwise uses, without rebuilding it or copying it
    around:

        export HUNSPELL_ROOT=/path/to/deps/root-clang        # <root>/include + <root>/lib

    (CMake also searches the CMAKE_PREFIX_PATH environment variable and a hunspell_ROOT
    cache/env variable on its own, per policy CMP0074 -- both work too. HUNSPELL_ROOT is spelled
    out here because it is the one that reads unambiguously in a shell script.)

    NOTE for anyone reaching for HATN_USE_HUNSPELL directly: that macro is an OUTPUT of this
    module succeeding (base/CMakeLists.txt writes it into base/config.h), never an input. Defining
    it by hand -- in the environment, or via CXXFLAGS -- does not enable the backend: it makes
    src/hunspellchecker.cpp's body live while its #include <hunspell/hunspell.hxx> is still not on
    the include path, and the TARGET_LINK_LIBRARIES that puts libhunspell on the link line lives
    inside base/CMakeLists.txt's IF(hunspell_FOUND) block. Point this module at the library
    instead; see also $ENV{HATN_USE_HUNSPELL} in base/CMakeLists.txt, which turns a failed find
    into a hard error rather than a silently-disabled backend.

    Result:
      hunspell_FOUND        -- TRUE/FALSE
      hunspell::hunspell     -- imported target, INTERFACE_INCLUDE_DIRECTORIES +
                                 INTERFACE_LINK_LIBRARIES already set
]]

FIND_PATH(HUNSPELL_INCLUDE_DIR
    NAMES hunspell/hunspell.hxx
    HINTS
        ${HUNSPELL_ROOT}/include
        $ENV{HUNSPELL_ROOT}/include
        ${DEPS_ROOT}/include
        ${HATN_INCLUDE_DIRECTORIES}
    PATHS
        /opt/homebrew/include
        /usr/local/include
        /usr/include
)

FIND_LIBRARY(HUNSPELL_LIBRARY
    NAMES hunspell hunspell-1.7 libhunspell
    HINTS
        ${HUNSPELL_ROOT}/lib
        $ENV{HUNSPELL_ROOT}/lib
        ${DEPS_ROOT}/lib
        ${HATN_LINK_DIRECTORIES}
    PATHS
        /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(hunspell
    REQUIRED_VARS HUNSPELL_LIBRARY HUNSPELL_INCLUDE_DIR
)

IF (hunspell_FOUND AND NOT TARGET hunspell::hunspell)
    ADD_LIBRARY(hunspell::hunspell UNKNOWN IMPORTED)
    SET_TARGET_PROPERTIES(hunspell::hunspell PROPERTIES
        IMPORTED_LOCATION "${HUNSPELL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${HUNSPELL_INCLUDE_DIR}"
        # Static Windows builds must define this before including hunspell.hxx, or its
        # LIBHUNSPELL_DLL_EXPORTED macro resolves to a dllimport that never gets satisfied --
        # see hunvisapi.h.in / the CMake shim in build/deps/desktop/libs/hunspell-CMakeLists.txt.
        INTERFACE_COMPILE_DEFINITIONS "HUNSPELL_STATIC"
    )
ENDIF()

MARK_AS_ADVANCED(HUNSPELL_INCLUDE_DIR HUNSPELL_LIBRARY)
