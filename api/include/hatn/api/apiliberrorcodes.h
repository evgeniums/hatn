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
    Do(ApiLibError,TOO_BIG_TX_MESSAGE,_TR("sending message size too big","api")) \
    Do(ApiLibError,TOO_BIG_RX_MESSAGE,_TR("receivng message size too big","api")) \
    Do(ApiLibError,CONNECTION_NOT_READY_RECV,_TR("connection not ready to receive data","api")) \
    Do(ApiLibError,FAILED_SERIALIZE_REQUEST,_TR("failed to serialize request","api")) \
    Do(ApiLibError,FAILED_DESERIALIZE_RESPONSE,_TR("failed to deserialize response","api")) \
    Do(ApiLibError,FAILED_DESERIALIZE_RESPONSE_ERROR,_TR("failed to deserialize response error","api")) \
    Do(ApiLibError,SERVER_RESPONDED_WITH_ERROR,_TR("server responded with error","api")) \
    Do(ApiLibError,SERVER_CLOSED,_TR("server closed during request","api")) \
    Do(ApiLibError,CONNECTION_CLOSED,_TR("connection was closed during request","api")) \
    Do(ApiLibError,FORCE_CONNECTION_CLOSE,_TR("connection was forced to close","api")) \
    Do(ApiLibError,UNKNOWN_BRIDGE_SERVICE,_TR("unknown bridge service","api")) \
    Do(ApiLibError,UNKNOWN_BRIDGE_METHOD,_TR("unknown bridge method","api")) \
    Do(ApiLibError,UNKNOWN_MICROSERVICE_TYPE,_TR("unknown microservice type","api")) \
    Do(ApiLibError,MICROSERVICES_CONFIG_SECTION_MISSING,_TR("microservices section missing in configuration tree","api")) \
    Do(ApiLibError,MICROSERVICES_CONFIG_SECTION_INVALID,_TR("invalid type of microservices section in configuration tree","api")) \
    Do(ApiLibError,MICROSERVICE_CONFIG_INVALID,_TR("invalid microservice configuration","api")) \
    Do(ApiLibError,MICROSERVICE_CREATE_FAILED,_TR("failed to create microservice","api")) \
    Do(ApiLibError,MICROSERVICE_RUN_FAILED,_TR("failed to run microservice","api")) \
    Do(ApiLibError,DUPLICATE_MICROSERVICE,_TR("duplicate microservice","api")) \


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
