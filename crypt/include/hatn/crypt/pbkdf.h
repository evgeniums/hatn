/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/pbkdf.h
 *
 *      Base class for Password Based Key Derivation Functions (PBKDF)
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTPBKDF_H
#define HATNCRYPTPBKDF_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/kdf.h>

HATN_CRYPT_NAMESPACE_BEGIN

class PassphraseKey;

//! Base class for Password Based Key Derivation Functions
class PBKDF : public KDF
{
    public:

        /**
         * @brief Ctor
         * @param targetKeyAlg Algorithm the derived keys will be used with
         * @param kdfAlg KDF algorithm, if null then default PBKDF2-SHA1 must be used in derived class
         */
        PBKDF(const CryptAlgorithm* targetKeyAlg, const CryptAlgorithm* kdfAlg=nullptr) noexcept
            : KDF(targetKeyAlg,kdfAlg)
        {}

        /**
         * @brief Derive key from password kept in null-terminated c-string
         * @param passwdData Password, null-terminated c-string
         * @param key Derived key
         * @param salt Salt to be used for key derivation
         * @return Operation status
         */
        template <typename SaltContainerT>
        common::Error derive(
            const char* passwdData,
            common::SharedPtr<SymmetricKey>& key,
            const SaltContainerT& salt
        )
        {
            return derive(passwdData,strlen(passwdData),key,salt);
        }

        /**
         * @brief Derive key from password kept in null-terminated c-string
         * @param passwdData Password, null-terminated c-string
         * @param key Derived key
         * @param salt Salt to be used for key derivation
         * @return Operation status
         */
        common::Error derive(
            const char* passwdData,
            common::SharedPtr<SymmetricKey>& key
        )
        {
            return derive(passwdData,strlen(passwdData),key,common::ConstDataBuf());
        }

        /**
         * @brief Derive key from content of master key
         * @param masterKey Master key
         * @param key Derived key
         * @param salt Salt to be used for key derivation
         * @return Operation status
         */
        template <typename SaltContainerT>
        common::Error derive(
            const SecureKey* masterKey,
            common::SharedPtr<SymmetricKey>& key,
            const SaltContainerT& salt
        )
        {
            return derive(masterKey->content().data(),masterKey->content().size(),key,salt);
        }

        /**
         * @brief Derive key from content of master key
         * @param masterKey Master key
         * @param key Derived key
         * @return Operation status
         */
        common::Error derive(
            const SecureKey* masterKey,
            common::SharedPtr<SymmetricKey>& key
        )
        {
            return derive(masterKey->content().data(),masterKey->content().size(),key,common::ConstDataBuf());
        }

        /**
         * @brief Derive key from data buffer with password
         * @param passwdData Data buffer with password
         * @param passwdLength Length of password
         * @param key Derived key
         * @param salt Salt to be used for key derivation
         * @return Operation status
         */
        template <typename SaltContainerT>
        common::Error derive(
            const char* passwdData,
            size_t passwdLength,
            common::SharedPtr<SymmetricKey>& key,
            const SaltContainerT& salt
        )
        {
            return doDerive(passwdData,passwdLength,key,salt.data(),salt.size());
        }

        /**
         * @brief Derive key from data buffer with password
         * @param passwdData Data buffer with password
         * @param passwdLength Length of password
         * @param key Derived key
         * @return Operation status
         */
        common::Error derive(
            const char* passwdData,
            size_t passwdLength,
            common::SharedPtr<SymmetricKey>& key
        )
        {
            return derive(passwdData,passwdLength,key,common::ConstDataBuf());
        }

        /**
         * @brief Derive key for passphrase and set it into parent key as derived key
         * @param passphrase Passphrase key
         * @param salt Salt to be used for key derivation
         * @return Operation status
         */
        template <typename SaltContainerT>
        static common::Error derive(
            PassphraseKey* passphrase,
            const SaltContainerT& salt
        );

        /**
         * @brief Derive key for passphrase and set it into parent key as derived key
         * @param passphrase Passphrase key
         * @return Operation status
         */
        static common::Error derive(
            PassphraseKey* passphrase
        )
        {
            return derive(passphrase,common::ConstDataBuf());
        }

    protected:

        /**
         * @brief Derive key from data buffer with password
         * @param passwdData Data buffer with password
         * @param passwdLength Length of password
         * @param key Derived key
         * @param saltData Salt buffer
         * @param saltLength Salt length
         * @return Operation status
         */
        virtual common::Error doDerive(
            const char* passwdData,
            size_t passwdLength,
            common::SharedPtr<SymmetricKey>& key,
            const char* saltData,
            size_t saltLength
        ) =0;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTPBKDF_H
