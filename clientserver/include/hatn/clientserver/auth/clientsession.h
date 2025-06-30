/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/auth/clientsession.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSESSION_H
#define HATNCLIENTSESSION_H

#include <hatn/common/objecttraits.h>
#include <hatn/common/env.h>
#include <hatn/common/flatmap.h>

#include <hatn/crypt/keycontainer.h>
#include <hatn/crypt/ciphersuite.h>

#include <hatn/api/authprotocol.h>
#include <hatn/api/client/session.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/auth/authprotocol.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

namespace clientapi=HATN_API_NAMESPACE::client;
namespace api=HATN_API_NAMESPACE;

class ClientAuthProtocol : public api::AuthProtocol
{
    public:

        ClientAuthProtocol(lib::string_view name,
                           VersionType version) : api::AuthProtocol(name,version)
        {}
};

template <typename Traits>
class ClientAuthProtocolT : public api::AuthProtocol,
                            public common::WithTraits<Traits>
{
    public:

        template <typename ContextT, typename CallbackT>
        void handleAuth(
            common::SharedPtr<ContextT> ctx,
            common::SharedPtr<CallbackT> callback,
            common::SharedPtr<auth_negotiate_response::managed> authNegotiateResponse
        )
        {
            this->traits().handleAuth(std::move(ctx),std::move(callback),std::move(authNegotiateResponse));
        }
};

class HATN_CLIENT_SERVER_EXPORT AuthClientSharedSecretImpl : public ClientAuthProtocol
{
    public:

        AuthClientSharedSecretImpl(const crypt::CipherSuites* cipherSuites=nullptr)
            : ClientAuthProtocol(
                AUTH_PROTOCOL_HATN_SHARED_SECRET,
                AUTH_PROTOCOL_HATN_SHARED_SECRET_VERSION
              ),
              m_cipherSuites(cipherSuites)
        {}

        void setCipherSuites(const crypt::CipherSuites* cipherSuites) noexcept
        {
            m_cipherSuites=cipherSuites;
        }

        const crypt::CipherSuites* cipherSuites() const noexcept
        {
            return m_cipherSuites;
        }

        void setSharedSecret(
            crypt::SecurePlainContainer sharedSecret
        )
        {
            m_sharedSecret=std::move(sharedSecret);
        }

        static Result<crypt::SecurePlainContainer> calculateSharedSecret(
            lib::string_view login,
            lib::string_view password,
            lib::string_view cipherSuiteId={}
        );

        Error calculateMAC(
            lib::string_view challenge
        );

    protected:

        crypt::SecurePlainContainer m_sharedSecret;
        const crypt::CipherSuites* m_cipherSuites;
};

class AuthClientSharedSecret : public AuthClientSharedSecretImpl
{
    public:

        using AuthClientSharedSecretImpl::AuthClientSharedSecretImpl;

        template <typename ContextT, typename CallbackT>
        void handleAuth(
            common::SharedPtr<ContextT> ctx,
            common::SharedPtr<CallbackT> callback,
            common::SharedPtr<auth_negotiate_response::managed> authNegotiateResponse
        );
};

class HATN_CLIENT_SERVER_EXPORT ClientSessionImpl
{
    public:

        using Callback=clientapi::SessionRefreshCb;

        void setLogin(
            lib::string_view login,
            lib::string_view topic={}
        );

        Error loadSessionToken(lib::string_view content);

        Error loadRefreshToken(lib::string_view content);

        bool checkNeedRefreshForAuthError(const api::Service*, const api::Method*, const clientapi::Response&) const;

        void auth(Callback callback);

    private:

        common::SharedPtr<auth_token::managed> m_sessionToken;
        common::SharedPtr<auth_token::managed> m_refreshToken;
};

template <typename ...AuthProtocols>
class ClientSessionTraits : public common::TaskContext,
                            public ClientSessionImpl,
                            public common::Env<AuthProtocols...>
{
    public:

        template <typename ...Args>
        ClientSessionTraits(Args&& ...args);

        void refresh(lib::string_view ctxId, Callback callback, clientapi::Response ={});

    private:

        using AuthHandler=std::function<void (common::TaskContextShared,Callback,common::SharedPtr<auth_negotiate_response::managed>)>;

        void handleAuth(
            Callback callback,
            common::SharedPtr<auth_negotiate_response::managed> authNegotiateResponse
        );

        common::FlatMap<ClientAuthProtocol,AuthHandler> m_protocolHandlers;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSESSION_H
