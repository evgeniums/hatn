/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/common.h
  *
  *  Export definitions of Common Library.
  *
  */

/****************************************************************************/

#ifndef HATNCOMMON_H
#define HATNCOMMON_H

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#ifdef _MSC_VER
#define NOMINMAX
#endif

#include <hatn/common/config.h>

// define export symbols for windows platform
#ifndef HATN_COMMON_EXPORT
#  if defined(WIN32)
#        ifdef BUILD_HATN_COMMON
#            define HATN_COMMON_EXPORT __declspec(dllexport)
#        else
#            define HATN_COMMON_EXPORT __declspec(dllimport)
#        endif
#  else
#    define HATN_COMMON_EXPORT
#  endif
#endif

#ifdef __GNUC__

    #define HATN_IGNORE_UNUSED_FUNCTION_BEGIN \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wunused-function\"")

    #define HATN_IGNORE_UNUSED_FUNCTION_END \
        _Pragma("GCC diagnostic pop")

    #define HATN_IGNORE_UNUSED_VARIABLE_BEGIN \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")

    #define HATN_IGNORE_UNUSED_VARIABLE_END \
        _Pragma("GCC diagnostic pop")

    #define HATN_IGNORE_UNUSED_CONST_VARIABLE_BEGIN \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wunused-const-variable\"")

    #define HATN_IGNORE_UNUSED_CONST_VARIABLE_END \
        _Pragma("GCC diagnostic pop")

#else

    #define HATN_IGNORE_UNUSED_FUNCTION_BEGIN
    #define HATN_IGNORE_UNUSED_FUNCTION_END
    #define HATN_IGNORE_UNUSED_VARIABLE_BEGIN
    #define HATN_IGNORE_UNUSED_VARIABLE_END
    #define HATN_IGNORE_UNUSED_CONST_VARIABLE_BEGIN
    #define HATN_IGNORE_UNUSED_CONST_VARIABLE_END

#endif

#define HATN_COMMON_NAMESPACE_BEGIN namespace hatn { namespace common {
#define HATN_COMMON_NAMESPACE_END }}

#define HATN_COMMON_NAMESPACE hatn::common
#define HATN_COMMON_USING using namespace hatn::common;
#define HATN_COMMON_NS common

#define HATN_NAMESPACE hatn
#define HATN_NAMESPACE_BEGIN namespace hatn {
#define HATN_NAMESPACE_END }
#define HATN_USING using namespace hatn;

#define HATN_TEST_NAMESPACE hatn::test
#define HATN_TEST_NAMESPACE_BEGIN namespace hatn { namespace test {
#define HATN_TEST_NAMESPACE_END }}

#define HATN_TEST_USING using namespace hatn::test;

#endif // HATNCOMMON_H
