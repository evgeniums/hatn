/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/securestreamcontext.h
  *
  *   Base class for contexts of secure streams, e.g. TLS context
  *
  */

/****************************************************************************/

#ifndef HATNSECURECONTEXT_H
#define HATNSECURECONTEXT_H

#include <functional>

#include <hatn/common/sharedptr.h>
#include <hatn/common/memorylockeddata.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/objectid.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/x509certificate.h>
#include <hatn/crypt/x509certificatechain.h>
#include <hatn/crypt/x509certificatestore.h>
#include <hatn/crypt/securestreamtypes.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/dh.h>

HATN_CRYPT_NAMESPACE_BEGIN

class SecureStreamV;

//! Base class for TLS contexts
class SecureStreamContext
{
    public:

        //! Endpoint type
        using EndpointType=SecureStreamTypes::Endpoint;
        //! Verification mode
        using VerifyMode=SecureStreamTypes::Verification;

        //! Constructor
        SecureStreamContext(
            EndpointType endpointType=EndpointType::Generic,
            VerifyMode verifyMode=VerifyMode::None
        ) noexcept :
                m_minProtocolVersion(SecureStreamTypes::ProtocolVersion::TLS1_3),
                m_maxProtocolVersion(SecureStreamTypes::ProtocolVersion::TLS1_3),
                m_endpointType(endpointType),
                m_verifyMode(verifyMode),
                m_ignoreAllErrors(false),
                m_collectAllErrors(false)
        {
        }

        virtual ~SecureStreamContext()=default;
        SecureStreamContext(const SecureStreamContext&)=delete;
        SecureStreamContext(SecureStreamContext&&) =delete;
        SecureStreamContext& operator=(const SecureStreamContext&)=delete;
        SecureStreamContext& operator=(SecureStreamContext&&) =delete;

        //! Create TLS stream
        virtual common::SharedPtr<SecureStreamV> createSecureStream(
            common::Thread* thread=common::Thread::currentThread() //!< Thread for stream
        )
        {
            Assert(false,"No default TLS stream builder");
            std::ignore=thread;
            return common::SharedPtr<SecureStreamV>();
        }

        //! Get endpoint type
        inline EndpointType endpointType() const noexcept
        {
            return m_endpointType;
        }
        //! Set socket type
        inline void setEndpointType(EndpointType type)
        {
            m_endpointType=type;
            updateEndpointType();
        }

        //! Get verification mode
        inline VerifyMode verifyMode() const noexcept
        {
            return m_verifyMode;
        }
        //! Set verification mode
        inline void setVerifyMode(VerifyMode mode)
        {
            m_verifyMode=mode;
            updateVerifyMode();
        }

        //! Set list of errors to ignore
        inline void setIgnoredErrors(
            SecureStreamErrors errors
        ) noexcept
        {
            m_ignoredErrors=std::move(errors);
        }

        //! Add an error to ignore list
        inline void addIgnoredError(
            const SecureStreamError& error
        )
        {
            m_ignoredErrors.push_back(error);
        }

        //! Add an error to ignore list
        inline void addIgnoredError(
            SecureStreamError&& error
        )
        {
            m_ignoredErrors.push_back(std::move(error));
        }

        //! Clear list errors to ignore
        inline void clearIgnoredErrors() noexcept
        {
            m_ignoredErrors.clear();
        }

        //! Get ignored errors
        inline const SecureStreamErrors& ignoredErrors() const noexcept
        {
            return m_ignoredErrors;
        }

        //! Check if an error is in ignored list
        inline bool isErrorIgnored(
            const SecureStreamError& error
        ) const noexcept
        {
            for (auto&& it : m_ignoredErrors)
            {
                if (it==error)
                {
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Create error from error content
         * @param content Error content
         * @return Error object
         */
        virtual SecureStreamError createError(
            const common::ByteArray& content
        ) const
        {
            std::ignore=content;
            return SecureStreamError();
        }

        //! Ignore all errors
        inline void setAllErrorsIgnored(bool enabled) noexcept
        {
            m_ignoreAllErrors=enabled;
        }

        //! Check if all errors are ignored
        inline bool checkAllErrorsIgnored() const noexcept
        {
            return m_ignoreAllErrors;
        }

        inline void setCollectAllErrors(bool enable) noexcept
        {
            m_collectAllErrors=enable;
        }
        inline bool checkCollectAllErrors() const noexcept
        {
            return m_collectAllErrors;
        }

        //! Set min protocol version
        void setMinProtocolVersion(SecureStreamTypes::ProtocolVersion version)
        {
            m_minProtocolVersion=version;
            updateProtocolVersion();
        }
        //! Get min protocol version
        SecureStreamTypes::ProtocolVersion minProtocolVersion() const noexcept
        {
            return m_minProtocolVersion;
        }

        //! Set max protocol version
        void setMaxProtocolVersion(SecureStreamTypes::ProtocolVersion version)
        {
            m_maxProtocolVersion=version;
            updateProtocolVersion();
        }
        //! Get max protocol version
        SecureStreamTypes::ProtocolVersion maxProtocolVersion() const noexcept
        {
            return m_maxProtocolVersion;
        }

        /**
         * @brief Set trusted X.509 certificate
         * @param certificate Certificate object
         *
         * @return Operation status
         */
        virtual common::Error setX509Certificate(
            const common::SharedPtr<X509Certificate>& certificate
        ) noexcept =0;

        /**
         * @brief Set X.509 certificate chain
         * @param chain X.509 certificates chain
         * @param size Size of content
         *
         * @return Operation status
         */
        virtual common::Error setX509CertificateChain(
            const common::SharedPtr<X509CertificateChain>& chain
        ) noexcept =0;

        /**
         * @brief Set store of trusted X.509 certificates
         * @param store X.509 certificates store
         * @param size Size of content
         *
         * @return Operation status
         */
        virtual common::Error setX509CertificateStore(
            const common::SharedPtr<X509CertificateStore>& store
        ) noexcept =0;

        /**
         * @brief Set self private key
         * @param content Content of private key
         * @param format Format content
         *
         * @return Operation status
         */
        virtual common::Error setPrivateKey(
            const common::SharedPtr<PrivateKey>& key
        ) noexcept =0;

        /**
         * @brief Set object to obtain temporary Diffie-Hellman parameters for DH algorithm
         * @param dh DH object
         *
         * @return Operation status
         */
        virtual common::Error setDH(
            const common::SharedPtr<DH>& dh
        ) noexcept
        {
            std::ignore=dh;
            return common::Error(common::CommonError::UNSUPPORTED);
        }

        virtual common::Error setDH(
              bool enableAuto
            ) noexcept
        {
            std::ignore=enableAuto;
            return common::Error(common::CommonError::UNSUPPORTED);
        }

        /**
         * @brief Set list of ECDH algorithms
         * @param algs ECDH algorithms
         *
         * @return Operation status
         */
        virtual common::Error setECDHAlgs(
            const std::vector<const CryptAlgorithm*>& algs
        ) noexcept
        {
            std::ignore=algs;
            return common::Error(common::CommonError::UNSUPPORTED);
        }

        /**
         * @brief Set list of ECDH algorithms
         * @param algs ECDH algorithms
         *
         * @return Operation status
         */
        virtual common::Error setECDHAlgs(
            const std::vector<std::string>& algs
        ) noexcept
        {
            std::ignore=algs;
            return common::Error(common::CommonError::UNSUPPORTED);
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
        ) noexcept
        {
            std::ignore=suites;
            return common::Error(common::CommonError::UNSUPPORTED);
        }

        //! Set verification depth
        virtual common::Error setVerifyDepth(int depth) noexcept =0;

        //! Load key for encryptiong session tickets according to RFC5077
        virtual common::Error loadSessionTicketKey(
            const char* buf,
            size_t size,
            bool keepPrev=true,
            KeyProtector* protector=nullptr
        ) noexcept
        {
            std::ignore=buf;
            std::ignore=size;
            std::ignore=keepPrev;
            std::ignore=protector;
            return common::Error();
        }

        //! Get native context handler
        virtual void* nativeHandler() noexcept
        {
            return nullptr;
        }

        //! Load key for encryptiong session tickets according to RFC5077
        inline common::Error loadSessionTicketKey(
            const common::MemoryLockedArray& content,
            bool keepPrevious=true
        ) noexcept
        {
            return loadSessionTicketKey(content.data(),content.size(),keepPrevious);
        }

    protected:

        //! Update endpoint type in derived class
        virtual void updateEndpointType()
        {}

        //! Update verification mode in derived class
        virtual void updateVerifyMode()
        {}

        //! Update protocol version in derived class
        virtual void updateProtocolVersion()
        {}

    private:

        SecureStreamTypes::ProtocolVersion m_minProtocolVersion;
        SecureStreamTypes::ProtocolVersion m_maxProtocolVersion;

        EndpointType m_endpointType;
        VerifyMode m_verifyMode;

        SecureStreamErrors m_ignoredErrors;
        bool m_ignoreAllErrors;
        bool m_collectAllErrors;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNSECURECONTEXT_H
