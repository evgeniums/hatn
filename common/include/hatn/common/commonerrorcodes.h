/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/commonerrorcodes.h
  *
  *     Contains common error codes.
  *
  */

/****************************************************************************/

#ifndef HATNCOMMONERRORCODES_H
#define HATNCOMMONERRORCODES_H

#include <hatn/common/common.h>
#include <hatn/common/errorcodes.h>

#define HATN_COMMON_ERRORS(Do) \
    Do(CommonError,OK,_TR("OK")) \
    Do(CommonError,UNKNOWN,_TR("unknown")) \
    Do(CommonError,INVALID_SIZE,_TR("invalid size")) \
    Do(CommonError,INVALID_ARGUMENT,_TR("invalid argument")) \
    Do(CommonError,UNSUPPORTED,_TR("operation not supported")) \
    Do(CommonError,INVALID_FILENAME,_TR("invalid file name")) \
    Do(CommonError,FILE_FLUSH_FAILED,_TR("failed to flush file")) \
    Do(CommonError,FILE_ALREADY_OPEN,_TR("file already open")) \
    Do(CommonError,FILE_WRITE_FAILED,_TR("failed to write file")) \
    Do(CommonError,FILE_READ_FAILED,_TR("failed to read file")) \
    Do(CommonError,FILE_NOT_OPEN,_TR("file not open")) \
    Do(CommonError,TIMEOUT,_TR("operation timeout")) \
    Do(CommonError,RESULT_ERROR,_TR("cannot get value of error result")) \
    Do(CommonError,RESULT_NOT_ERROR,_TR("cannot move not error result")) \
    Do(CommonError,NOT_IMPLEMENTED,_TR("requested operation with provided arguments not implemented yet")) \
    Do(CommonError,NOT_FOUND,_TR("not found")) \
    Do(CommonError,INVALID_TIME_FORMAT,_TR("invalid time format")) \
    Do(CommonError,INVALID_DATE_FORMAT,_TR("invalid date format")) \
    Do(CommonError,INVALID_DATETIME_FORMAT,_TR("invalid datetime format")) \
    Do(CommonError,INVALID_DATERANGE_FORMAT,_TR("invalid format of date range"))

HATN_COMMON_NAMESPACE_BEGIN

//! Common errors.
enum class CommonError : int
{
    HATN_COMMON_ERRORS(HATN_ERROR_CODE)
};

//! Common errors codes as strings.
constexpr const char* const CommonErrorStrings[] = {
    HATN_COMMON_ERRORS(HATN_ERROR_STR)
};

//! Common error code to string.
inline const char* commonErrorString(CommonError code)
{
    return errorString(code,CommonErrorStrings);
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNCOMMONERRORCODES_H
