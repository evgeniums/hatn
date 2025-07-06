/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/auth/authprotocol.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERAUTHPROTOCOL_H
#define HATNCLIENTSERVERAUTHPROTOCOL_H

#include <hatn/api/authunit.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const uint32_t AuthServiceVersion=1;
constexpr const char* AuthServiceName="auth";
constexpr const char* AuthNegotiateMethodName="negotiate";
constexpr const char* AuthRefreshMethodName="refresh";
constexpr const char* AuthHssCheckMethodName="hss_check";

HDU_UNIT(auth_negotiate_request,
    HDU_FIELD(login,TYPE_STRING,1,true)
    HDU_FIELD(topic,TYPE_STRING,2)
    HDU_REPEATED_FIELD(protocols,HATN_API_NAMESPACE::auth_protocol::TYPE,3)
    HDU_REPEATED_FIELD(session_auth,HATN_API_NAMESPACE::auth_protocol::TYPE,4)
)

HDU_UNIT(auth_protocol_response,
    HDU_FIELD(token,TYPE_BYTES,1)
    HDU_FIELD(message_type,TYPE_STRING,2)
    HDU_FIELD(message,TYPE_DATAUNIT,3)
)

HDU_UNIT_WITH(auth_negotiate_response,(HDU_BASE(auth_protocol_response)),
    HDU_FIELD(protocol,HATN_API_NAMESPACE::auth_protocol::TYPE,10)
    HDU_FIELD(session_auth,HATN_API_NAMESPACE::auth_protocol::TYPE,11)
)

constexpr const char* AUTH_PROTOCOL_HATN_SHARED_SECRET="hss";
constexpr static const uint32_t AUTH_PROTOCOL_HATN_SHARED_SECRET_VERSION=1;

//! @todo Negotiate cipher suites

HDU_UNIT(auth_hss_challenge,
    HDU_FIELD(challenge,TYPE_BYTES,1)
    HDU_FIELD(expire,TYPE_DATETIME,2)
    HDU_FIELD(cipher_suite,TYPE_STRING,3)
)

HDU_UNIT(auth_hss_check,
    HDU_FIELD(token,TYPE_BYTES,1,true)
    HDU_FIELD(mac,TYPE_BYTES,2,true)
)

HDU_UNIT(auth_with_token,
    HDU_FIELD(token,TYPE_BYTES,1)
    HDU_FIELD(tag,TYPE_STRING,2)
)

HDU_UNIT_WITH(auth_token,(HDU_BASE(auth_with_token)),
    HDU_FIELD(expire,TYPE_DATETIME,3)
)

HDU_UNIT(auth_complete,
    HDU_FIELD(session_token,auth_token::TYPE,1)
    HDU_FIELD(refresh_token,auth_token::TYPE,2)
)

HDU_UNIT(auth_refresh,
    HDU_FIELD(token,auth_with_token::TYPE,1)
)

inline bool isAuthTokenExpired(const auth_token::shared_managed* token)
{
    auto now=common::DateTime::currentUtc();
    return now.after(token->fieldValue(auth_token::expire));
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERAUTHPROTOCOL_H
