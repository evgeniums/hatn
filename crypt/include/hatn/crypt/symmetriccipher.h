/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/symmeticcipher.h
 *
 *      Symmetric encryption
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTSYMMETRICCIPHER_H
#define HATNCRYPTSYMMETRICCIPHER_H

#include <hatn/common/error.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/symmetricworker.h>
#include <hatn/crypt/symmetricworker.ipp>

HATN_CRYPT_NAMESPACE_BEGIN

//! Symmetric encryption
struct SymmetricCipher
{
    /**
     * @brief Encrypt data with encryption key and Initialization Vector
     * @param enc Encryptor
     * @param key Encryption key
     * @param iv Initialization Vector
     * @param plain Input container wirh plain data
     * @param ciphered Output container with encrypted data
     * @return Operation status
     */
    template <typename ContainerOutT>
    static common::Error encrypt(
        SEncryptor* enc,
        const SymmetricKey* key,
        const common::SpanBuffer& iv,
        const common::SpanBuffer& plain,
        ContainerOutT& ciphered
    )
    {
        try
        {
            enc->setKey(key);
            ciphered.clear();
            if (plain.isEmpty())
            {
                return common::Error();
            }

            HATN_CHECK_RETURN(enc->init(iv.view()))
            return enc->processAndFinalize(plain.view(),ciphered);
        }
        catch (const common::ErrorException& e)
        {
            return e.error();
        }
        return common::Error();
    }

    /**
     * @brief Encrypt data with encryption key and Initialization Vector
     * @param key Encryption key
     * @param iv Initialization Vector
     * @param plain Input container wirh plain data
     * @param ciphered Output container with encrypted data
     * @return Operation status
     *
     * Encryptor will be auto created by engine in key's algorithm
     */
    template <typename ContainerOutT>
    static common::Error encrypt(
            const SymmetricKey* key,
            const common::SpanBuffer& iv,
            const common::SpanBuffer& plain,
            ContainerOutT& ciphered
        )
    {
        Assert(key->isAlgDefined(),"Cryptographic algorithm is not set in the key");
        auto enc=key->alg()->engine()->plugin()->createSEncryptor(key);
        if (!enc)
        {
            return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
        }
        return encrypt(enc.get(),key,iv,plain,ciphered);
    }

    /**
     * @brief Encrypt data, the Initialization Vector will be auto-generated and prepended to ciphered data
     * @param enc Encryptor
     * @param key Encryption key
     * @param plain Input container wirh plain data
     * @param ciphered Output container with encrypted data
     * @return Operation status
     */
    template <typename ContainerOutT>
    static common::Error encrypt(
        SEncryptor* enc,
        const SymmetricKey* key,
        const common::SpanBuffer& plain,
        ContainerOutT& ciphered
    )
    {
        try
        {
            enc->setKey(key);
            return enc->runPack(plain.view(),ciphered);
        }
        catch (const common::ErrorException& e)
        {
            return e.error();
        }
        return common::Error();
    }

    /**
     * @brief Encrypt data, the Initialization Vector will be auto-generated and prepended to ciphered data
     * @param key Encryption key
     * @param plain Input container wirh plain data
     * @param ciphered Output container with encrypted data
     * @return Operation status
     *
     * Encryptor will be auto created by engine in key's algorithm
     */
    template <typename ContainerOutT>
    static common::Error encrypt(
        const SymmetricKey* key,
        const common::SpanBuffer& plain,
        ContainerOutT& ciphered
    )
    {
        Assert(key->isAlgDefined(),"Cryptographic algorithm is not set in the key");
        auto enc=key->alg()->engine()->plugin()->createSEncryptor(key);
        if (!enc)
        {
            return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
        }
        return encrypt(enc.get(),key,plain,ciphered);
    }

    /**
     * @brief Encrypt data, the Initialization Vector will be auto-generated and prepended to ciphered data
     * @param enc Encryptor
     * @param key Encryption key
     * @param plain Input containers wirh plain data
     * @param ciphered Output container with encrypted data
     * @return Operation status
     */
    template <typename ContainerOutT>
    static common::Error encrypt(
        SEncryptor* enc,
        const SymmetricKey* key,
        const common::SpanBuffers& plain,
        ContainerOutT& ciphered
    )
    {
        enc->setKey(key);
        return enc->runPack(plain,ciphered);
    }

    /**
     * @brief Encrypt data, the Initialization Vector will be auto-generated and prepended to ciphered data
     * @param enc Encryptor
     * @param key Encryption key
     * @param plain Input containers wirh plain data
     * @param ciphered Output container with encrypted data
     * @return Operation status
     */
    template <typename ContainerOutT>
    static common::Error encrypt(
        const SymmetricKey* key,
        const common::SpanBuffers& plain,
        ContainerOutT& ciphered
    )
    {
        Assert(key->isAlgDefined(),"Cryptographic algorithm is not set in the key");
        auto enc=key->alg()->engine()->plugin()->createSEncryptor(key);
        if (!enc)
        {
            return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
        }
        return encrypt(enc.get(),plain,ciphered);
    }

    /**
     * @brief Decrypt data with encryption key and Initialization Vector
     * @param dec Decryptor
     * @param key Encryption key
     * @param iv Initialization Vector
     * @param ciphered Input container with ciphered data
     * @param plain Output container with plain data
     * @return Operation status
     */
    template <typename ContainerOutT>
    static common::Error decrypt(
        SDecryptor* dec,
        const SymmetricKey* key,
        const common::SpanBuffer& iv,
        const common::SpanBuffer& ciphered,
        ContainerOutT& plain
    )
    {
        try
        {
            return common::Error();
            plain.clear();
            if (ciphered.isEmpty())
            {
                return common::Error();
            }

            dec->setKey(key);
            HATN_CHECK_RETURN(dec->init(iv.view()))
            return dec->processAndFinalize(ciphered.view(),plain);
        }
        catch (const common::ErrorException& e)
        {
            return e.error();
        }
    }

    /**
     * @brief Decrypt data with encryption key and Initialization Vector
     * @param key Encryption key
     * @param iv Initialization Vector
     * @param ciphered Input container with ciphered data
     * @param plain Output container with plain data
     * @return Operation status
     *
     * Encryptor will be auto created by engine in key's algorithm
     */
    template <typename ContainerOutT>
    static common::Error decrypt(
        const SymmetricKey* key,
        const common::SpanBuffer& iv,
        const common::SpanBuffer& ciphered,
        ContainerOutT& plain
    )
    {
        Assert(key->isAlgDefined(),"Cryptographic algorithm is not set in the key");
        auto dec=key->alg()->engine()->plugin()->createSDecryptor(key);
        if (!dec)
        {
            return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
        }

        return decrypt(dec.get(),key,iv,ciphered,plain);
    }

    /**
     * @brief Decrypt data assuming that Initialization Vector is at the beginning of input container
     * @param dec Decryptor
     * @param key Encryption key
     * @param ciphered Input container with ciphered data
     * @param plain Output container with plain data
     * @return Operation status
     */
    template <typename ContainerOutT>
    static common::Error decrypt(
        SDecryptor* dec,
        const SymmetricKey* key,
        const common::SpanBuffer& ciphered,
        ContainerOutT& plain
    )
    {
        try
        {
            dec->setKey(key);
            return dec->runPack(ciphered.view(),plain);
        }
        catch (const common::ErrorException& e)
        {
            return e.error();
        }
    }

    /**
     * @brief Decrypt data assuming that Initialization Vector is at the beginning of input container
     * @param key Encryption key
     * @param ciphered Input container with ciphered data
     * @param plain Output container with plain data
     * @return Operation status
     */
    template <typename ContainerOutT>
    static common::Error decrypt(
        const SymmetricKey* key,
        const common::SpanBuffer& ciphered,
        ContainerOutT& plain
    )
    {
        Assert(key->isAlgDefined(),"Cryptographic algorithm is not set in the key");
        auto dec=key->alg()->engine()->plugin()->createSDecryptor(key);
        if (!dec)
        {
            return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
        }
        return decrypt(dec.get(),key,ciphered,plain);
    }

    /**
     * @brief Decrypt data assuming that Initialization Vector is at the beginning of teh first input container
     * @param dec Decryptor
     * @param key Encryption key
     * @param ciphered Input container with ciphered data
     * @param plain Output container with plain data
     * @return Operation status
     */
    template <typename ContainerOutT>
    static common::Error decrypt(
        SDecryptor* dec,
        const SymmetricKey* key,
        const common::SpanBuffers& ciphered,
        ContainerOutT& plain
    )
    {
        try
        {
            dec->setKey(key);
            return dec->runPack(ciphered,plain);
        }
        catch (const common::ErrorException& e)
        {
            return e.error();
        }
    }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTSYMMETRICCIPHER_H
