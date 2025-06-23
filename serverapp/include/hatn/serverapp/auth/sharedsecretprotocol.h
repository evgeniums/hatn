/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/auth/sharedsecretprotocol.h
  */

/****************************************************************************/

#ifndef HATNAUTHSHAREDSECRETPROTOCOL_H
#define HATNAUTHSHAREDSECRETPROTOCOL_H

#include <hatn/base/configobject.h>

#include <hatn/crypt/ciphersuite.h>

#include <hatn/clientserver/auth/authprotocol.h>

#include <hatn/serverapp/serverappdefs.h>
#include <hatn/serverapp/auth/authprotocol.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

HDU_UNIT(auth_protocol_shared_secret_config,
    HDU_FIELD(secret,TYPE_STRING,1)
    HDU_FIELD(token_ttl_secs,TYPE_UINT32,2,false,300)
)

class HATN_SERVERAPP_EXPORT SharedSecretAuthBase : public AuthProtocol,
                                                   public HATN_BASE_NAMESPACE::ConfigObject<auth_protocol_shared_secret_config::type>
{
    public:

        SharedSecretAuthBase() : AuthProtocol(
                HATN_CLIENT_SERVER_NAMESPACE::AUTH_PROTOCOL_HATN_SHARED_SECRET,
                HATN_CLIENT_SERVER_NAMESPACE::AUTH_PROTOCOL_HATN_SHARED_SECRET_VERSION
            )
        {}

        Error init(
            const crypt::CipherSuite* suite
        );

        Result<common::SharedPtr<auth_negotiate_response::managed>> prepareChallengeToken(
            const common::SharedPtr<auth_negotiate_request::managed>& authRequest
        );

    private:

        common::SharedPtr<crypt::SymmetricKey> m_tokenEncryptionKey;
};

template <typename ContextTraits>
class SharedSecretAuth : public SharedSecretAuthBase
{
    public:

        using Context=typename ContextTraits::Context;

        template <typename CallbackT>
        void prepare(
            common::SharedPtr<Context> ctx,
            CallbackT callback, // void (common::SharedPtr<Context> ctx, const Error& ec, common::SharedPtr<auth_negotiate_response::managed> message)
            const common::SharedPtr<auth_negotiate_request::managed>& authRequest
        );

        template <typename CallbackT>
        void check(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const HATN_CLIENT_SERVER_NAMESPACE::auth_hss_check::managed* message
        );
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNAUTHSHAREDSECRETPROTOCOL_H
