/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslasymmetric.h
 *
 * 	Asymmetric cryptography with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLASYMMETRIC_H
#define HATNOPENSSLASYMMETRIC_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <boost/algorithm/string.hpp>

#include <hatn/crypt/cryptalgorithm.h>
#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/asymmetricworker.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslec.h>
#include <hatn/crypt/plugins/openssl/opensslrsa.h>
#include <hatn/crypt/plugins/openssl/openssldsa.h>
#include <hatn/crypt/plugins/openssl/openssled.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslAsymmetric ********************/

//! Asymmetric cryptography with OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslAsymmetric
{
    public:

        static Error findNativeAlgorithm(
            CryptAlgorithm::Type type,
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept;

        static std::vector<std::string> listSignatures();

        static std::vector<std::string> listAsymmetricCiphers();

        /**
         * @brief Create private key from content
         * @param pkey Target key
         * @param buf Buffer pointer
         * @param size Buffer size
         * @param format Data format
         * @param protector Key protector
         * @param plugin Backend plugin
         * @param engineName Name of backend engine
         * @return Operation status
         *
         */
        static Error createPrivateKeyFromContent(
            common::SharedPtr<PrivateKey>& pkey,
            const char* buf,
            size_t size,
            ContainerFormat format,
            CryptPlugin* plugin,
            KeyProtector* protector=nullptr,
            const char* engineName=nullptr
        );

        /**
         * @brief Get algorithm of public key
         * @param alg Target algorithm
         * @param key Public key
         * @param plugin Backend plugin
         * @param engineName Name of backend engine
         * @return Operation status
         */
        static Error publicKeyAlgorithm(
            CryptAlgorithmConstP &alg,
            const PublicKey* key,
            CryptPlugin* plugin,
            const char* engineName=nullptr
        );
};

/******************* OpenSslAencryptor ********************/

namespace detail
{
struct ASymmetricTraits
{
    static void free(EVP_CIPHER_CTX* ctx)
    {
        ::EVP_CIPHER_CTX_free(ctx);
    }
};
}

class HATN_OPENSSL_EXPORT OpenSslAencryptor : public common::NativeHandlerContainer<
                                   EVP_CIPHER_CTX,
                                   detail::ASymmetricTraits,
                                   AEncryptor,
                                   OpenSslAencryptor
                                   >
{
    public:

        using common::NativeHandlerContainer<
            EVP_CIPHER_CTX,
            detail::ASymmetricTraits,
            AEncryptor,
            OpenSslAencryptor
            >::NativeHandlerContainer;

        size_t getIVSize() const noexcept
        {
            return static_cast<size_t>(::EVP_CIPHER_iv_length(cipher()));
        }

    protected:

        const EVP_CIPHER* cipher() const noexcept
        {
            return this->alg()->template nativeHandler<EVP_CIPHER>();
        }

        virtual common::Error doGenerateIV(char* ivData, size_t* size=nullptr) const override
        {
            auto sz=getIVSize();
            if (size!=nullptr)
            {
                auto reservedSize=*size;
                *size=sz;
                if (reservedSize==0)
                {
                    return OK;
                }
                if (reservedSize<sz)
                {
                    return cryptError(CryptError::INSUFFITIENT_BUFFER_SIZE);
                }
            }
            return genRandData(ivData,sz);
        }

        template <typename ReceiverKeyT, typename EncryptedKeyT>
        Error initCtx(
            const ReceiverKeyT &receiverKey,
            common::ByteArray& iv,
            EncryptedKeyT &encryptedSymmetricKey
        );

        virtual common::Error doInit(
            const common::SharedPtr<PublicKey>& receiverKey,
            common::ByteArray& iv,
            common::ByteArray& encryptedSymmetricKey
        ) override
        {
            return initCtx(receiverKey,iv,encryptedSymmetricKey);
        }

        virtual common::Error doInit(
            const common::SharedPtr<X509Certificate>& receiverCert,
            common::ByteArray& iv,
            common::ByteArray& encryptedSymmetricKey
            ) override
        {
            Error ec;
            auto key=receiverCert->publicKey(&ec);
            HATN_CHECK_EC(ec);
            return initCtx(key,iv,encryptedSymmetricKey);
        }

        virtual common::Error doInit(
            const std::vector<common::SharedPtr<PublicKey>>& receiverKeys,
            common::ByteArray& iv,
            std::vector<common::ByteArray>& encryptedSymmetricKey
        ) override
        {
            return initCtx(receiverKeys,iv,encryptedSymmetricKey);
        }

        virtual common::Error doInit(
                const std::vector<common::SharedPtr<X509Certificate>>& receiverCerts,
                common::ByteArray& iv,
                std::vector<common::ByteArray>& encryptedSymmetricKey
            ) override
        {
            std::vector<common::SharedPtr<PublicKey>> receiverKeys;
            receiverKeys.resize(receiverCerts.size());
            for (size_t i=0;i<receiverCerts.size();i++)
            {
                Error ec;
                auto key=receiverCerts[i]->publicKey(&ec);
                HATN_CHECK_EC(ec);
                receiverKeys[i]=std::move(key);
            }
            return initCtx(receiverKeys,iv,encryptedSymmetricKey);
        }

        //! Reset cipher so that it can be used again with new data
        virtual void doReset() noexcept override
        {
            if (!this->nativeHandler().isNull())
            {
                ::EVP_CIPHER_CTX_reset(this->nativeHandler().handler);
            }
        }

        virtual common::Error doProcess(
            const char* bufIn,
            size_t sizeIn,
            char* bufOut,
            size_t& sizeOut,
            bool lastBlock
        ) override;
};

/******************* OpenSslAdecryptor ********************/

class HATN_OPENSSL_EXPORT OpenSslAdecryptor : public common::NativeHandlerContainer<
                                                  EVP_CIPHER_CTX,
                                                  detail::ASymmetricTraits,
                                                  ADecryptor,
                                                  OpenSslAdecryptor
                                                  >
{
public:

    using common::NativeHandlerContainer<
        EVP_CIPHER_CTX,
        detail::ASymmetricTraits,
        ADecryptor,
        OpenSslAdecryptor
        >::NativeHandlerContainer;

    size_t getIVSize() const noexcept
    {
        return static_cast<size_t>(::EVP_CIPHER_iv_length(cipher()));
    }

protected:

    const EVP_CIPHER* cipher() const noexcept
    {
        return this->alg()->template nativeHandler<EVP_CIPHER>();
    }

    virtual common::Error doInit(
        const common::ConstDataBuf& iv,
        const common::ConstDataBuf& encryptedSymmetricKey
    ) override;

    //! Reset cipher so that it can be used again with new data
    virtual void backendReset() noexcept override
    {
        if (!this->nativeHandler().isNull())
        {
            ::EVP_CIPHER_CTX_reset(this->nativeHandler().handler);
        }
    }

    virtual common::Error doProcess(
        const char* bufIn,
        size_t sizeIn,
        char* bufOut,
        size_t& sizeOut,
        bool lastBlock
        ) override;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLASYMMETRIC_H
