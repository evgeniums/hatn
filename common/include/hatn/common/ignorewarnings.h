/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/ignorewarnings.h
  *
  *  Ignore some GCC warnings.
  *
  */

/****************************************************************************/

#ifndef HATNIGNOREWARNINGS_H
#define HATNIGNOREWARNINGS_H

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

    #if defined(__clang__)

    #define HATN_IGNORE_STRING_LITERAL_BEGIN \
    _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wgnu-string-literal-operator-template\"")

    #define HATN_IGNORE_STRING_LITERAL_END \
        _Pragma("GCC diagnostic pop")

    #define HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_BEGIN \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Winstantiation-after-specialization\"")

    #define HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_END \
        _Pragma("GCC diagnostic pop")

    #else

    #define HATN_IGNORE_STRING_LITERAL_BEGIN
    #define HATN_IGNORE_STRING_LITERAL_END

    #define HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_BEGIN
    #define HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_END

    #endif

    #define HATN_IGNORE_UNINITIALIZED_BEGIN \
    _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")

    #define HATN_IGNORE_UNINITIALIZED_END \
        _Pragma("GCC diagnostic pop")

#else

    #define HATN_IGNORE_UNUSED_FUNCTION_BEGIN
    #define HATN_IGNORE_UNUSED_FUNCTION_END
    #define HATN_IGNORE_UNUSED_VARIABLE_BEGIN
    #define HATN_IGNORE_UNUSED_VARIABLE_END
    #define HATN_IGNORE_UNUSED_CONST_VARIABLE_BEGIN
    #define HATN_IGNORE_UNUSED_CONST_VARIABLE_END
    #define HATN_IGNORE_STRING_LITERAL_BEGIN
    #define HATN_IGNORE_STRING_LITERAL_END
    #define HATN_IGNORE_UNINITIALIZED_BEGIN
    #define HATN_IGNORE_UNINITIALIZED_END
    #define HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_BEGIN
    #define HATN_IGNORE_INSTANTIATION_AFTER_SPECIALIZATION_END

#endif

#endif // HATNIGNOREWARNINGS_H
