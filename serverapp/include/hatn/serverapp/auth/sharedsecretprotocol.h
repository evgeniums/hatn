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

#include <hatn/common/withsharedvalue.h>

#include <hatn/base/configobject.h>

#include <hatn/crypt/ciphersuite.h>

#include <hatn/db/topic.h>

#include <hatn/api/server/serverservice.h>

#include <hatn/clientserver/auth/authprotocol.h>

#include <hatn/serverapp/serverappdefs.h>
#include <hatn/serverapp/auth/authprotocol.h>
#include <hatn/serverapp/encryptedtoken.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

/**************************** SharedSecretAuth ***************************/

HDU_UNIT(auth_protocol_shared_secret_config,
    HDU_FIELD(secret,TYPE_STRING,1)
    HDU_FIELD(token_ttl_secs,TYPE_UINT32,2,false,300)
    HDU_FIELD(min_challenge_size,TYPE_UINT32,3,false,8)
    HDU_FIELD(max_challenge_size,TYPE_UINT32,4,false,20)
)

class HATN_SERVERAPP_EXPORT SharedSecretAuthBase : public AuthProtocol,
                                                   public EncryptedToken,
                                                   public base::ConfigObject<auth_protocol_shared_secret_config::type>
{
    public:

        SharedSecretAuthBase() : AuthProtocol(
                AUTH_PROTOCOL_HATN_SHARED_SECRET,
                AUTH_PROTOCOL_HATN_SHARED_SECRET_VERSION
            )
        {}

        Error init(
            const crypt::CipherSuite* suite
        );

        Result<common::SharedPtr<auth_negotiate_response::managed>> prepareChallengeToken(
            common::SharedPtr<auth_negotiate_request::managed> message,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const;
};

class SharedSecretAuthProtocol : public SharedSecretAuthBase
{
    public:

        template <typename ContextT, typename CallbackT>
        void prepare(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback, // void (common::SharedPtr<Context> ctx, const Error& ec, common::SharedPtr<auth_negotiate_response::managed> message)
            common::SharedPtr<auth_negotiate_request::managed> message,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const;

        template <typename ContextT, typename CallbackT, typename LoginControllerT>
        void check(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback, // void (common::SharedPtr<Context> ctx, const Error& ec, common::SharedPtr<login_profile::managed> login)
            common::SharedPtr<auth_hss_check::managed> message,
            const LoginControllerT& loginController,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const;
};

using WithSharedSecretAuthProtocol=common::WithSharedValue<SharedSecretAuthProtocol>;

/**************************** AuthHssCheckMethod ***************************/

template <typename RequestT>
class AuthHssCheckMethodImpl
{
    public:

        using Request=RequestT;
        using Message=auth_hss_check::managed;

        void exec(
            common::SharedPtr<serverapi::RequestContext<Request>> request,
            serverapi::RouteCb<Request> callback,
            common::SharedPtr<Message> msg
        ) const;

        HATN_VALIDATOR_NAMESPACE::error_report validate(
            const common::SharedPtr<serverapi::RequestContext<Request>> request,
            const Message& msg
        ) const;
};

template <typename RequestT>
class AuthHssCheckMethod : public serverapi::ServiceMethodV<serverapi::ServiceMethodT<AuthHssCheckMethodImpl<RequestT>>,RequestT>
{
    public:

        using Base=serverapi::ServiceMethodV<serverapi::ServiceMethodT<AuthHssCheckMethodImpl<RequestT>>,RequestT>;

        AuthHssCheckMethod() : Base(AuthHssCheckMethodName)
        {}
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNAUTHSHAREDSECRETPROTOCOL_H
