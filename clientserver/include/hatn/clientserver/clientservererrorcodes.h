/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientserver/clientservererrorcodes.h
  *
  * Contains error codes for hatnclientserver lib.
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERERRORCODES_H
#define HATNCLIENTSERVERERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/clientserver/clientserver.h>

#define HATN_CLIENTSERVER_ERRORS(Do) \
    Do(ClientServerError,OK,_TR("OK")) \
    Do(ClientServerError,ACCOUNT_CONFIG_DESERIALIZATION,_TR("invalid format of account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_PASSPHRASE_REQUIRED,_TR("passphrase is required to decrypt account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_DATA_DESERIALIZATION,_TR("invalid format of account configuration data","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_DECRYPTION,_TR("invalid passphrase","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_EXPIRED,_TR("account configuration expired","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_INVALID_ROUTE,_TR("invalid route to server in account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_UNSUPPORTED_NETWORK_ROUTE_CHAIN,_TR("unsupported network routing chain for server in account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_UNSUPPORTED_NETWORK_PROTOCOL,_TR("unsupported network protocol for server in account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_UNSUPPORTED_AUTH_PROTOCOL,_TR("unsupported authentication protocol for server in account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_UNKNOWN_HOST,_TR("unknown network address of server in account configuration","clientserver")) \
    Do(ClientServerError,AUTH_TOKEN_TAG_INVALID,_TR("invalid authentication token tag","clientserver")) \
    Do(ClientServerError,AUTH_TOKEN_EXPIRED,_TR("authentication token expired","clientserver")) \
    Do(ClientServerError,AUTH_TOKEN_INVALID,_TR("invalid authentication token","clientserver")) \
    Do(ClientServerError,AUTH_TOKEN_INVALID_TYPE,_TR("invalid type of authentication token","clientserver")) \
    Do(ClientServerError,AUTH_SESSION_INACTIVE,_TR("authentication session is not active","clientserver")) \
    Do(ClientServerError,AUTH_SESSION_LOGINS_MISMATCH,_TR("authentication session login mismatches token login","clientserver")) \
    Do(ClientServerError,AUTH_SESSION_NOT_FOUND,_TR("authentication session not found","clientserver")) \
    Do(ClientServerError,AUTH_PROCESSING_FAILED,_TR("failed to process authentication","clientserver")) \
    Do(ClientServerError,AUTH_COMPLETION_FAILED,_TR("failed to complete authentication","clientserver")) \
    Do(ClientServerError,LOGIN_NOT_SET,_TR("login must be specified","clientserver")) \
    Do(ClientServerError,INVALID_LOGIN_FORMAT,_TR("invalid format of login","clientserver")) \
    Do(ClientServerError,LOGIN_NOT_FOUND,_TR("login not found","clientserver")) \
    Do(ClientServerError,USER_NOT_FOUND,_TR("user not found","clientserver")) \
    Do(ClientServerError,LOGIN_BLOCKED,_TR("login blocked","clientserver")) \
    Do(ClientServerError,LOGIN_EXPIRED,_TR("login expired","clientserver")) \
    Do(ClientServerError,USER_BLOCKED,_TR("user blocked","clientserver")) \
    Do(ClientServerError,INVALID_NETWORK_PROTOCOL_CONFIGURATION,_TR("invalid configuration of network protocol","clientserver")) \
    Do(ClientServerError,DATA_NOT_FOUND_OR_EXPIRED,_TR("data not found or expired","clientserver")) \
    Do(ClientServerError,USER_CANT_BE_CONTACTED,_TR("you cannot contact this user","clientserver")) \

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//! Error codes of hatnclientserver lib.
enum class ClientServerError : int
{
    HATN_CLIENTSERVER_ERRORS(HATN_ERROR_CODE)
};

//! clientServer errors codes as strings.
constexpr const char* const ClientServerErrorStrings[] = {
    HATN_CLIENTSERVER_ERRORS(HATN_ERROR_STR)
};

//! ClientServer error code to string.
inline const char* clientServerErrorString(ClientServerError code)
{
    return errorString(code,ClientServerErrorStrings);
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERERRORCODES_H
