/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/apiliberrorcodes.h
  *
  * Contains error codes for hatnapi lib.
  *
  */

/****************************************************************************/

#ifndef HATNAPILIBERRORCODES_H
#define HATNAPILIBERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/api/api.h>

#define HATN_API_ERRORS(Do) \
    Do(ApiLibError,OK,_TR("OK")) \
    Do(ApiLibError,QUEUE_OVERFLOW,_TR("too many requests in a queue","api")) \
    Do(ApiLibError,CONNECTION_BUSY,_TR("invalid operation on busy connection","api")) \
    Do(ApiLibError,TOO_BIG_TX_MESSAGE,_TR("sending message size to big","api")) \
    Do(ApiLibError,TOO_BIG_RX_MESSAGE,_TR("receivng message size to big","api")) \
    Do(ApiLibError,CONNECTION_NOT_READY_RECV,_TR("connection not ready to receive data","api")) \
    Do(ApiLibError,FAILED_SERIALIZE_REQUEST,_TR("failed to serialize request","api"))

HATN_API_NAMESPACE_BEGIN

//! Error codes of hatnapi lib.
enum class ApiLibError : int
{
    HATN_API_ERRORS(HATN_ERROR_CODE)
};

//! API errors codes as strings.
constexpr const char* const ApiErrorStrings[] = {
    HATN_API_ERRORS(HATN_ERROR_STR)
};

//! API error code to string.
inline const char* apiErrorString(ApiLibError code)
{
    return errorString(code,ApiErrorStrings);
}

HATN_API_NAMESPACE_END

#endif // HATNAPILIBERRORCODES_H
