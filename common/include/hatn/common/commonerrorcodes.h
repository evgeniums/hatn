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
    Do(CommonError,OK,_TR("OK","common")) \
    Do(CommonError,UNKNOWN,_TR("unknown","common")) \
    Do(CommonError,INVALID_SIZE,_TR("invalid size","common")) \
    Do(CommonError,INVALID_ARGUMENT,_TR("invalid argument","common")) \
    Do(CommonError,UNSUPPORTED,_TR("operation not supported","common")) \
    Do(CommonError,INVALID_FILENAME,_TR("invalid file name","common")) \
    Do(CommonError,FILE_FLUSH_FAILED,_TR("failed to flush file","common")) \
    Do(CommonError,FILE_ALREADY_OPEN,_TR("file already open","common")) \
    Do(CommonError,FILE_WRITE_FAILED,_TR("failed to write file","common")) \
    Do(CommonError,FILE_READ_FAILED,_TR("failed to read file","common")) \
    Do(CommonError,FILE_NOT_OPEN,_TR("file not open","common")) \
    Do(CommonError,FILE_NOT_FOUND,_TR("file not found","common")) \
    Do(CommonError,FILE_SYNC_FAILED,_TR("failed to sync file","common")) \
    Do(CommonError,FILE_FSYNC_FAILED,_TR("failed to fsync file","common")) \
    Do(CommonError,FILE_TRUNCATE_FAILED,_TR("failed to truncate file","common")) \
    Do(CommonError,TIMEOUT,_TR("operation timeout","common")) \
    Do(CommonError,ABORTED,_TR("operation aborted","common")) \
    Do(CommonError,RESULT_ERROR,_TR("cannot get value of error result","common")) \
    Do(CommonError,RESULT_NOT_ERROR,_TR("cannot move not error result","common")) \
    Do(CommonError,NOT_IMPLEMENTED,_TR("requested operation with provided arguments not implemented yet","common")) \
    Do(CommonError,NOT_FOUND,_TR("not found","common")) \
    Do(CommonError,INVALID_TIME_FORMAT,_TR("invalid time format","common")) \
    Do(CommonError,INVALID_DATE_FORMAT,_TR("invalid date format","common")) \
    Do(CommonError,INVALID_DATETIME_FORMAT,_TR("invalid datetime format","common")) \
    Do(CommonError,INVALID_DATERANGE_FORMAT,_TR("invalid format of date range","common")) \
    Do(CommonError,INVALID_COMMAND_LINE_ARGUMENTS,_TR("invalid command line arguments","common")) \
    Do(CommonError,VALIDATION_ERROR,_TR("validation error","common")) \
    Do(CommonError,INVALID_FORMAT,_TR("invalid data format","common")) \
    Do(CommonError,GENERIC_ERROR,_TR("generic error","common")) \

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
