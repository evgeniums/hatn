/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslcipher.h
 *
 * 	Implementation of symmetric cipher with OpenSSL EVP backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLSYMMETRIC_H
#define HATNOPENSSLSYMMETRIC_H

#include <boost/algorithm/string.hpp>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/symmetricworker.h>

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslsecretkey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

namespace detail
{
struct SymmetricTraits
{
    static void free(EVP_CIPHER_CTX* ctx)
    {
        ::EVP_CIPHER_CTX_free(ctx);
    }
};
}

template <bool Encrypt,typename BaseT,typename DerivedT>
class OpenSslSymmetricWorker :
            public common::NativeHandlerContainer<
                                        EVP_CIPHER_CTX,
                                        detail::SymmetricTraits,
                                        BaseT,
                                        DerivedT
                                     >
{
    public:

        using common::NativeHandlerContainer<
                                EVP_CIPHER_CTX,
                                detail::SymmetricTraits,
                                BaseT,
                                DerivedT
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

        /**
         * @brief Init encryptor/decryptor
         * @param iv Initialization data, usually it is IV
         * @param config Extra configuration parameters of the cipher
         * @return Operation status
         */
        virtual common::Error doInit(const char* iv, size_t size=0) override
        {
            if (size!=0)
            {
                if (size!=getIVSize())
                {
                    return cryptError(CryptError::INVALID_IV_SIZE);
                }
            }

            if (this->nativeHandler().isNull())
            {
                this->nativeHandler().handler = ::EVP_CIPHER_CTX_new();
                if (this->nativeHandler().isNull())
                {
                    return makeLastSslError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
                }
            }
            else
            {
                if (::EVP_CIPHER_CTX_reset(this->nativeHandler().handler)!=1)
                {
                    return makeLastSslError(CryptError::ENCRYPTION_FAILED);
                }
            }

            // if key's content is protected then unpack the key
            if (this->key()->isContentProtected())
            {
                // the key will stay unprotected in the memory
                HATN_CHECK_RETURN(const_cast<SymmetricKey*>(this->key())->unpackContent())
            }

            // check if key size is ok
            if (this->key()->content().size()
                    !=
                static_cast<size_t>(::EVP_CIPHER_key_length(cipher())))
            {
                return makeLastSslError(CryptError::INVALID_KEY_LENGTH);
            }

            HATN_CHECK_RETURN(subInit(iv))

            if (::EVP_CIPHER_CTX_set_padding(this->nativeHandler().handler,this->alg()->enablePadding())!=1)
            {
                return makeLastSslError(CryptError::ENCRYPTION_FAILED);
            }

            return Error();
        }

        //! Reset cipher so that it can be used again with new data
        virtual void doReset() noexcept override
        {
            if (!this->nativeHandler().isNull())
            {
                ::EVP_CIPHER_CTX_reset(this->nativeHandler().handler);
            }
        }

        virtual common::Error subInit(const char* iv)
        {
            if (Encrypt)
            {
                if (::EVP_EncryptInit_ex(this->nativeHandler().handler,
                                  cipher(),
                                  this->key()->alg()->engine()->template nativeHandler<ENGINE>(),
                                  reinterpret_cast<const unsigned char*>(this->key()->content().data()),
                                  reinterpret_cast<const unsigned char*>(iv)
                                  ) != 1)
                {
                    return makeLastSslError(CryptError::ENCRYPTION_FAILED);
                }
            }
            else
            {
                if (::EVP_DecryptInit_ex(this->nativeHandler().handler,
                                  cipher(),
                                  this->key()->alg()->engine()->template nativeHandler<ENGINE>(),
                                  reinterpret_cast<const unsigned char*>(this->key()->content().data()),
                                  reinterpret_cast<const unsigned char*>(iv)
                                  ) != 1)
                {
                    return makeLastSslError(CryptError::DECRYPTION_FAILED);
                }
            }

            return Error();
        }
};

//! Symmetric encryptor with OpenSSL EVP backend
class HATN_OPENSSL_EXPORT OpenSslSymmetricEncryptor :
        public OpenSslSymmetricWorker<true,SEncryptor,OpenSslSymmetricEncryptor>
{
    public:

        using OpenSslSymmetricWorker<true,SEncryptor,OpenSslSymmetricEncryptor>::OpenSslSymmetricWorker;

    private:

        /**
         * @brief Actually encrypt data
         * @param bufIn Input buffer
         * @param sizeIn Size of input data
         * @param bufOut Output buffer
         * @param sizeOut Resulting size
         * @param lastBlock Finalize processing
         * @return Operation status
         */
        virtual common::Error doProcess(
            const char* bufIn,
            size_t sizeIn,
            char* bufOut,
            size_t& sizeOut,
            bool lastBlock
        ) override;
};

//! Symmetric decryptor with OpenSSL EVP backend
class HATN_OPENSSL_EXPORT OpenSslSymmetricDecryptor :
        public OpenSslSymmetricWorker<false,SDecryptor,OpenSslSymmetricDecryptor>
{
    public:

        using OpenSslSymmetricWorker<false,SDecryptor,OpenSslSymmetricDecryptor>::OpenSslSymmetricWorker;

    private:

        /**
         * @brief Actually decrypt data
         * @param bufIn Input buffer
         * @param sizeIn Size of input data
         * @param bufOut Output buffer
         * @param sizeOut Resulting size
         * @param lastBlock Finalize processing
         * @return Operation status
         */
        virtual common::Error doProcess(
            const char* bufIn,
            size_t sizeIn,
            char* bufOut,
            size_t& sizeOut,
            bool lastBlock
        ) override;
};

class HATN_OPENSSL_EXPORT CipherAlg : public CryptAlgorithm
{
    public:

        CipherAlg(const CryptEngine* engine, const char* name, const char* nativeName, CryptAlgorithm::Type type=CryptAlgorithm::Type::SENCRYPTION, bool enablePadding=true) noexcept
            : CryptAlgorithm(engine,type,name,0,::EVP_get_cipherbyname(nativeName)),
              m_enablePadding(enablePadding)
        {}

        virtual size_t keySize() const override
        {
            return static_cast<size_t>(::EVP_CIPHER_key_length(nativeHandler<EVP_CIPHER>()));
        }

        virtual size_t ivSize() const override
        {
            return static_cast<size_t>(::EVP_CIPHER_iv_length(nativeHandler<EVP_CIPHER>()));
        }

        virtual size_t blockSize() const override
        {
            return static_cast<size_t>(::EVP_CIPHER_block_size(nativeHandler<EVP_CIPHER>()));
        }

        virtual common::SharedPtr<SymmetricKey> createSymmetricKey() const override;

        virtual bool enablePadding() const override
        {
            return m_enablePadding;
        }
        virtual void setEnablePadding(bool enable) override
        {
            m_enablePadding=enable;
        }

    private:

        bool m_enablePadding;
};

class HATN_OPENSSL_EXPORT OpenSslSymmetric
{
    public:

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept
        {
            std::vector<std::string> parts;
            splitAlgName(name,parts);
            if (parts.size()<1)
            {
                return cryptError(CryptError::INVALID_ALGORITHM);
            }
            auto algName=parts[0];

            bool padding=true;
            if (parts.size()>1)
            {
                auto paddingStr=parts[1];
                if (boost::iequals(paddingStr,std::string("no-padding")))
                {
                    padding=false;
                }
            }

            alg=std::make_shared<CipherAlg>(engine,name,algName.c_str(),CryptAlgorithm::Type::SENCRYPTION,padding);
            return common::Error();
        }

        static std::vector<std::string> listCiphers();
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLSYMMETRIC_H
