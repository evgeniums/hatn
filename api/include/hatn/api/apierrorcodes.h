/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/apierrorcodes.h
  *
  * Contains error codes for hatnapi lib.
  *
  */

/****************************************************************************/

#ifndef HATNAPIERRORCODES_H
#define HATNAPIERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/api/api.h>

#define HATN_API_ERRORS(Do) \
    Do(ApiLibError,OK,_TR("OK")) \
    Do(ApiLibError,QUEUE_OVERFLOW,_TR("Too many requests in a queue")) \
    Do(ApiLibError,CONNECTION_BUSY,_TR("Invalid operation on busy connection")) \
    Do(ApiLibError,TOO_BIG_TX_MESSAGE,_TR("Sending message size to big")) \
    Do(ApiLibError,TOO_BIG_RX_MESSAGE,_TR("Receivng message size to big")) \
    Do(ApiLibError,CONNECTION_NOT_READY_RECV,_TR("Connection not ready to receive data")) \

HATN_API_NAMESPACE_BEGIN

//! Error codes of hatnapi lib.
enum class ApiLibError : int
{
    HATN_API_ERRORS(HATN_ERROR_CODE)
};

//! base errors codes as strings.
constexpr const char* const ApiErrorStrings[] = {
    HATN_API_ERRORS(HATN_ERROR_STR)
};

//! Base error code to string.
inline const char* apiErrorString(ApiLibError code)
{
    return errorString(code,ApiErrorStrings);
}

HATN_API_NAMESPACE_END

#endif // HATNAPIERRORCODES_H
