/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/accountconfig.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELSACCOUNTCONFIG_H
#define HATNCLIENTSERVERMODELSACCOUNTCONFIG_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(
    server_route_hop,
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(host,TYPE_STRING,2)
    HDU_FIELD(port,TYPE_UINT16,3)
    HDU_FIELD(protocol,TYPE_STRING,4)
    HDU_REPEATED_FIELD(certificate_chain,TYPE_STRING,5)
    HDU_FIELD(auth_protocol,TYPE_STRING,6)
    HDU_FIELD(auth_token1,TYPE_BYTES,7)
    HDU_FIELD(auth_token2,TYPE_BYTES,8)
    HDU_FIELD(auth_token3,TYPE_BYTES,9)
)

HDU_UNIT(account_config,
    HDU_FIELD(config_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(valid_till,TYPE_DATETIME,2)
    HDU_FIELD(user_name,TYPE_STRING,3)
    HDU_FIELD(server_name,TYPE_STRING,4)
    HDU_FIELD(server_id,TYPE_STRING,5)
    HDU_FIELD(server_root_certificate,TYPE_STRING,6)
    HDU_REPEATED_FIELD(server_certificate_chain,TYPE_STRING,7)
    HDU_FIELD(user_token,TYPE_BYTES,8)
    HDU_REPEATED_FIELD(server_route,server_route_hop::TYPE,9)
    HDU_FIELD(description,TYPE_STRING,10)
)

HDU_UNIT(account_config_token,
    HDU_FIELD(content,TYPE_BYTES,1)
    HDU_FIELD(signature,TYPE_BYTES,2)
    HDU_FIELD(encrypted,TYPE_BOOL,3)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSACCOUNTCONFIG_H
