/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/logcontexterrorcodes.h
  *
  * Contains error codes for hatnlogcontext lib.
  *
  */

/****************************************************************************/

#ifndef HATNLOGCONTEXTERRORCODES_H
#define HATNLOGCONTEXTERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/logcontext/logcontext.h>

#define HATN_LOGCONTEXT_ERRORS(Do) \
    Do(LogContextError,OK,_TR("OK")) \
    Do(LogContextError,INVALID_DEFAULT_LOG_LEVEL,_TR("invalid default log level","logcontext")) \
    Do(LogContextError,INVALID_TAG_LOG_LEVEL,_TR("invalid tag log level","logcontext")) \
    Do(LogContextError,INVALID_MODULE_LOG_LEVEL,_TR("invalid module log level","logcontext")) \
    Do(LogContextError,INVALID_SCOPE_LOG_LEVEL,_TR("invalid scope log level","logcontext"))

HATN_LOGCONTEXT_NAMESPACE_BEGIN

//! Error codes of hatnlogcontext lib.
enum class LogContextError : int
{
    HATN_LOGCONTEXT_ERRORS(HATN_ERROR_CODE)
};

//! logcontext errors codes as strings.
constexpr const char* const LogContextErrorStrings[] = {
    HATN_LOGCONTEXT_ERRORS(HATN_ERROR_STR)
};

//! LogContext error code to string.
inline const char* logcontextErrorString(LogContextError code)
{
    return errorString(code,LogContextErrorStrings);
}

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNLOGCONTEXTERRORCODES_H
