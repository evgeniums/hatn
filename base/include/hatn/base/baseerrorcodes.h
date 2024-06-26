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

HATN_BASE_NAMESPACE_BEGIN

//! Error codes of hatnbase lib.
enum class BaseError : int
{
    OK=static_cast<int>(common::CommonError::OK),
    INVALID_TYPE,
    UNSUPPORTED_TYPE,
    VALUE_NOT_SET,
    STRING_NOT_NUMBER,
    UNSUPPORTED_CONFIG_FORMAT,
    CONFIG_PARSE_ERROR,
    UNKKNOWN_CONFIG_MERGE_MODE,
    CONFIG_LOAD_ERROR,
    CONFIG_SAVE_ERROR,
    CONFIG_OBJECT_LOAD_ERROR,
    CONFIG_OBJECT_VALIDATE_ERROR
};

HATN_BASE_NAMESPACE_END

#endif // HATNBASEERRORCODES_H
