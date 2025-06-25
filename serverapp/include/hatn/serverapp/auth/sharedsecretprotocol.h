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

#include <hatn/db/topic.h>

#include <hatn/clientserver/auth/authprotocol.h>

#include <hatn/serverapp/serverappdefs.h>
#include <hatn/serverapp/auth/authprotocol.h>
#include <hatn/serverapp/encryptedtoken.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

HDU_UNIT(auth_protocol_shared_secret_config,
    HDU_FIELD(secret,TYPE_STRING,1)
    HDU_FIELD(token_ttl_secs,TYPE_UINT32,2,false,300)
    HDU_FIELD(min_challenge_size,TYPE_UINT32,3,false,8)
    HDU_FIELD(max_challenge_size,TYPE_UINT32,4,false,20)
)

class HATN_SERVERAPP_EXPORT SharedSecretAuthBase : public AuthProtocol,
                                                   public EncryptedToken,
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
            const common::SharedPtr<auth_negotiate_request::managed>& authRequest,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );
};

class SharedSecretAuth : public SharedSecretAuthBase
{
    public:

        template <typename ContextT, typename CallbackT>
        void prepare(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback, // void (common::SharedPtr<Context> ctx, const Error& ec, common::SharedPtr<auth_negotiate_response::managed> message)
            const common::SharedPtr<auth_negotiate_request::managed>& authRequest,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        template <typename ContextT, typename CallbackT, typename LoginControllerT>
        void check(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback, // void (common::SharedPtr<Context> ctx, const Error& ec, Login login)
            common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::auth_hss_check::managed> message,
            const LoginControllerT* loginController,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNAUTHSHAREDSECRETPROTOCOL_H
