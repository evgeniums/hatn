/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/datauniterrorcodes.h
  *
  * Contains error codes for hatndataunit lib.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITERRORCODES_H
#define HATNDATAUNITERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/dataunit/dataunit.h>

#define HATN_DATAUNIT_ERRORS(Do) \
    Do(UnitError,OK,_TR("OK")) \
    Do(UnitError,PARSE_ERROR,_TR("failed to parse object","dataunit")) \
    Do(UnitError,SERIALIZE_ERROR,_TR("failed to serialize object","dataunit")) \
    Do(UnitError,JSON_PARSE_ERROR,_TR("failed to parse JSON","dataunit")) \

HATN_DATAUNIT_NAMESPACE_BEGIN

//! Error codes of hatndataunit lib.
enum class UnitError : int
{
    HATN_DATAUNIT_ERRORS(HATN_ERROR_CODE)
};

//! Unit errors codes as strings.
constexpr const char* const UnitErrorStrings[] = {
    HATN_DATAUNIT_ERRORS(HATN_ERROR_STR)
};

//! Unit error code to string.
inline const char* unitErrorString(UnitError code)
{
    return errorString(code,UnitErrorStrings);
}

enum class RawErrorCode : int
{
    OK=static_cast<int>(common::CommonError::OK),
    REQUIRED_FIELD_MISSING,
    FIELD_TYPE_MISMATCH,
    END_OF_STREAM,
    FAILED_PREPARE_BUFFER,
    INVALID_TAG,
    FIELD_PARSING_FAILED,
    UNTERMINATED_VARINT,
    INVALID_BLOCK_LENGTH,
    UNKNOWN_FIELD_TYPE,
    FIELD_SERIALIZING_FAILED,
    SUSPECT_OVERFLOW,
    JSON_PARSE_ERROR,
    JSON_FIELD_SERIALIZE_ERROR
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITERRORCODES_H
