/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/auth/clientauthprotocolsharedsecret.h
  */

/****************************************************************************/

#ifndef HATNAUTHPROTOCOLSHAREDSECRET_H
#define HATNAUTHPROTOCOLSHAREDSECRET_H

#include <hatn/crypt/keycontainer.h>
#include <hatn/crypt/ciphersuite.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/auth/clientauthprotocol.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

class HATN_CLIENT_SERVER_EXPORT ClientAuthProtocolSharedSecretImpl : public ClientAuthProtocol
{
    public:

        ClientAuthProtocolSharedSecretImpl(std::shared_ptr<api::Service> service={}, const crypt::CipherSuites* cipherSuites=nullptr);

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

        Result<crypt::SecurePlainContainer> calculateSharedSecret(
            lib::string_view login,
            lib::string_view password,
            lib::string_view cipherSuiteId={}
        ) const;

        Error calculateMAC(
            lib::string_view challenge,
            common::ByteArray& targetBuf,
            lib::string_view cipherSuiteId={}
        ) const;

    protected:

        crypt::SecurePlainContainer m_sharedSecret;
        const crypt::CipherSuites* m_cipherSuites;
};

class ClientAuthProtocolSharedSecret : public ClientAuthProtocolSharedSecretImpl
{
    public:

        using ClientAuthProtocolSharedSecretImpl::ClientAuthProtocolSharedSecretImpl;

        template <typename ContextT, typename CallbackT, typename ClientT>
        void invoke(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            ClientT* client,
            common::SharedPtr<auth_negotiate_response::managed> authNegotiateResponse,
            lib::string_view topic,
            uint32_t timeoutMs
        ) const;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNAUTHPROTOCOLSHAREDSECRET_H
