/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientapperrorcodes.h
  *
  * Contains error codes for hatnclientapp lib.
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTAPPERRORCODES_H
#define HATNCLIENTAPPERRORCODES_H

#include <hatn/common/error.h>

#include <hatn/clientapp/clientappdefs.h>

#define HATN_CLIENTAPP_ERRORS(Do) \
    Do(ClientAppError,OK,_TR("OK")) \
    Do(ClientAppError,UNKNOWN_BRIDGE_SERVICE,_TR("unknown bridge service","clientapp")) \
    Do(ClientAppError,UNKNOWN_BRIDGE_METHOD,_TR("unknown bridge method","clientapp")) \
    Do(ClientAppError,UNKNOWN_BRIDGE_MESSASGE,_TR("unknown bridge message type","clientapp")) \
    Do(ClientAppError,FAILED_PARSE_BRIDGE_JSON,_TR("failed to parse JSON message of bridge request","clientapp"))

HATN_CLIENTAPP_NAMESPACE_BEGIN

//! Error codes of hatnapi lib.
enum class ClientAppError : int
{
    HATN_CLIENTAPP_ERRORS(HATN_ERROR_CODE)
};

//! API errors codes as strings.
constexpr const char* const ClientAppErrorStrings[] = {
    HATN_CLIENTAPP_ERRORS(HATN_ERROR_STR)
};

//! API error code to string.
inline const char* clientAppErrorString(ClientAppError code)
{
    return errorString(code,ClientAppErrorStrings);
}

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTAPPERRORCODES_H
