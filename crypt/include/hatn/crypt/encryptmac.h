/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/encryptmac.h
 *
 *      Implementing AEAD using Encrypt-Then-MAC
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTENCRYPTMAC_H
#define HATNCRYPTENCRYPTMAC_H

#include <hatn/common/bytearray.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/digest.h>
#include <hatn/crypt/cipher.h>
#include <hatn/crypt/aeadworker.h>
#include <hatn/crypt/aeadworker.ipp>

HATN_CRYPT_NAMESPACE_BEGIN

/**
 * @brief Key for AEAD over Encrypt-Then-MAC
 */
class HATN_CRYPT_EXPORT EncryptMacKey : public SymmetricKey
{
    public:

        EncryptMacKey(
            common::SharedPtr<SymmetricKey> cipherKey,
            common::SharedPtr<MACKey> macKey
        ) noexcept;

        SymmetricKey* cipherKey() const noexcept
        {
            return m_cipherKey.get();
        }

        MACKey* macKey() const noexcept
        {
            return m_macKey.get();
        }

        //! Check if native key handler is valid
        virtual bool isNativeValid() const noexcept override
        {
            return m_macKey && m_cipherKey && m_macKey->isNativeValid() && m_cipherKey->isNativeValid();
        }

        /**
         * @brief Import raw data from key deriiation functions
         * @param buf Buffer to import from
         * @param bufSize Size of the buffer
         * @return Operation status
         */
        virtual common::Error importFromKDF(
            const char* buf,
            size_t size
        ) override;

    protected:

        virtual common::Error doExportToBuf(common::MemoryLockedArray& buf,ContainerFormat format,bool unprotected) const override;
        virtual common::Error doImportFromBuf(const char* buf, size_t size, ContainerFormat format, bool keepContent) override;

        virtual common::Error doGenerate() override;

    private:

        common::SharedPtr<SymmetricKey> m_cipherKey;
        common::SharedPtr<MACKey> m_macKey;
};

/**
 * @brief Implements AEAD using Encrypt-Then-MAC.
  */
template <bool Encrypt>
class EncryptMAC : public AeadWorker<Encrypt>
{
    public:

        /**
         * @brief Ctor
         * @param key Key used for encryption and decryption
         */
        EncryptMAC(
                const SymmetricKey* key=nullptr
            ) : AeadWorker<Encrypt>(key),
                m_authMode(false)
        {
            updateKey();
        }

        //! Start processing not encrypted data attached for authentication
        virtual void beginNotEncrypted() override
        {
            m_authMode=true;
        }
        //! Finish processing not encrypted data attached for authentication
        virtual void endNotEncrypted() override
        {
            m_authMode=false;
        }

        //! Get block size for this encryption algorithm
        virtual size_t blockSize() const noexcept override;
        //! Get IV size for this encryption algorithm
        virtual size_t ivSize() const noexcept override;

        /**
         * @brief First do whole verification and then whole decryption
         * @param iv Initialization vector
         * @param ciphertext Cipher text
         * @param authdata Not encryoted authenticated data
         * @param tag Athentication tag
         * @param plaintext Decrypted result
         * @return Operation status
         *
         * Use it if you want check the MAC first without decryption and only if MAC is ok then decrypt.
         */
        template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
        common::Error verifyDecrypt(
            const common::SpanBuffer& iv,
            const CipherTextT& ciphertext,
            const AuthDataT& authdata,
            const common::SpanBuffer& tag,
            ContainerOutT& plaintext
        );

        //! Overloaded method without not encrypted authentication data
        template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
        common::Error verifyDecrypt(
            const common::SpanBuffer& iv,
            const CipherTextT& ciphertext,
            const common::SpanBuffer& tag,
            ContainerOutT& plaintext
        )
        {
            return verifyDecrypt(iv,ciphertext,common::SpanBuffer(),tag,plaintext);
        }

        template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
        /**
         * @brief First do whole pack verification and then whole pack decryption
         * @param ciphertext Ciphered pack
         * @param authdata Not encrypted authentication data
         * @param plaintext Decrypted result
         * @return Operation status
         */
        common::Error verifyDecryptPack(
            const CipherTextT& ciphertext,
            const AuthDataT& authdata,
            ContainerOutT& plaintext
        );

        //! Overloaded method without not encrypted authentication data
        template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
        common::Error verifyDecryptPack(
            const CipherTextT& ciphertext,
            ContainerOutT& plaintext
        )
        {
            return verifyDecryptPack(ciphertext,common::SpanBuffer(),plaintext);
        }

    protected:

        virtual common::Error doSetTag(const char* data) noexcept override;
        virtual common::Error doGetTag(char* data) noexcept override;

        virtual common::Error doProcess(
            const char* bufIn,
            size_t sizeIn,
            char* bufOut,
            size_t& sizeOut,
            bool lastBlock
        ) override;
        virtual common::Error doGenerateIV(char* iv) const override;
        virtual common::Error doInit(const char* iv) override;
        virtual void doReset() noexcept override;
        virtual void doUpdateKey() override
        {
            updateKey();
        }

    private:

        /**
         * @brief Update encryption key
         *
         * @throws ErrorException if the key can not be updated
         */
        void updateKey();

        common::SharedPtr<CipherWorker<Encrypt>> m_cipher;
        common::SharedPtr<MAC> m_mac;

        bool m_authMode;

        common::ByteArray m_tag;
};

using EncryptMACEnc=EncryptMAC<true>;
using EncryptMACDec=EncryptMAC<false>;

/**
 * @brief Encrypt-Then-MAC algorithm
 */
class HATN_CRYPT_EXPORT EncryptMacAlg : public CryptAlgorithm
{
    public:

        EncryptMacAlg(const CryptEngine* engine, const char* name, const std::string& cipherName, const std::string& macName, size_t tagSize=0);

        const CryptAlgorithm* cipherAlg() const noexcept
        {
            return m_cipherAlg;
        }
        const CryptAlgorithm* macAlg() const noexcept
        {
            return m_macAlg;
        }

        //! Get key size of algorithm if applicable
        virtual size_t keySize() const override
        {
            return m_cipherAlg->keySize()+m_macAlg->keySize();
        }

        //! Get IV size of algorithm if applicable
        virtual size_t ivSize() const override
        {
            return m_cipherAlg->ivSize();
        }

        //! Get hash size of algorithm if applicable
        virtual size_t hashSize(bool safe) const override
        {
            return m_macAlg->hashSize(safe);
        }

        //! Get block size of algorithm if applicable
        virtual size_t blockSize() const override
        {
            return m_cipherAlg->blockSize();
        }

        virtual common::SharedPtr<SymmetricKey> createSymmetricKey() const override;

        virtual bool isBackendAlgorithm() const override
        {
            return false;
        }

        virtual size_t tagSize() const override
        {
            return m_tagSize;
        }
        virtual void setTagSize(size_t size) override
        {
            m_tagSize=size;
        }

    private:

        const CryptAlgorithm* m_cipherAlg;
        const CryptAlgorithm* m_macAlg;

        size_t m_tagSize;
};

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
common::Error EncryptMAC<Encrypt>::verifyDecrypt(
        const common::SpanBuffer& iv,
        const CipherTextT& ciphertext,
        const AuthDataT& authdata,
        const common::SpanBuffer& tag,
        ContainerOutT& plaintext
    )
{
    if (Encrypt)
    {
        return cryptError(CryptError::INVALID_OPERATION);
    }
    else
    {
        plaintext.clear();

        common::SpanBuffers authVector{iv};
        common::SpanBuffer::append(authVector,authdata);
        common::SpanBuffer::append(authVector,ciphertext);
        HATN_CHECK_RETURN(m_mac->runVerify(authVector,tag))
        return m_cipher->run(iv,ciphertext,plaintext);
    }
    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
common::Error EncryptMAC<Encrypt>::verifyDecryptPack(
        const CipherTextT& ciphertext,
        const AuthDataT& authdata,
        ContainerOutT& plaintext
    )
{
    if (Encrypt)
    {
        return cryptError(CryptError::INVALID_OPERATION);
    }
    else
    {
        plaintext.clear();

        // extract IV and tag
        common::Error ec;
        auto ivAndTag=detail::AeadArgTraits::extractIvAndTag(this,ciphertext,ec);
        HATN_CHECK_EC(ec)
        size_t offset=ivAndTag.first.size()+ivAndTag.second.size();

        // initialize mac
        HATN_CHECK_RETURN(m_mac->init())

        // process IV in mac
        HATN_CHECK_RETURN(m_mac->process(ivAndTag.first))

        // process auth data in mac
        HATN_CHECK_RETURN(m_mac->process(authdata))

        // process ciphertext in mac
        HATN_CHECK_RETURN(m_mac->process(ciphertext,offset))

        // verify mac
        HATN_CHECK_RETURN(m_mac->finalizeAndVerify(ivAndTag.second))

        // initialize decryptor
        HATN_CHECK_RETURN(m_cipher->init(ivAndTag.first))

        // decrypt data after offset
        HATN_CHECK_RETURN(detail::AeadArgTraits::decrypt(m_cipher.get(),ciphertext,plaintext,offset))

        // finalize
        size_t sizeOut=0;
        return m_cipher->finalize(plaintext,sizeOut,plaintext.size());
    }

    return common::Error();
}

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTENCRYPTMAC_H
