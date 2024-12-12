/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssled.h
 *
 * 	Edwards DSA signature algorithms with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLED_H
#define HATNOPENSSLED_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/ec.h>
#include <openssl/evp.h>

#include <hatn/common/nativehandler.h>
#include <hatn/crypt/cryptalgorithm.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslsignature.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Private key for EdDSA algorithms with OpenSSL backend
template <int NativeType>
class EDPrivateKey : public OpenSslPrivateKey
{
    public:

        using OpenSslPrivateKey::OpenSslPrivateKey;

        virtual int nativeType() const noexcept override
        {
            return NativeType;
        }

    protected:

        virtual common::Error doGenerate() override;
};

//! Edwards DSA cryptographic algorithm
class HATN_OPENSSL_EXPORT EDAlg : public OpenSslSignatureAlg
{
    public:

        /**
         * @brief Ctor
         * @param engine Backend engine to use
         * @param algName Algorithm name: "ED/448" or "ED/25519"
         */
        EDAlg(const CryptEngine* engine, CryptAlgorithm::Type type, const char* algName);

        virtual common::SharedPtr<PrivateKey> createPrivateKey() const override;

        virtual common::SharedPtr<SignatureSign> createSignatureSign() const override;
        virtual common::SharedPtr<SignatureVerify> createSignatureVerify() const override;

    private:

        int m_nativeType;
};

/**
 * @brief Class for signing a digital signature using EdDSA with OpenSSL backend
 */
class HATN_OPENSSL_EXPORT OpenSslEdSign : public OpenSslSignatureSign
{
    public:

        using OpenSslSignatureSign::OpenSslSignatureSign;

    protected:

        /**
         * @brief Init processor
         * @return Operation status
         */
        virtual common::Error doInit() noexcept override;

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
         * @brief Reset processor so that it can be used again with new data
         *
         * @return Operation status
         */
        virtual void doReset() noexcept override;

        //! Get actual size of signature after signing process (after finalize())
        virtual size_t actualSignatureSize() const override;

    private:

        common::Error calcSignature(const char *buf, size_t size) noexcept;

        common::ByteArray m_sig;
        OpenSslDigest m_digest;
};

/**
 * @brief Class for verification of a digital signature with OpenSSL backend
 */
class HATN_OPENSSL_EXPORT OpenSslEdVerify : public OpenSslSignatureVerify
{
    public:

        using OpenSslSignatureVerify::OpenSslSignatureVerify;

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
         * @brief Reset context so that it can be used again with new data
         *
         * @return Operation status
         */
        virtual void doReset() noexcept override;

        /**
         * @brief Init
         * @param nativeAlg Digest algoritnm, if null then use previuosly set algorithm
         * @return Operation status
         */
        virtual common::Error doInit() noexcept override;

    private:

        common::ConstDataBuf m_buf;
        OpenSslDigest m_digest;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLED_H
