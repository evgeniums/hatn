/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/apperrorcodes.h
  *
  * Contains error codes for hatnapp lib.
  *
  */

/****************************************************************************/

#ifndef HATNAPPERRORCODES_H
#define HATNAPPERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/app/app.h>

#define HATN_APP_ERRORS(Do) \
    Do(AppError,OK,_TR("OK")) \
    Do(AppError,UNKNOWN_LOGGER,_TR("unknown logger type","app")) \


HATN_APP_NAMESPACE_BEGIN

//! Error codes of hatnapp lib.
enum class AppError : int
{
    HATN_APP_ERRORS(HATN_ERROR_CODE)
};

//! app errors codes as strings.
constexpr const char* const AppErrorStrings[] = {
    HATN_APP_ERRORS(HATN_ERROR_STR)
};

//! App error code to string.
inline const char* appErrorString(AppError code)
{
    return errorString(code,AppErrorStrings);
}

HATN_APP_NAMESPACE_END

#endif // HATNAPPERRORCODES_H
