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
    Do(ClientServerError,ACCOUNT_CONFIG_DESERIALIZATION,_TR("failed to deserialize account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_PASSPHRASE_REQUIRED,_TR("passphrase is required to decrypt account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_DATA_DESERIALIZATION,_TR("failed to deserialize data of account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_DECRYPTION,_TR("failed to decrypt data of account configuration","clientserver")) \
    Do(ClientServerError,ACCOUNT_CONFIG_EXPIRED,_TR("account configuration expired","clientserver")) \


HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//! Error codes of hatnclientServer lib.
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
