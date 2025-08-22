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
    Do(ClientAppError,UNKNOWN_BRIDGE_MESSAGE,_TR("unknown bridge message type","clientapp")) \
    Do(ClientAppError,BRIDGE_MESSASGE_TYPE_MISMATCH,_TR("mismatching bridge message type","clientapp")) \
    Do(ClientAppError,FAILED_PARSE_BRIDGE_JSON,_TR("failed to parse JSON message of bridge request","clientapp")) \
    Do(ClientAppError,CIPHER_SUITES_UNDEFINED,_TR("cipher suites undefined","clientapp")) \
    Do(ClientAppError,DEFAULT_CIPHER_SUITES_UNDEFINED,_TR("default cipher suites undefined","clientapp")) \
    Do(ClientAppError,STORAGE_KEYS_REQUIRED,hatn::_TR("storage keys required","clientapp")) \
    Do(ClientAppError,STORAGE_NOTIFICATION_KEY_REQUIRED,hatn::_TR("notifications storage key required","clientapp")) \
    Do(ClientAppError,ENCRYPTION_KEY_NOT_FOUND,_TR("encryption key not found","clientapp")) \
    Do(ClientAppError,APPLICATION_SETTING_NOT_SET,_TR("application setting not set","clientapp")) \
    Do(ClientAppError,APPLICATION_CONFIG_NOT_SET,_TR("application configuration parameter not set","clientapp")) \
    Do(ClientAppError,CONFIRMATION_REQUIRED,_TR("confirmation required","clientapp")) \
    Do(ClientAppError,CONFIRMATION_FAILED,_TR("confirmation failed","clientapp")) \

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
