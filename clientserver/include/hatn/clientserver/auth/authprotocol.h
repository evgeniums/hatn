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

HDU_UNIT(auth_negotiate_request,
    HDU_REPEATED_FIELD(subject_path,TYPE_STRING,1)
    HDU_REPEATED_FIELD(protocols,HATN_API_NAMESPACE::auth_protocol::TYPE,2)
)

HDU_UNIT(auth_protocol_response,
    HDU_FIELD(token,TYPE_BYTES,1)
    HDU_FIELD(content,TYPE_DATAUNIT,2)
)

HDU_UNIT_WITH(auth_negotiate_response,(HDU_BASE(auth_protocol_response)),
    HDU_FIELD(protocol,HATN_API_NAMESPACE::auth_protocol::TYPE,3)
)

constexpr const char* AUTH_PROTOCOL_HATN_SHARED_SECRET="hss";
constexpr static const uint32_t AUTH_PROTOCOL_HATN_SHARED_SECRET_VERSION=1;

HDU_UNIT(auth_hss_challenge,
    HDU_FIELD(challenge,TYPE_BYTES,1)
)

HDU_UNIT(auth_hss_check,
    HDU_FIELD(token,TYPE_BYTES,1)
    HDU_FIELD(login,TYPE_OBJECT_ID,2)
    HDU_FIELD(hash,TYPE_BYTES,3)
)

HDU_UNIT(auth_token,
    HDU_FIELD(content,TYPE_BYTES,1)
    HDU_FIELD(expire,TYPE_DATETIME,2)
    HDU_FIELD(tag,TYPE_STRING,3)
)

HDU_UNIT(auth_complete,
    HDU_FIELD(session_token,auth_token::TYPE,1)
    HDU_FIELD(refresh_token,auth_token::TYPE,2)
)

HDU_UNIT(auth_refresh,
    HDU_FIELD(token,TYPE_BYTES,1)
)

HDU_UNIT(auth_with_token,
    HDU_FIELD(token,TYPE_BYTES,1)
    HDU_FIELD(tag,TYPE_STRING,2)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERAUTHPROTOCOL_H
