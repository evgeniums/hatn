/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/networkerrorcodes.h
  *
  *  Error classes for Hatn Network Library.
  *
  */

/****************************************************************************/

#ifndef HATNNETWORKERRORCODES_H
#define HATNNETWORKERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/network/network.h>

#define HATN_NETWORK_ERRORS(Do) \
    Do(NetworkError,OK,_TR("OK")) \
    Do(NetworkError,NOT_APPLICABLE,_TR("not applicable","network")) \
    Do(NetworkError,NOT_SUPPORTED,_TR("not supported","network")) \
    Do(NetworkError,OPEN_FAILED,_TR("failed to open socket","network")) \
    Do(NetworkError,ADDRESS_IN_USE,_TR("address already in use","network")) \
    Do(NetworkError,CONNECT_FAILED,_TR("failed to connect","network")) \
    Do(NetworkError,PROXY_AUTH_FAILED,_TR("SOCKS5 proxy authorization error: invalid login ot password","network")) \
    Do(NetworkError,PROXY_UNSUPPORTED_VERSION,_TR("SOCKS5 unsupported version","network")) \
    Do(NetworkError,PROXY_UNSUPPORTED_AUTH_METHOD,_TR("SOCKS5 unsupported authorization method","network")) \
    Do(NetworkError,PROXY_REPORTED_ERROR,_TR("SOCKS5 proxy server returned error status","network")) \
    Do(NetworkError,PROXY_UNSUPPORTED_IP_PROTOCOL,_TR("SOCKS5 unsupported IP protocol","network")) \
    Do(NetworkError,PROXY_MAILFORMED_DATA,_TR("SOCKS5 data mailformed","network")) \
    Do(NetworkError,PROXY_INVALID_PARAMETERS,_TR("SOCKS5 invalid parameters of proxy server","network")) \
    Do(NetworkError,DNS_FAILED,_TR("failed to resolve DNS address","network"))
//! @note DNS_FAILED is intentionally the last error code.

HATN_NETWORK_NAMESPACE_BEGIN

//! Error codes of of hatnnetwork lib.
enum class NetworkError : int
{
    HATN_NETWORK_ERRORS(HATN_ERROR_CODE)
};

//! Network errors codes as strings.
constexpr const char* const NetworkErrorStrings[] = {
    HATN_NETWORK_ERRORS(HATN_ERROR_STR)
};

//! Network error code to string.
inline const char* networkErrorString(NetworkError code)
{
    return errorString(code,NetworkErrorStrings);
}

HATN_NETWORK_NAMESPACE_END

#endif // HATNNETWORKERRORCODES_H
