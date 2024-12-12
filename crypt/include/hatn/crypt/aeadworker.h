/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/aeadworker.h
 *
 *      Base classes for implementation of AEAD ciphers
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTAEADWORKER_H
#define HATNCRYPTAEADWORKER_H

#include <hatn/crypt/cipherworker.h>

HATN_CRYPT_NAMESPACE_BEGIN

/********************** AeadWorker **********************************/

//! Base template class for AEAD
template <bool Encrypt>
class AeadWorker : public CipherWorker<Encrypt>
{
    public:

        /**
         * @brief Ctor
         * @param key Symmetric key to be used for encryption/decryption
         *
         * Worker will use encryption algotithm as set in encryption key.
         *
         * @throws std::runtime_error if key doesn't have role ENCRYPT_SYMMETRIC
         */
        AeadWorker(const SymmetricKey* key=nullptr);

        /**
         * @brief Set tag for AEAD check in decryption if applicable
         * @param data Pointer to tag's buffer
         */
        common::Error setTag(const char* data) noexcept;

        /**
         * @brief Set tag for AEAD check in decryption if applicable
         * @param buf Tag's buffer
         * @param offset Offset in buffer
         * @param size Size of tag, if zero then use buf.size()-offset
         */
        template <typename ContainerT>
        common::Error setTag(const ContainerT& buf, size_t offset=0) noexcept;

        common::Error setTag(const common::SpanBuffer& buf) noexcept;

        /**
         * @brief Get size of AEAD tag
         * @return Tag size
         */
        size_t getTagSize() const noexcept;

        /**
         * @brief Get IV size for this encryption algorithm
         * @return IV size
         */
        virtual size_t ivSize() const noexcept override;

        /**
         * @brief Get tag after encryption to use later for AEAD check if applicable
         * @param data Pointer to tag's buffer
         *
         * Size of tag is fixed and can be found out by getAEADTagSize()
         *
         */
        common::Error getTag(char* data) noexcept;

        //! Get maximum extra size that can be added to size of plain text
        size_t maxExtraSize() const;

        /**
         * @brief Get tag after encryption to use later for AEAD check if applicable
         * @param tag Buffer for output result
         * @param offset Offset in output buffer starting from which to put result
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error getTag(
            ContainerT& tag,
            size_t offset=0
        );

        /**
         * @brief Encrypt data using pre-initialized encryptor
         * @param plaintext Data to encrypt, can be either single SpanBuffer or multiple SpanBuffers
         * @param authdata Not encrypted data to attach for authentication, can be either single SpanBuffer or multiple SpanBuffers
         * @param ciphertext Encrypted result
         * @param tag Authentication flag
         * @return Operation status
         */
        template <typename ContainerOutT, typename PlainTextT, typename AuthDataT, typename ContainerTagT>
        common::Error encryptAndFinalize(
            const PlainTextT& plaintext,
            const AuthDataT& authdata,
            ContainerOutT& ciphertext,
            ContainerTagT& tag
        );

        /**
         * @brief Encrypt data and pack IV and AEAD tag with ciphertext
         * @param plaintext Data to encrypt, can be either single SpanBuffer or multiple SpanBuffers
         * @param authdata Not encrypted data to attach for authentication, can be either single SpanBuffer or multiple SpanBuffers
         * @param ciphertext Encrypted result
         * @param iv Initialization vector. If empty then IV will be auto-generated. If less than required then will be zero-padded.
         * @param offsetOut Offset in the target buffer
         * @return Operation status
         *
         * Payload of result buffer is as follows:
         * <pre>
         * Section      | Length in bytes
         * ____________ | _______________
         *              |
         * MAC Tag      | length of HMAC digest defined by the hash algotithm that was used
         * IV           | length of Initialization Vector pre-defined (or recommended) by used encryption algorithm
         * ciphertext   | length of ciphertext resulted from encryption
         * </pre>
         */
        template <typename ContainerOutT, typename PlainTextT, typename AuthDataT>
        common::Error encryptPack(
            const PlainTextT& plaintext,
            const AuthDataT& authdata,
            ContainerOutT& ciphertext,
            const common::SpanBuffer& iv=common::SpanBuffer(),
            size_t offsetOut=0
        );

        /**
         * @brief Decrypt data using pre-initialized decryptor
         * @param ciphertext Encrypted data, can be either single SpanBuffer or multiple SpanBuffers
         * @param authdata Not encrypted data attached for authentication, can be either single SpanBuffer or multiple SpanBuffers
         * @param tag AEAD tag
         * @param plaintext Decrypted result
         * @return Operation status
         */
        template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
        common::Error decryptAndFinalize(
            const CipherTextT& ciphertext,
            const AuthDataT& authdata,
            const common::SpanBuffer& tag,
            ContainerOutT& plaintext
        );

        /**
         * @brief Decrypt data taking IV and AEAD tag from the beggining of the buffer with ciphered data
         * @param ciphertext Encrypted data
         * @param authdata Not encrypted data attached for authentication
         * @param plaintext Decrypted result
         * @return Operation status
         */
        template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
        common::Error decryptPack(
            const CipherTextT& ciphertext,
            const AuthDataT& authdata,
            ContainerOutT& plaintext
        );

        //! Start processing not encrypted data attached for authentication
        virtual void beginNotEncrypted() =0;
        //! Finish processing not encrypted data attached for authentication
        virtual void endNotEncrypted() =0;

    protected:

        virtual common::Error doSetTag(const char* data) noexcept
        {
            std::ignore=data;
            return cryptError(CryptError::INVALID_OPERATION);
        }

        virtual common::Error doGetTag(char* data) noexcept
        {
            std::ignore=data;
            return cryptError(CryptError::INVALID_OPERATION);
        }
};

using AEADEncryptor=AeadWorker<true>;
using AEADDecryptor=AeadWorker<false>;

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTAEADWORKER_H
