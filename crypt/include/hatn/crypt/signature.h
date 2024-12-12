/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/signature.h
 *
 *      Base classes for digital signature signing and verifying message digests with asymmetric algorithms
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTHSIGNATUREDIGEST_H
#define HATNCRYPTHSIGNATUREDIGEST_H

#include <hatn/common/bytearray.h>
#include <hatn/common/runonscopeexit.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/digest.h>
#include <hatn/crypt/publickey.h>
#include <hatn/crypt/x509certificate.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Base class for processing digital signatures
class Signature : public Digest
{
    public:

        Signature() noexcept : m_digestAlg(nullptr)
        {}

        virtual const CryptAlgorithm* digestAlg() const noexcept override
        {
            return m_digestAlg;
        }
        virtual void setDigestAlg(const CryptAlgorithm* algorithm) noexcept override
        {
            m_digestAlg=algorithm;
        }

    protected:

        virtual common::Error beforeInit() noexcept override
        {
            if (alg()->type()!=CryptAlgorithm::Type::SIGNATURE)
            {
                return cryptError(CryptError::INVALID_ALGORITHM);
            }
            return common::Error();
        }

    private:

        const CryptAlgorithm* m_digestAlg;
};

/**
 * @brief Digital signing of a message.
 *
 * For some algorithms the signing prosess can consist of two steps: first, a digest hash is calculated on the message,
 * second, the digest is signed with the private key using asymmetric algorithm.
 */
class SignatureSign : public Signature
{
    public:

        /**
         * @brief Ctor
         * @param key Private key to use for signing
         *
         */
        SignatureSign(const PrivateKey* key=nullptr) noexcept : m_key(key)
        {}

        /**
         * @brief Set private key
         * @param key Key
         */
        inline void setKey(const PrivateKey* key) noexcept
        {
            m_key=key;
        }

        //! Get private key
        inline const PrivateKey* key() const noexcept
        {
            return m_key;
        }

        /**
         * @brief Sign a mesage in a single shot
         * @param dataIn Input data to calculate MAC
         * @param sig Buffer to put the signature to
         * @param sigOffset Offset in signature buffer
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers
         * @note Some algorithms can process only single SpanBuffer in one shot.
         */
        template <typename BufferT,typename ContainerSigT>
        common::Error sign(
            const BufferT& dataIn,
            ContainerSigT& sig,
            size_t sigOffset=0
        )
        {
            HATN_CHECK_RETURN(runFinalize(dataIn,sig,sigOffset))
            sig.resize(sig.size()-resultSize()+actualSignatureSize());
            return common::Error();
        }

        //! Get actual size of signature after signing process (after finalize())
        virtual size_t actualSignatureSize() const
        {
            return resultSize();
        }

    private:

        const PrivateKey* m_key;
};

/**
 * @brief Verifying digital signature of a message.
 *
 * For some algorithms the verifying prosess can consist of two steps: first, a digest hash is calculated on the message,
 * second, the digital signature is verified with the public key using asymmetric algorithm.
 */
class SignatureVerify : public Signature
{
    public:

        /**
         * @brief Ctor
         * @param key Public key to use for verifying
         *
         */
        SignatureVerify(const PublicKey* key=nullptr) noexcept : m_key(key)
        {}

        /**
         * @brief Set public key
         * @param key Key
         */
        inline void setKey(const PublicKey* key) noexcept
        {
            m_key=key;
        }

        //! Get public key
        inline const PublicKey* key() const noexcept
        {
            return m_key;
        }

        /**
         * @brief Calculate and verify signature
         * @param dataIn Input data to calculate signature on
         * @param sig Signature to verify
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers.
         * @note Some algorithms can process only single SpanBuffer in one shot.
         */
        template <typename BufferT>
        common::Error verify(
            const BufferT& dataIn,
            const common::SpanBuffer& sig
        )
        {
            try
            {
                return verify(dataIn,sig.view());
            }
            catch (const common::ErrorException& e)
            {
                return e.error();
            }
            return common::Error();
        }

        /**
         * @brief Calculate and verify signature
         * @param dataIn Input data to calculate signature on
         * @param sig Signature to verify
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers
         * @note Some algorithms can process only single SpanBuffer in one shot.
         */
        template <typename BufferT, typename ContainerSigT>
        common::Error verify(
            const BufferT& dataIn,
            const ContainerSigT& sig
        )
        {
            return verify(dataIn,sig.data(),sig.size());
        }

        /**
         * @brief Calculate and verify signature
         * @param dataIn Input data to calculate signature on
         * @param sig Signature to verify
         * @param sigSize Size of signature
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers
         * @note Some algorithms can process only single SpanBuffer in one shot.
         */
        template <typename BufferT>
        common::Error verify(
            const BufferT& dataIn,
            const char* sig,
            size_t sigSize
        )
        {
            HATN_CHECK_RETURN(init())
            HATN_CHECK_RETURN(process(dataIn))
            return doVerify(sig,sigSize);
        }

        /**
         * @brief Calculate and verify signature
         * @param dataIn Input data to calculate signature on
         * @param sig Signature to verify
         * @param cert X509 certificate
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers.
         * @note Some algorithms can process only single SpanBuffer in one shot.
         */
        template <typename BufferT>
        common::Error verifyX509(
            const BufferT& dataIn,
            const common::SpanBuffer& sig,
            const X509Certificate* cert
        )
        {
            try
            {
                return verifyX509(dataIn,sig.view(),cert);
            }
            catch (const common::ErrorException& e)
            {
                return e.error();
            }
            return common::Error();
        }

        /**
         * @brief Calculate and verify signature
         * @param dataIn Input data to calculate signature on
         * @param sig Signature to verify
         * @param cert X509 certificate
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers
         * @note Some algorithms can process only single SpanBuffer in one shot.
         */
        template <typename BufferT, typename ContainerSigT>
        common::Error verifyX509(
            const BufferT& dataIn,
            const ContainerSigT& sig,
            const X509Certificate* cert
        )
        {
            return verifyX509(dataIn,sig.data(),sig.size(),cert);
        }

        /**
         * @brief Calculate and verify signature
         * @param dataIn Input data to calculate signature on
         * @param sig Signature to verify
         * @param sigSize Size of signature
         * @param cert X509 certificate
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers
         * @note Some algorithms can process only single SpanBuffer in one shot.
         */
        template <typename BufferT>
        common::Error verifyX509(
            const BufferT& dataIn,
            const char* sig,
            size_t sigSize,
            const X509Certificate* cert
        )
        {
            common::RunOnScopeExit guard(
                [this]()
                {
                    setKey(nullptr);
                }
            );
            common::Error ec;
            auto pubKey=cert->publicKey(&ec);
            HATN_CHECK_EC(ec);
            setKey(pubKey.get());
            HATN_CHECK_RETURN(init())
            HATN_CHECK_RETURN(process(dataIn))
            return doVerify(sig,sigSize);
        }

    protected:

        /**
         * @brief Verify signature
         * @param signature Signature buffer
         * @param signatureLength Signature length
         * @return Operation status
         */
        virtual common::Error doVerify(
            const char* signature,
            size_t signatureLength
        ) noexcept =0;

    private:

        /**
         * @brief doFinalize
         * @param buf
         * @return
         *
         * Just a stub. Verification must be done explicitly using verify().
         */
        virtual common::Error doFinalize(
            char* buf
        ) noexcept override final
        {
            std::ignore=buf;
            return common::Error(common::CommonError::UNKNOWN);
        }

        const PublicKey* m_key;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTHSIGNATUREDIGEST_H
