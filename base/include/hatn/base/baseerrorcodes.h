/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/baseerrorcodes.h
  *
  * Contains error codes for hatnbase lib.
  *
  */

/****************************************************************************/

#ifndef HATNBASEERRORCODES_H
#define HATNBASEERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/base/base.h>

#define HATN_BASE_ERRORS(Do) \
    Do(BaseError,OK,_TR("OK")) \
    Do(BaseError,INVALID_TYPE,_TR("invalid type","base")) \
    Do(BaseError,UNSUPPORTED_TYPE,_TR("unsupported type","base")) \
    Do(BaseError,VALUE_NOT_SET,_TR("value not set","base")) \
    Do(BaseError,STRING_NOT_NUMBER,_TR("cannot convert string to number","base")) \
    Do(BaseError,UNSUPPORTED_CONFIG_FORMAT,_TR("configuration format not supported","base")) \
    Do(BaseError,CONFIG_PARSE_ERROR,_TR("failed to parse configuration file","base")) \
    Do(BaseError,UNKKNOWN_CONFIG_MERGE_MODE,_TR("unknown merge mode","base")) \
    Do(BaseError,CONFIG_LOAD_ERROR,_TR("failed to load configuration file","base")) \
    Do(BaseError,CONFIG_SAVE_ERROR,_TR("failed to save configuration file","base")) \
    Do(BaseError,CONFIG_OBJECT_LOAD_ERROR,_TR("failed to load configuration object","base")) \
    Do(BaseError,CONFIG_OBJECT_VALIDATE_ERROR,_TR("failed to validate configuration object","base"))

HATN_BASE_NAMESPACE_BEGIN

//! Error codes of hatnbase lib.
enum class BaseError : int
{
    HATN_BASE_ERRORS(HATN_ERROR_CODE)
};

//! base errors codes as strings.
constexpr const char* const BaseErrorStrings[] = {
    HATN_BASE_ERRORS(HATN_ERROR_STR)
};

//! Base error code to string.
inline const char* baseErrorString(BaseError code)
{
    return errorString(code,BaseErrorStrings);
}

HATN_BASE_NAMESPACE_END

#endif // HATNBASEERRORCODES_H
