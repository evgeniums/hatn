/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslsignature.h
 *
 * 	HMAC (Hash Message Authentication Code) implementation with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLSIGNATUREDIGEST_H
#define HATNOPENSSLSIGNATUREDIGEST_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/hmac.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/signature.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/openssldigest.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslpublickey.h>
#include <hatn/crypt/plugins/openssl/openssldigestsign.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Base class for signature algorithms with OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslSignatureAlg : public CryptAlgorithm
{
    public:

        using CryptAlgorithm::CryptAlgorithm;

        virtual common::SharedPtr<SignatureSign> createSignatureSign() const override;
        virtual common::SharedPtr<SignatureVerify> createSignatureVerify() const override;
};

/**
 * @brief Class for signing a digital signature with OpenSSL backend
 */
class HATN_OPENSSL_EXPORT OpenSslSignatureSign :
                public common::NativeHandlerContainer<EVP_MD_CTX,detail::DigestSignTraits,SignatureSign,OpenSslSignatureSign>
{
    public:

        using common::NativeHandlerContainer<EVP_MD_CTX,detail::DigestSignTraits,SignatureSign,OpenSslSignatureSign>::NativeHandlerContainer;

    protected:

        /**
         * @brief Actually process data
         * @param buf Input buffer
         * @param size Size of input data
         * @return Operation status
         */
        virtual common::Error doProcess(
            const char* buf,
            size_t size
        ) noexcept override;

        /**
         * @brief Actually finalize processing and put signature to buffer
         * @param buf Output buffer
         * @param size Actual size of result data that was put to output buffer
         * @return Operation status
         */
        virtual common::Error doFinalize(
            char* buf
        ) noexcept override;

        /**
         * @brief Init processor
         * @return Operation status
         */
        virtual common::Error doInit() noexcept override;

        /**
         * @brief Reset processor so that it can be used again with new data
         *
         * @return Operation status
         */
        virtual void doReset() noexcept override;

        //! Get maximum size of result signature
        /**
         * @brief Get maximum result size
         * @return Maxium possible size of signature, the actual result size must be queried using actualSignatureSize()
         *
         * @throws common::ErrorException if native alg engine is not set
         *
         */
        virtual size_t resultSize() const override;

        //! Get actual size of signature after signing process (after finalize())
        virtual size_t actualSignatureSize() const override;

    private:

        common::ValueOrDefault<size_t,0> m_sigSize;
};

/**
 * @brief Class for verification of a digital signature with OpenSSL backend
 */
class HATN_OPENSSL_EXPORT OpenSslSignatureVerify :
            public common::NativeHandlerContainer<EVP_MD_CTX,detail::DigestSignTraits,SignatureVerify,OpenSslSignatureVerify>
{
    public:

        using common::NativeHandlerContainer<EVP_MD_CTX,detail::DigestSignTraits,SignatureVerify,OpenSslSignatureVerify>::NativeHandlerContainer;

    protected:

        /**
         * @brief Actually process data
         * @param buf Input buffer
         * @param size Size of input data
         * @return Operation status
         */
        virtual common::Error doProcess(
            const char* buf,
            size_t size
        ) noexcept override;

        /**
         * @brief Verify signature
         * @param signature Signature buffer
         * @param signatureLength Signature length
         * @return Operation status
         */
        virtual common::Error doVerify(
            const char* signature,
            size_t signatureLength
        ) noexcept override;

        /**
         * @brief Init
         * @param nativeAlg Digest algoritnm, if null then use previuosly set algorithm
         * @return Operation status
         */
        virtual common::Error doInit() noexcept override;

        /**
         * @brief Reset context so that it can be used again with new data
         *
         * @return Operation status
         */
        virtual void doReset() noexcept override;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLSIGNATUREDIGEST_H
