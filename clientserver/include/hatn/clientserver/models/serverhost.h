/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/serverhost.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELSSERVERHOST_H
#define HATNCLIENTSERVERMODELSSERVERHOST_H

#include <hatn/network/ipendpoint.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(server_host,
    HDU_FIELD(host,TYPE_STRING,1)
    HDU_FIELD(port,TYPE_UINT16,2)
    HDU_FIELD(ip_version,HDU_TYPE_ENUM(HATN_NETWORK_NAMESPACE::IpVersion),3,false,HATN_NETWORK_NAMESPACE::IpVersion::V4)
    HDU_FIELD(service,TYPE_BOOL,4)
    HDU_FIELD(dns_service,TYPE_STRING,5)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSSERVERHOST_H
