/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslcontext.h
  *
  *   TLS context with OpenSSL backend
  *
  */

/****************************************************************************/

#ifndef HATNOPENSSLCONTEXT_H
#define HATNOPENSSLCONTEXT_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/conf.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <hatn/crypt/securestreamcontext.h>
#include <hatn/crypt/securekey.h>

#include <hatn/crypt/plugins/openssl/opensslsessionticketkey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! TLS context with OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslContext : public SecureStreamContext
{
    public:

        //! Constructor
        OpenSslContext(
            EndpointType endpointType=EndpointType::Generic,
            VerifyMode verifyMode=VerifyMode::Peer
        );

        //! Destructor
        virtual ~OpenSslContext();
        OpenSslContext(const OpenSslContext&)=delete;
        OpenSslContext(OpenSslContext&&) =delete;
        OpenSslContext& operator=(const OpenSslContext&)=delete;
        OpenSslContext& operator=(OpenSslContext&&) =delete;

        //! Get TLS context
        inline SSL_CTX* nativeContext() noexcept
        {
            return m_sslCtx;
        }

        //! Get TLS verification mode
        inline int nativeVerifyMode() const noexcept
        {
            return m_nativeVerifyMode;
        }

        //! Create secure stream
        virtual common::SharedPtr<SecureStreamV> createSecureStream(
            const lib::string_view& id=lib::string_view{}, //!< Stream ID
            common::Thread* thread=common::Thread::currentThread() //!< Thread for stream
        ) override;

        /**
         * @brief Create error from error content
         * @param content Error content
         * @return Error object
         */
        virtual SecureStreamError createError(
            const common::ByteArray& content
        ) const override;

        /**
         * @brief Set trusted X.509 certificate
         * @param certificate Certificate object
         *
         * @return Operation status
         */
        virtual Error setX509Certificate(
            const common::SharedPtr<X509Certificate>& certificate
        ) noexcept override;

        /**
         * @brief Set X.509 certificate chain
         * @param content X.509 certificates chain
         * @param size Size of content
         *
         * @return Operation status
         */
        virtual Error setX509CertificateChain(
            const common::SharedPtr<X509CertificateChain>& chain
        ) noexcept override;

        /**
         * @brief Set store of trusted X.509 certificates
         * @param store X.509 certificates store
         * @param size Size of content
         *
         * @return Operation status
         */
        virtual common::Error setX509CertificateStore(
            const common::SharedPtr<X509CertificateStore>& store
        ) noexcept override;

        /**
         * @brief Set self private key
         * @param content Content of private key
         * @param format Format content
         *
         * @return Operation status
         */
        virtual Error setPrivateKey(
            const common::SharedPtr<PrivateKey>& key
        ) noexcept override;

        /**
         * @brief Set object to obtain temporary Diffie-Hellman parameters
         * @param dh DH object
         *
         * @return Operation status
         */
        virtual Error setDH(
            const common::SharedPtr<DH>& dh
        ) noexcept override;

#if OPENSSL_API_LEVEL >= 30100
        virtual common::Error setDH(
            bool enableAuto
            ) noexcept override;
#endif

        /**
         * @brief Set list of ECDH algorithms
         * @param algs ECDH algorithms
         *
         * @return Operation status
         */
        virtual common::Error setECDHAlgs(
            const std::vector<const CryptAlgorithm*>& algs
        ) noexcept override;

        /**
         * @brief Set list of ECDH algorithms
         * @param algs ECDH algorithms
         *
         * @return Operation status
         */
        virtual common::Error setECDHAlgs(
            const std::vector<std::string>& algs
        ) noexcept override;

        //! Set verification depth
        virtual Error setVerifyDepth(int depth) noexcept override;

        //! Load key used to encrypt session tickets according to RFC5077
        virtual common::Error loadSessionTicketKey(
            const char* buf,
            size_t size,
            bool keepPrev=true,
            KeyProtector* protector=nullptr
        ) noexcept override;

        //! Get native context handler
        virtual void* nativeHandler() noexcept override
        {
            return m_sslCtx;
        }

        /**
         * @brief Set list of cipher suites in preferred order
         * @param suites Cipher suites
         *
         * @return Operation status
         *
         * Operation is valid only for TLS1.3 and higher.
         * Standard suite names for TLS1.3:
           <pre>
           TLS_AES_128_GCM_SHA256
           TLS_AES_256_GCM_SHA384
           TLS_CHACHA20_POLY1305_SHA256
           TLS_AES_128_CCM_SHA256
           TLS_AES_128_CCM_8_SHA256
           </pre>
         *
         */
        virtual common::Error setCipherSuites(
            const std::vector<std::string>& suites
        ) noexcept override;

        //! Get current session tickets key
        inline const OpenSslSessionTicketKey& sessionTicketKey() const noexcept
        {
            return m_sessionTicketKey;
        }

        //! Get previuos session tickets key
        inline const OpenSslSessionTicketKey& prevSessionTicketKey() const noexcept
        {
            return m_prevSessionTicketKey;
        }

    protected:

        //! Update endpoint type in derived class
        virtual void updateEndpointType() override
        {
            doUpdateEndpointType();
        }
        //! Update verification mode in derived class
        virtual void updateVerifyMode() override
        {
            doUpdateVerifyMode();
        }
        //! Update protocol version in derived class
        virtual void updateProtocolVersion() override
        {
            doUpdateProtocolVersion();
        }

    private:

        //! Update endpoint type in derived class
        void doUpdateEndpointType();
        //! Update verification mode in derived class
        void doUpdateVerifyMode();
        //! Update protocol version in derived class
        void doUpdateProtocolVersion();

        void updateSessionTicketEncCb();

        common::Error checkPkeyMatchCrt();

        SSL_CTX* m_sslCtx;
        int m_nativeVerifyMode;

        OpenSslSessionTicketKey m_sessionTicketKey;
        OpenSslSessionTicketKey m_prevSessionTicketKey;

        bool m_pkeySet;
        bool m_crtSet;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLCONTEXT_H
