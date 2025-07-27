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

#include <hatn/network/ipendpoint.h>

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/auth/authprotocol.h>
#include <hatn/clientserver/models/serverroute.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(account_config,
    HDU_FIELD(config_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(account_id,TYPE_OBJECT_ID,2)
    HDU_FIELD(valid_till,TYPE_DATETIME,3)
    HDU_FIELD(user_name,TYPE_STRING,4)
    HDU_FIELD(server_name,TYPE_STRING,5)
    HDU_FIELD(server_id,TYPE_STRING,6)
    HDU_FIELD(server_root_certificate,TYPE_STRING,7)
    HDU_REPEATED_FIELD(server_certificate_chain,TYPE_STRING,8)
    HDU_REPEATED_FIELD(server_route,server_node::TYPE,9)
    HDU_FIELD(description,TYPE_STRING,10)
    HDU_FIELD(token,TYPE_BYTES,11)
    HDU_FIELD(token_tag,TYPE_STRING,12)
    HDU_FIELD(topic,TYPE_STRING,13)
)

HDU_UNIT(account_config_token,
    HDU_FIELD(content,TYPE_BYTES,1)
    HDU_FIELD(signature,TYPE_BYTES,2)
    HDU_FIELD(encrypted,TYPE_BOOL,3)
)

struct AccountConfig
{
    using type=account_config::managed;
    using shared_type=HATN_COMMON_NAMESPACE::SharedPtr<type>;

    AccountConfig() : message(common::makeShared<type>())
    {}

    AccountConfig(shared_type msg) : message(std::move(msg))
    {}

    shared_type message;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSACCOUNTCONFIG_H
