/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/keyprotector.h
 *
 *      Protector of secret keys
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTKEYPROTECTOR_H
#define HATNCRYPTKEYPROTECTOR_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/passphrasekey.h>
#include <hatn/crypt/aead.h>
#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/cryptcontainer.h>

HATN_CRYPT_NAMESPACE_BEGIN

class PassphraseKey;
class CryptAlgorithm;

/**
 * @brief Protector of secret keys.
 *
 * Normally, protector only holds either passhrase key of PassphraseKey type or symmetric key of SymmetricKey type
 * that must be used by backend for key importing/exporting.
 *
 * Protector also implements default key protection scheme using CryptContainer to encrypt/decrypt secret keys.
 * In this case PassphraseKey or SymmetricKey are used by CryptContainer embedded into the Protector.
 * If needed, backends can use this default implementation to protect secret keys of some types.
 *
 * For example, the openssl plugin protects PEM keys by forwarding content of PassphraseKey to corresponding functions
 * of openssl library, whilst symmetric keys are encrypted using default protection scheme of the Protector.
 */
class KeyProtector
{
    public:

        /**
         * @brief Ctor
         * @param passphrase Passphrase to use for protection
         * @param suite Cipher suite
         * @param factory Allocator factory
         */
        KeyProtector(
                common::SharedPtr<PassphraseKey> passphrase,
                const CipherSuite *suite,
                const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) noexcept : m_passphrase(std::move(passphrase)),
                     m_impl(m_passphrase.get(),suite,factory)
        {
            reset();
        }

        /**
         * @brief Ctor
         * @param masterKey Symmetric key to use for key encryption/decryption
         * @param suite Cipher suite
         * @param factory Allocator factory
         */
        KeyProtector(
                common::SharedPtr<SymmetricKey> symmetricKey,
                const CipherSuite *suite,
                const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) noexcept : m_symmetricKey(std::move(symmetricKey)),
                     m_impl(m_symmetricKey.get(),suite,factory)
        {
            reset();
        }

        /**
         * @brief Ctor
         * @param factory Allocator factory
         */
        KeyProtector(
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : KeyProtector(common::SharedPtr<PassphraseKey>(),nullptr,factory)
        {}

        /**
         * @brief Ctor
         * @param suite Cipher suite
         * @param factory Allocator factory
         */
        KeyProtector(
                const CipherSuite *suite,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : KeyProtector(common::SharedPtr<PassphraseKey>(),suite,factory)
        {}

        ~KeyProtector()=default;
        KeyProtector(const KeyProtector&)=delete;
        KeyProtector(KeyProtector&&) =delete;
        KeyProtector& operator=(const KeyProtector&)=delete;
        KeyProtector& operator=(KeyProtector&&) =delete;

        /**
         * @brief Encrypt raw data of secret key using default scheme.
         * @param containerIn Raw data of secret key.
         * @param containerOut Encrypted result.
         * @return Operation status.
         */
        template <typename ContainerInT, typename ContainerOutT>
        common::Error pack(
            const ContainerInT& containerIn,
            ContainerOutT& containerOut
        )
        {
            common::RunOnScopeExit guard(
                [this]()
                {
                    reset();
                }
            );

            HATN_CHECK_RETURN(checkReady())

            return m_impl.pack(containerIn,containerOut);
        }

        /**
         * @brief Decrypt raw data of secret key using default scheme.
         * @param containerIn Encrypted secret key.
         * @param containerOut Raw data of secret key.
         * @return Operation status.
         */
        template <typename ContainerInT, typename ContainerOutT>
        common::Error unpack(
            const ContainerInT& containerIn,
            ContainerOutT& containerOut
        )
        {
            common::RunOnScopeExit guard(
                [this]()
                {
                    reset();
                }
            );

            HATN_CHECK_RETURN(checkReady())
            return m_impl.unpack(containerIn,containerOut);
        }

        //! Check if the passphrase is valid
        inline bool isPassphraseValid() const noexcept
        {
            return !m_passphrase.isNull();
        }

        //! Alias for isSymmetricKeyValid()
        inline bool isHkdfMasterKeyValid() const noexcept
        {
            return isSymmetricKeyValid();
        }

        //! Check if the symmetric key is valid
        inline bool isSymmetricKeyValid() const noexcept
        {
            return !m_symmetricKey.isNull();
        }

        /**
         * @brief Set passphrase key
         * @param passphrase
         *
         * @throws common::ErrorException if initialization failed
         */
        inline void setPassphrase(common::SharedPtr<PassphraseKey> passphrase)
        {
            m_symmetricKey.reset();
            m_passphrase=std::move(passphrase);
            m_impl.setMasterKey(m_passphrase.get());
            reset();
        }
        //! Get passphrase key
        inline PassphraseKey* passphrase() const noexcept
        {
            return m_passphrase.get();
        }

        //! Alias for setSymmetricKey()
        inline void setHkdfMasterKey(common::SharedPtr<SymmetricKey> key)
        {
            setSymmetricKey(std::move(key));
        }
        //! Alias for symmetricKey()
        inline SymmetricKey* hkdfMasterKey() const noexcept
        {
            return symmetricKey();
        }

        /**
         * @brief Set symmetric key used to encrypt/decrypt keys.
         * @param key Symmetric encryption key.
         *
         * @throws common::ErrorException if initialization failed
         */
        inline void setSymmetricKey(common::SharedPtr<SymmetricKey> key)
        {
            m_passphrase.reset();
            m_impl.setMasterKey(key.get());
            m_symmetricKey=std::move(key);
            reset();
        }
        //! Get symmetric key
        inline SymmetricKey* symmetricKey() const noexcept
        {
            return m_symmetricKey.get();
        }

        inline common::Error checkReady() const noexcept
        {
            if (!isPassphraseValid() && !isHkdfMasterKeyValid())
            {
                return cryptError(CryptError::INVALID_KEY_PROTECTION);
            }
            return m_impl.checkState();
        }

        container_descriptor::KdfType kdfType() const noexcept
        {
            return m_passphrase.isNull() ? container_descriptor::KdfType::HKDF : container_descriptor::KdfType::PBKDF;
        }

        const CryptContainer& cryptContainer() const
        {
            return m_impl;
        }

    private:

        inline void reset() noexcept
        {
            m_impl.setKdfType(kdfType());
            m_impl.setChunkMaxSize(0);
            m_impl.setFirstChunkMaxSize(0);
        }

        common::SharedPtr<PassphraseKey> m_passphrase;
        common::SharedPtr<SymmetricKey> m_symmetricKey;

        CryptContainer m_impl;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTKEYPROTECTOR_H
