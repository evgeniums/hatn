/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/serverroute.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELSSERVERROUTE_H
#define HATNCLIENTSERVERMODELSSERVERROUTE_H

#include <hatn/network/ipendpoint.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/auth/authprotocol.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const char* RouteProtocolTcp="tcp";
constexpr const char* RouteProtocolTls="tls";

HDU_UNIT(server_host,
    HDU_FIELD(host,TYPE_STRING,1)
    HDU_FIELD(port,TYPE_UINT16,2)
    HDU_FIELD(ip_version,HDU_TYPE_ENUM(HATN_NETWORK_NAMESPACE::IpVersion),3,false,HATN_NETWORK_NAMESPACE::IpVersion::V4)
    HDU_FIELD(service,TYPE_BOOL,4)
)

HDU_UNIT(
    server_route_hop,
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_REPEATED_FIELD(hosts,server_host::TYPE,2)
    HDU_FIELD(protocol,TYPE_STRING,3)
    HDU_REPEATED_FIELD(certificate_chain,TYPE_STRING,4)
    HDU_FIELD(auth_protocol,TYPE_STRING,5,false,AUTH_PROTOCOL_HATN_SHARED_SECRET)
    HDU_FIELD(auth_protocol_version,TYPE_UINT32,6,false,AUTH_PROTOCOL_HATN_SHARED_SECRET_VERSION)
    HDU_FIELD(auth_token1,TYPE_STRING,7)
    HDU_FIELD(auth_token2,TYPE_STRING,8)
    HDU_FIELD(auth_token3,TYPE_STRING,9)
    HDU_FIELD(auth_token4,TYPE_STRING,10)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSSERVERROUTE_H
