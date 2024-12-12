/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/encrypthmac.h
 *
 *      Implementing protection of data block using Encrypt-Then-HMAC
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTENCRYPTHMAC_H
#define HATNCRYPTENCRYPTHMAC_H

#include <hatn/common/bytearray.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/digest.h>
#include <hatn/crypt/cipher.h>

HATN_CRYPT_NAMESPACE_BEGIN

/**
 * @brief Implements protection of data block using Encrypt-Then-HMAC.
 *
 *
 * Data is first encrypted then HMAC on ciphertext is calculated.
 * Format of result buffer is as follows:
 * <pre>
 * Section      | Length in bytes
 * ____________ | _______________
 *              |
 * MAC Tag      | length of HMAC digest defined by the hash algotithm that was used
 * IV           | length of Initialization Vector pre-defined (or recommended) by used encryption algorithm
 * ciphertext   | length of ciphertext resulted from encryption
 * </pre>
 *
 * @deprecated Legacy class, not recommended for use. Use EncryptMac and AEAD classes instead.
 */
class EncryptHMAC
{
    public:

        /**
         * @brief Ctor
         * @param encryptionKey Key used for encryption and decryption
         * @param macKey Key used for HMAC
         *
         * @throws ErrorException if can't create processors (encryptor,decryptor,hmac)
         */
        EncryptHMAC(
                common::SharedPtr<SymmetricKey> cipherKey,
                common::SharedPtr<MACKey> macKey
            ) : m_cipherKey(std::move(cipherKey)),
                m_macKey(std::move(macKey)),
                m_enc(m_cipherKey->alg()->engine()->plugin()->createSEncryptor(m_cipherKey.get())),
                m_dec(m_cipherKey->alg()->engine()->plugin()->createSDecryptor(m_cipherKey.get()))
        {
            if (m_enc.isNull())
            {
                throw common::ErrorException(cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN));
            }
            if (m_dec.isNull())
            {
                throw common::ErrorException(cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN));
            }

            if (m_macKey->alg())
            {
                m_hmac=m_macKey->alg()->engine()->plugin()->createHMAC();
            }
            else
            {
                m_hmac=m_cipherKey->alg()->engine()->plugin()->createHMAC();
            }
            if (m_hmac.isNull())
            {
                throw common::ErrorException(cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN));
            }
            m_hmac->setKey(m_macKey.get());
            if (m_macKey->alg())
            {
                setup();
            }
        }

        void setup(
                const CryptAlgorithm* hmacAlg=nullptr
            )
        {
            if (hmacAlg==nullptr)
            {
                hmacAlg=m_macKey->alg();
            }
            Assert(hmacAlg,"HMAC algorithm is not defined");
            m_hmac->setAlgorithm(hmacAlg);
        }

        /**
         * @brief Encrypt and HMAC data
         * @param containerIn Input container
         * @param containerOut Output contaiiner
         * @return
         */
        template <typename ContainerInT, typename ContainerOutT>
        common::Error pack(
            const ContainerInT& containerIn,
            ContainerOutT& containerOut
        ) const
        {
            Assert(m_hmac->alg(),"HMAC algorithm is not set, call setup() first");
            size_t tagSize=m_hmac->hashSize();
            containerOut.resize(tagSize);
            HATN_CHECK_RETURN(m_enc->runPack(containerIn,containerOut,0,0,tagSize))
            common::ByteArray tag;
            HATN_CHECK_RETURN(m_hmac->run(containerOut,tag,tagSize));
            std::copy(tag.data(),tag.data()+tag.size(),containerOut.data());
            return common::Error();
        }

        /**
         * @brief Check HMAC and decrypt data
         * @param containerIn Input container
         * @param containerOut Output contaiiner
         * @return
         */
        template <typename ContainerInT, typename ContainerOutT>
        common::Error unpack(
            const ContainerInT& containerIn,
            ContainerOutT& containerOut
        ) const
        {
            size_t tagSize=m_hmac->hashSize();
            if (containerIn.size()<tagSize)
            {
                return cryptError(CryptError::MAC_FAILED);
            }

            common::SpanBuffer buf(containerIn,tagSize);
            HATN_CHECK_RETURN(m_hmac->runVerify(buf,containerIn.data(),tagSize))

            return m_dec->runPack(containerIn,containerOut,tagSize);
        }

        inline const SymmetricKey* cipherKey() const noexcept
        {
            return m_cipherKey.get();
        }

        inline const SymmetricKey* macKey() const noexcept
        {
            return m_macKey.get();
        }

        void setCipherKey(common::SharedPtr<SymmetricKey> cipherKey)
        {
            Assert(cipherKey->alg()==m_cipherKey->alg(),"Algorithms of previous key and new key mismatch");
            m_cipherKey=std::move(cipherKey);
            m_enc->setKey(m_cipherKey.get());
            m_dec->setKey(m_cipherKey.get());
        }

        void setMacKey(common::SharedPtr<MACKey> macKey)
        {
            Assert(macKey->alg()==m_macKey->alg(),"Algorithms of previous key and new key mismatch");
            m_macKey=std::move(macKey);
            m_hmac->setKey(m_macKey.get());
        }

    private:

        common::SharedPtr<SymmetricKey> m_cipherKey;
        common::SharedPtr<MACKey> m_macKey;

        common::SharedPtr<SEncryptor> m_enc;
        common::SharedPtr<SDecryptor> m_dec;
        common::SharedPtr<HMAC> m_hmac;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTENCRYPTHMAC_H
