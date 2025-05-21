/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file utility/utilityerrorcodes.h
  *
  * Contains error codes for hatnutility lib.
  *
  */

/****************************************************************************/

#ifndef HATNUTILITYERRORCODES_H
#define HATNUTILITYERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/utility/utility.h>

#define HATN_UTILITY_ERRORS(Do) \
    Do(UtilityError,OK,_TR("OK")) \
    Do(UtilityError,OPERATION_FORBIDDEN,_TR("operation forbidden by rules of access control")) \
    Do(UtilityError,MAC_FORBIDDEN,_TR("operation forbidden by mandatory access policy")) \
    Do(UtilityError,UNKNOWN_ROLE,_TR("unknown role")) \

HATN_UTILITY_NAMESPACE_BEGIN

//! Error codes of hatnutility lib.
enum class UtilityError : int
{
    HATN_UTILITY_ERRORS(HATN_ERROR_CODE)
};

//! utility errors codes as strings.
constexpr const char* const UtilityErrorStrings[] = {
    HATN_UTILITY_ERRORS(HATN_ERROR_STR)
};

//! Utility error code to string.
inline const char* utilityErrorString(UtilityError code)
{
    return errorString(code,UtilityErrorStrings);
}

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYERRORCODES_H
