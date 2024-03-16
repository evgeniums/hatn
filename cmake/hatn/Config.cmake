INCLUDE(CheckIncludeFileCXX)

IF (NOT DEPS_ROOT)
    IF (BUILD_IOS OR BUILD_ANDROID)
        FILE(TO_CMAKE_PATH $ENV{deps_arch} DEPS_ARCH_PATH)
        SET(DEPS_ROOT ${DEPS_ARCH_PATH} CACHE STRING "Root folder of dependencies")
    ELSE()
        FILE(TO_CMAKE_PATH $ENV{deps_root} DEPS_ROOT_PATH)
        SET(DEPS_ROOT ${DEPS_ROOT_PATH} CACHE STRING "Root folder of dependencies")
    ENDIF()
ENDIF()

MESSAGE(STATUS "Using DEPS_ROOT: ${DEPS_ROOT}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${DEPS_ROOT}/cmake ${DEPS_ROOT}/lib/cmake)
SET(HATN_INCLUDE_DIRECTORIES ${DEPS_ROOT}/include CACHE STRING "Include folder of dependencies")
SET(HATN_LINK_DIRECTORIES ${DEPS_ROOT}/lib CACHE STRING "Library folder of dependencies")

IF (NOT MSVC)
    SET_PROPERTY(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)
ENDIF()

STRING (TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
IF(${BUILD_TYPE} STREQUAL "DEBUG")
    MESSAGE(STATUS "Compiling with Debug build type")
    SET(LIB_POSTFIX d)
    SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DHATN_BUILD_DEBUG)
    SET(BUILD_TYPE_CONFIGURATION Debug)
    SET(BUILD_DEBUG TRUE)
ELSE(${BUILD_TYPE} STREQUAL "DEBUG")
    MESSAGE(STATUS "Compiling with Release build type")
    SET(BUILD_TYPE_CONFIGURATION Release)
    SET(BUILD_RELEASE TRUE)
ENDIF(${BUILD_TYPE} STREQUAL "DEBUG")

IF ($ENV{HATN_SMARTPOINTERS_STD})
    SET (HATN_SMARTPOINTERS_STD_DEFAULT $ENV{HATN_SMARTPOINTERS_STD})
ENDIF()

OPTION(BUILD_TESTS "Build tests" ON)
OPTION(BUILD_STATIC "Build static hatn libraries" OFF)
OPTION(BOOST_STATIC "Use static version of boost libraries" OFF)
OPTION(ENABLE_TRANSLATIONS "Enable translations (gettext is required)" OFF)
OPTION(STRIP_SYMBOLS "Strip symbols from release dynamic libraries or executable binaries" ON)
OPTION(INSTALL_DEV "Install development package" OFF)
OPTION(TREAT_WARNING_AS_ERROR "Treat all warnings as errors" OFF)
OPTION(ENABLE_DYNAMIC_PLUGINS_FOR_STATIC_BUILD "Enable dynamic plugins for static library" OFF)
OPTION(HATN_SMARTPOINTERS_STD "Use smartpointers from standard std library instead of hatn library" ${HATN_SMARTPOINTERS_STD_DEFAULT})

IF (BUILD_IOS)
    MESSAGE(STATUS "Building for iOS")
    SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DBUILD_IOS)
ENDIF()

IF (NOT CMAKE_CXX_STANDARD)
    IF ($ENV{CMAKE_CXX_STANDARD})
        SET (CMAKE_CXX_STANDARD $ENV{CMAKE_CXX_STANDARD})
        MESSAGE(STATUS "Using c++${CMAKE_CXX_STANDARD} standard as requested in environment")
    ELSE()
        SET (CMAKE_CXX_STANDARD 17)
        MESSAGE(STATUS "Using default c++${CMAKE_CXX_STANDARD} standard")
    ENDIF()

ENDIF()

IF (MSVC)
    MESSAGE(STATUS "Compiling with MSVC_VERSION=${MSVC_VERSION}")
    IF (MSVC_VERSION LESS 1914)
        MESSAGE(FATAL_ERROR "\n*****************************\nTo compile hatn library you need Visual Studio 2017 version 15.7 or later!\n*****************************\n")
    ENDIF()
    IF (CMAKE_CXX_STANDARD LESS 17)
        MESSAGE(FATAL_ERROR "\n*****************************\nOnly C++17 and later standards are supported for MSVC\nSet CMAKE_CXX_STANDARD=17 in build environment\n*****************************\n")
    ENDIF()
ELSE ()
    IF (CMAKE_CXX_STANDARD LESS 14)
        MESSAGE(FATAL_ERROR "\n*****************************\nOnly C++14 and later standards are supported\n*****************************\n")
    ENDIF()
ENDIF()

IF ($ENV{HATN_TEST_VALGRIND})
    SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DBUILD_VALGRIND)
    MESSAGE(STATUS "Building for Valgrind environment")
ENDIF()

IF ($ENV{HATN_TREAT_WARNING_AS_ERROR})
    SET (TREAT_WARNING_AS_ERROR $ENV{HATN_TREAT_WARNING_AS_ERROR})
ENDIF()

IF ($ENV{HATN_TEST_WRAP_C})
    SET (HATN_TEST_WRAP_C $ENV{HATN_TEST_WRAP_C})
ENDIF()

IF (APPLE AND IOS_SDK_VERSION_X10)
    MESSAGE(STATUS "Using IOS_SDK_VERSION_X10: ${IOS_SDK_VERSION_X10}")
    SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DIOS_SDK_VERSION_X10=${IOS_SDK_VERSION_X10})
ENDIF()

MESSAGE (STATUS "Using CMake generator ${CMAKE_GENERATOR}")

SET(CMAKE_DEBUG_POSTFIX d)

IF(BUILD_ANDROID)
    SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DBUILD_ANDROID)
ENDIF()

IF(BUILD_STATIC)
    SET(INSTALL_DEV ON CACHE BOOL "Install development package")
ENDIF(BUILD_STATIC)

IF(BUILD_RELEASE)
    IF (NOT MSVC)
        IF (STRIP_SYMBOLS)
            MESSAGE(STATUS "Strip symbols")
            IF (NOT (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
                SET(HATN_SHARED_LINKER_FLAGS "${HATN_SHARED_LINKER_FLAGS} -s")
                SET(HATN_EXE_LINKER_FLAGS "${HATN_EXE_LINKER_FLAGS} -s")
            ELSE()
            # for macOS use "strip -x" after building
            ENDIF()
        ENDIF ()
    ELSE()
        SET(HATN_EXE_LINKER_FLAGS "/TSAWARE /MANIFEST:NO")
    ENDIF()
ENDIF()

IF (MSVC AND BUILD_DEBUG)
    SET(HATN_COMPILE_OPTIONS "${HATN_COMPILE_OPTIONS} /bigobj")
ENDIF()

IF (BOOST_STATIC)
    SET (Boost_USE_STATIC_LIBS ON CACHE BOOL "Boost static libs")
ELSE (BOOST_STATIC)
    SET (Boost_USE_STATIC_LIBS OFF CACHE BOOL "Boost static libs")
    SET(Boost_USE_STATIC_RUNTIME OFF)
    IF(WIN32)
        SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DBOOST_ALL_NO_LIB)
    ENDIF(WIN32)
    SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DBOOST_ALL_DYN_LINK)
ENDIF (BOOST_STATIC)

IF (NOT PYTHON_EXE)
    IF (NOT "$ENV{PYTHON_EXE}" STREQUAL "")
        MESSAGE(STATUS "Using python from environment: $ENV{PYTHON_EXE}")
    	SET (PYTHON_EXE $ENV{PYTHON_EXE})
    ENDIF()
ELSE()
    MESSAGE (STATUS "Using python from command line: ${PYTHON_EXE}")
ENDIF()

IF (NOT PYTHON_EXE)
    IF(WIN32)
        FIND_PROGRAM(PYTHON_EXE python PATHS "$PATH" "${PYTHON_PATH}")
    ELSE(WIN32)
        SET(PYTHON_EXE "python")
    ENDIF(WIN32)
ENDIF (NOT PYTHON_EXE)

FIND_PROGRAM(PYTHON_CHECK ${PYTHON_EXE})
IF (NOT PYTHON_CHECK)
    MESSAGE (FATAL_ERROR "\n***************\nPython is not found (expected: ${PYTHON_EXE})\nPlease, install python and/or set PYTHON_EXE.\n***************\n")
ELSE ()
    MESSAGE (STATUS "Using python: ${PYTHON_CHECK}")
    SET(PYTHON_EXE ${PYTHON_CHECK})
ENDIF()

IF(MINGW)
    SET(HATN_COMPILE_OPTIONS ${HATN_COMPILE_OPTIONS} -U__STRICT_ANSI__)
ENDIF(MINGW)

FIND_PACKAGE(Threads REQUIRED)

SET (BOOST_MODULES system thread regex program_options date_time filesystem locale)

IF (CMAKE_CXX_STANDARD GREATER_EQUAL 17)
    CHECK_INCLUDE_FILE_CXX("memory_resource" HAVE_PMR)
ELSE()
    SET(HAVE_PMR OFF)
ENDIF()

IF (HAVE_PMR)
    MESSAGE(STATUS "Using standard pmr implementation")
ELSE()
    MESSAGE(STATUS "Using Boost.Container for pmr")
    SET (BOOST_MODULES ${BOOST_MODULES} container)
ENDIF()

IF(HATN_MASTER_PROJECT)
    SET (BOOST_MODULES ${BOOST_MODULES} unit_test_framework)
ENDIF(HATN_MASTER_PROJECT)

FIND_PACKAGE(Boost 1.65 COMPONENTS ${BOOST_MODULES} REQUIRED)
SET (HATN_LINK_DIRECTORIES
    ${HATN_LINK_DIRECTORIES}
    ${Boost_LIBRARY_DIRS}
)

SET(Boost_USE_MULTITHREADED ON)

IF(WIN32)
    SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DWIN32 -D_WIN32_WINNT=0x0600 -Dstrcasecmp=stricmp)
    IF(MSVC)
        SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
        SET(HATN_COMPILE_OPTIONS ${HATN_COMPILE_OPTIONS} -wd4251 -wd4275 -wd4996 -nologo -W3 -EHsc /wd4250 /wd4005 /wd4661 /Zc:__cplusplus /Zc:ternary)
    ENDIF(MSVC)
ENDIF(WIN32)

IF (NOT STATIC_BUILD)
    IF (NOT WIN32)
        SET(HATN_COMPILE_OPTIONS ${HATN_COMPILE_OPTIONS} -fPIC)
    ENDIF (NOT WIN32)
ENDIF (NOT STATIC_BUILD)

IF (NOT MSVC)
    SET(HATN_COMPILE_EXTRA_WARNINGS ${HATN_COMPILE_EXTRA_WARNINGS} -Wextra -Wall -Wnon-virtual-dtor)
    IF (NOT MINGW)
        SET(HATN_COMPILE_OPTIONS ${HATN_COMPILE_OPTIONS} -fstack-protector-all)
    ENDIF()
ENDIF (NOT MSVC)

IF (BUILD_DEBUG)
    SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DBUILD_DEBUG)
ENDIF()

IF ((${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
    SET(HATN_COMPILE_OPTIONS ${HATN_COMPILE_OPTIONS} -Wno-unused-function -Qunused-arguments)
ENDIF()

IF(WIN32)
    IF(NOT MSVC)
        SET(HATN_SYSTEM_LIBS ws2_32 iphlpapi uuid rpcrt4 winmm ole32)
    ELSE(NOT MSVC)
        SET(HATN_SYSTEM_LIBS)
        SET(CMAKE_INSTALL_PREFIX ${HATN_BINARY_DIR}/install)
    ENDIF(NOT MSVC)
ELSE(WIN32)
    SET(HATN_SYSTEM_LIBS dl)
    IF (LIBICONV_ROOT)
        SET (HATN_INCLUDE_DIRECTORIES
            ${HATN_INCLUDE_DIRECTORIES}
            ${LIBICONV_ROOT}/include
	)
        SET (HATN_LINK_DIRECTORIES
            ${HATN_LINK_DIRECTORIES}
            ${LIBICONV_ROOT}/lib
        )
    ENDIF(LIBICONV_ROOT)
    IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR BUILD_ANDROID)
        SET(HATN_SYSTEM_LIBS ${HATN_SYSTEM_LIBS} iconv)
    ENDIF()
ENDIF(WIN32)


IF(NOT BUILD_STATIC)
    INCLUDE(hatn/ConfigDynamic)
ELSE(NOT BUILD_STATIC)
    INCLUDE(hatn/ConfigStatic)
ENDIF(NOT BUILD_STATIC)

IF(NO_DYNAMIC_HATN_PLUGINS)
    SET(HATN_COMPILE_DEFINITIONS ${HATN_COMPILE_DEFINITIONS} -DNO_DYNAMIC_HATN_PLUGINS)
ENDIF(NO_DYNAMIC_HATN_PLUGINS)

IF (TREAT_WARNING_AS_ERROR)
    MESSAGE(STATUS "Treat warnings as errors")
    IF(MSVC)
        SET(HATN_COMPILE_OPTIONS ${HATN_COMPILE_OPTIONS} /WX)
    ELSE(MSVC)
        SET(HATN_COMPILE_OPTIONS ${HATN_COMPILE_OPTIONS} -Werror)
    ENDIF(MSVC)
ENDIF (TREAT_WARNING_AS_ERROR)

SET (HATN_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${HATN_SHARED_LINKER_FLAGS}")
SET (HATN_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${HATN_EXE_LINKER_FLAGS}")

SET (CMAKE_SHARED_LINKER_FLAGS ${HATN_SHARED_LINKER_FLAGS})
SET (CMAKE_EXE_LINKER_FLAGS ${HATN_EXE_LINKER_FLAGS})

SET (HATN_INCLUDE_DIRECTORIES
    ${HATN_INCLUDE_DIRECTORIES}
    ${Boost_INCLUDE_DIR}
)
