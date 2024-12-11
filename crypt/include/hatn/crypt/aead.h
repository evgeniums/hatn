/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/aead.h
 *
 *      AEAD functions
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTAEAD_H
#define HATNCRYPTAEAD_H

#include <hatn/common/error.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/aeadworker.h>
#include <hatn/crypt/aeadworker.ipp>

HATN_CRYPT_NAMESPACE_BEGIN

//! AEAD functions
struct AEAD
{
    /**
     * @brief Encrypt data in AEAD mode
     * @param enc Encryptor
     * @param iv Initialization vector
     * @param plaintext Data to encrypt, can be either single SpanBuffer or multiple SpanBuffers
     * @param authdata Not encrypted data to attach for authentication, can be either single SpanBuffer or multiple SpanBuffers
     * @param ciphertext Encrypted result
     * @param tag Authentication flag
     * @return Operation status
     */
    template <typename ContainerOutT,typename ContainerTagT, typename PlainTextT, typename AuthDataT>
    static common::Error encrypt(
        AEADEncryptor* enc,
        const common::SpanBuffer& iv,
        const PlainTextT& plaintext,
        const AuthDataT& authdata,
        ContainerOutT& ciphertext,
        ContainerTagT& tag
    )
    {
        try
        {
            ciphertext.clear();
            HATN_CHECK_RETURN(enc->init(iv.view()))
            return enc->encryptAndFinalize(plaintext,authdata,ciphertext,tag);
        }
        catch (const common::ErrorException& e)
        {
            return e.error();
        }
        return common::Error();
    }

    /**
     * @brief Encrypt data in AEAD mode
     * @param enc Encryptor
     * @param key Encryption key
     * @param iv Initialization vector
     * @param plaintext Data to encrypt, can be either single SpanBuffer or multiple SpanBuffers
     * @param authdata Not encrypted data to attach for authentication, can be either single SpanBuffer or multiple SpanBuffers
     * @param ciphertext Encrypted result
     * @param tag Authentication flag
     * @return Operation status
     */
    template <typename ContainerOutT,typename ContainerTagT, typename PlainTextT, typename AuthDataT>
    static common::Error encrypt(
        AEADEncryptor* enc,
        const SymmetricKey* key,
        const common::SpanBuffer& iv,
        const PlainTextT& plaintext,
        const AuthDataT& authdata,
        ContainerOutT& ciphertext,
        ContainerTagT& tag
    )
    {
        enc->setKey(key);
        return encrypt(enc,iv,plaintext,authdata,ciphertext,tag);
    }

    /**
     * @brief Encrypt data and pack IV and AEAD tag with ciphertext
     * @param enc Encryptor
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
     *
     */
    template <typename ContainerOutT, typename PlainTextT, typename AuthDataT>
    static common::Error encryptPack(
        AEADEncryptor* enc,
        const PlainTextT& plaintext,
        const AuthDataT& authdata,
        ContainerOutT& ciphertext,
        const common::SpanBuffer& iv=common::SpanBuffer(),
        size_t offsetOut=0
    )
    {
        return enc->encryptPack(plaintext,authdata,ciphertext,iv,offsetOut);
    }

    /**
     * @brief Encrypt data and pack IV and AEAD tag with ciphertext
     * @param enc Encryptor
     * @param key Encryption key
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
     *
     */
    template <typename ContainerOutT, typename PlainTextT, typename AuthDataT>
    static common::Error encryptPack(
        AEADEncryptor* enc,
        const SymmetricKey* key,
        const PlainTextT& plaintext,
        const AuthDataT& authdata,
        ContainerOutT& ciphertext,
        const common::SpanBuffer& iv=common::SpanBuffer(),
        size_t offsetOut=0
    )
    {
        enc->setKey(key);
        return encryptPack(enc,plaintext,authdata,ciphertext,iv,offsetOut);
    }

    /**
     * @brief Decrypt data in AEAD mode
     * @param dec Decryptor
     * @param iv Initialization vector
     * @param ciphertext Encrypted data, can be either single SpanBuffer or multiple SpanBuffers
     * @param authdata Not encrypted data attached for authentication, can be either single SpanBuffer or multiple SpanBuffers
     * @param tag AEAD tag
     * @param plaintext Decrypted result
     * @return Operation status
     */
    template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
    static common::Error decrypt(
        AEADDecryptor* dec,
        const common::SpanBuffer& iv,
        const CipherTextT& ciphertext,
        const AuthDataT& authdata,
        const common::SpanBuffer& tag,
        ContainerOutT& plaintext
    )
    {
        plaintext.clear();
        HATN_CHECK_RETURN(dec->init(iv))
        return dec->decryptAndFinalize(ciphertext,authdata,tag,plaintext);
    }

    /**
     * @brief Decrypt data in AEAD mode
     * @param dec Decryptor
     * @param key Encryption key
     * @param iv Initialization vector
     * @param ciphertext Encrypted data, can be either single SpanBuffer or multiple SpanBuffers
     * @param authdata Not encrypted data attached for authentication, can be either single SpanBuffer or multiple SpanBuffers
     * @param tag AEAD tag
     * @param plaintext Decrypted result
     * @return Operation status
     */
    template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
    static common::Error decrypt(
        AEADDecryptor* dec,
        const SymmetricKey* key,
        const common::SpanBuffer& iv,
        const CipherTextT& ciphertext,
        const AuthDataT& authdata,
        const common::SpanBuffer& tag,
        ContainerOutT& plaintext
    )
    {
        dec->setKey(key);
        return decrypt(dec,iv,ciphertext,authdata,tag,plaintext);
    }

    /**
     * @brief Decrypt data taking IV and AEAD tag from the beggining of the buffer with ciphered data
     * @param dec Decryptor
     * @param ciphertext Encrypted data, can be either single SpanBuffer or multiple SpanBuffers
     * @param authdata Not encrypted data attached for authentication, can be either single SpanBuffer or multiple SpanBuffers
     * @param plaintext Decrypted result
     * @return Operation status
     */
    template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
    static common::Error decryptPack(
        AEADDecryptor* dec,
        const CipherTextT& ciphertext,
        const AuthDataT& authdata,
        ContainerOutT& plaintext
    )
    {
        return dec->decryptPack(ciphertext,authdata,plaintext);
    }

    /**
     * @brief Decrypt data taking IV and AEAD tag from the beggining of the buffer with ciphered data
     * @param dec Decryptor
     * @param key Encryption key
     * @param ciphertext Encrypted data, can be either single SpanBuffer or multiple SpanBuffers
     * @param authdata Not encrypted data attached for authentication, can be either single SpanBuffer or multiple SpanBuffers
     * @param plaintext Decrypted result
     * @return Operation status
     */
    template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
    static common::Error decryptPack(
        AEADDecryptor* dec,
        const SymmetricKey* key,
        const CipherTextT& ciphertext,
        const AuthDataT& authdata,
        ContainerOutT& plaintext
    )
    {
        dec->setKey(key);
        return decryptPack(dec,ciphertext,authdata,plaintext);
    }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTAEAD_H
