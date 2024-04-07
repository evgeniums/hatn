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

#ifndef HATNBASEERRORCODES_H
#define HATNBASEERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//! Error codes of hatndataunit lib.
enum class UnitError : int
{
    OK=static_cast<int>(common::CommonError::OK),
    PARSE_ERROR=1,
    SERIALIZE_ERROR=2
};

enum class RawErrorCode : int
{
    OK=static_cast<int>(common::CommonError::OK),
    REQUIRED_FIELD_MISSING=1,
    FIELD_TYPE_MISMATCH=2,
    END_OF_STREAM=3,
    FAILED_PREPARE_BUFFER=4
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNBASEERRORCODES_H
