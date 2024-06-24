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
    Do(UnitError,SERIALIZE_ERROR,_TR("failed to serialize object"))

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
    REQUIRED_FIELD_MISSING=1,
    FIELD_TYPE_MISMATCH=2,
    END_OF_STREAM=3,
    FAILED_PREPARE_BUFFER=4,
    INVALID_TAG=5,
    FIELD_PARSING_FAILED=6,
    UNTERMINATED_VARINT=7,
    INVALID_BLOCK_LENGTH=8,
    UNKNOWN_FIELD_TYPE=9,
    FIELD_SERIALIZING_FAILED=10,
    SUSPECT_OVERFLOW=11,
    JSON_PARSE_ERROR=12,
    JSON_FIELD_SERIALIZE_ERROR=13
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITERRORCODES_H
