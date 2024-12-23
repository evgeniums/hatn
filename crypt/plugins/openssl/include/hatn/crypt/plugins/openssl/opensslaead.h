/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslcipher.h
 *
 * 	Implementation of AEAD with OpenSSL EVP backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLAEAD_H
#define HATNOPENSSLAEAD_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/aead.h>

#include <hatn/crypt/plugins/openssl/opensslsymmetric.h>

HATN_OPENSSL_NAMESPACE_BEGIN

template <bool Encrypt,typename DerivedT>
class OpenSslAeadWorker : public OpenSslSymmetricWorker<Encrypt,AeadWorker<Encrypt>,DerivedT>
{
    public:

        using OpenSslSymmetricWorker<Encrypt,AeadWorker<Encrypt>,DerivedT>::OpenSslSymmetricWorker;

        virtual void beginNotEncrypted() override
        {
            m_authMode.val=true;
        }
        virtual void endNotEncrypted() override
        {
            m_authMode.val=false;
        }

        //! Get IV size for this encryption algorithm
        virtual size_t ivSize() const noexcept override
        {
            return AeadWorker<Encrypt>::ivSize();
        }

    protected:

#if 0
    // see comments in opensslaead.cpp
        virtual common::Error subInit(const char* iv);
#endif
        virtual common::Error doSetTag(const char* data) noexcept override;
        virtual common::Error doGetTag(char* data) noexcept override;

        bool authNotCipher() const noexcept
        {
            return m_authMode.val;
        }

    private:

        common::ValueOrDefault<bool,false> m_authMode;
};

class OpenSslAeadEncryptor;
class OpenSslAeadDecryptor;

#ifdef _WIN32
template class HATN_OPENSSL_EXPORT OpenSslAeadWorker<true,OpenSslAeadEncryptor>;
template class HATN_OPENSSL_EXPORT OpenSslAeadWorker<false,OpenSslAeadDecryptor>;
#endif

//! Symmetric encryptor with OpenSSL EVP backend
class HATN_OPENSSL_EXPORT OpenSslAeadEncryptor : public OpenSslAeadWorker<true,OpenSslAeadEncryptor>
{
    public:

        using OpenSslAeadWorker<true,OpenSslAeadEncryptor>::OpenSslAeadWorker;

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
class HATN_OPENSSL_EXPORT OpenSslAeadDecryptor :  public OpenSslAeadWorker<false,OpenSslAeadDecryptor>
{
    public:

        using OpenSslAeadWorker<false,OpenSslAeadDecryptor>::OpenSslAeadWorker;

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

//! AEAD algorithm by OpenSSL backend
class HATN_OPENSSL_EXPORT AeadAlg : public CipherAlg
{
    public:

        AeadAlg(const CryptEngine* engine, const char* name, const char* nativeName, size_t tagSize=0, bool padding=false) noexcept;

        virtual size_t tagSize() const override
        {
            return m_tagSize;
        }
        virtual void setTagSize(size_t size) override
        {
            m_tagSize=size;
        }

    private:

        size_t m_tagSize;
};

//! AEAD processor by OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslAEAD
{
    public:

        inline static bool checkCipherName(const std::string& name) noexcept;
        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept;
        static std::vector<std::string> listCiphers();
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLAEAD_H
