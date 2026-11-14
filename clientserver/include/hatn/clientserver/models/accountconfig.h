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

/**
 * Pairing part of an account config carried by the QR code or file of "Add new device"
 * (multi-device-architecture.md §5.3). The config then has no token: the server route authenticates the
 * pairing login, and this unit authenticates the opener to the new device.
 */
HDU_UNIT(live_pairing_config,
    HDU_FIELD(version,TYPE_UINT32,1)
    HDU_FIELD(pairing_id,TYPE_STRING,2)
    HDU_FIELD(opener_eph_pub,TYPE_BYTES,3)  //!< DER SubjectPublicKeyInfo of the opener's ephemeral ECDH key
    HDU_FIELD(qr_secret,TYPE_BYTES,4)       //!< 16 random bytes the server never sees
    HDU_FIELD(expires_at,TYPE_DATETIME,5)
)

HDU_UNIT(account_config,
    HDU_FIELD(config_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(account_id,TYPE_OBJECT_ID,2)
    HDU_FIELD(valid_till,TYPE_DATETIME,3)
    HDU_FIELD(user_name,TYPE_STRING,4)
    HDU_FIELD(server_name,TYPE_STRING,5)
    HDU_FIELD(server_id,TYPE_STRING,6)
    HDU_FIELD(server_root_certificate,TYPE_STRING,7)
    HDU_FIELD(server_certificate_chain,TYPE_STRING,8)
    HDU_REPEATED_FIELD(server_route,server_node::TYPE,9)
    HDU_FIELD(description,TYPE_STRING,10)
    HDU_FIELD(token,TYPE_BYTES,11)
    HDU_FIELD(token_tag,TYPE_STRING,12)
    HDU_FIELD(topic,TYPE_STRING,13)
    HDU_FIELD(title,TYPE_STRING,14)
    HDU_FIELD(mock,TYPE_BOOL,15)
    // Feedback provider provisioning — optional; set by the server admin to route
    // account-level user feedback to a specific destination (e.g. a Sentry project).
    HDU_FIELD(feedback_scheme,TYPE_STRING,16)
    HDU_FIELD(feedback_dsn,TYPE_STRING,17)
    HDU_FIELD(feedback_release,TYPE_STRING,18)
    HDU_FIELD(feedback_environment,TYPE_STRING,19)
    // When set, the client publishes the username invitation of a character on its own: after the
    // account is activated and whenever the character's username is set.
    HDU_FIELD(auto_publish_username_invitation,TYPE_BOOL,20)
    // Set instead of token when the config adds a device by pairing; see live_pairing_config.
    HDU_FIELD(live_pairing,live_pairing_config::TYPE,21)
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
