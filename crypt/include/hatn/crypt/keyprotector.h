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
 * @brief Protector of secret keys
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
                common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) noexcept : m_passphrase(std::move(passphrase)),
                     m_impl(m_passphrase.get(),suite,factory)
        {
            reset();
        }

        /**
         * @brief Ctor
         * @param masterKey Master key to use for protection with HKDF algorithm
         * @param suite Cipher suite
         * @param factory Allocator factory
         */
        KeyProtector(
                common::SharedPtr<SymmetricKey> masterKey,
                const CipherSuite *suite,
                common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) noexcept : m_hkdfMasterKey(std::move(masterKey)),
                     m_impl(m_hkdfMasterKey.get(),suite,factory)
        {
            reset();
        }

        /**
         * @brief Ctor
         * @param factory Allocator factory
         */
        KeyProtector(
                common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : KeyProtector(common::SharedPtr<PassphraseKey>(),nullptr,factory)
        {}

        /**
         * @brief Ctor
         * @param suite Cipher suite
         * @param factory Allocator factory
         */
        KeyProtector(
                const CipherSuite *suite,
                common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : KeyProtector(common::SharedPtr<PassphraseKey>(),suite,factory)
        {}

        ~KeyProtector()=default;
        KeyProtector(const KeyProtector&)=delete;
        KeyProtector(KeyProtector&&) =delete;
        KeyProtector& operator=(const KeyProtector&)=delete;
        KeyProtector& operator=(KeyProtector&&) =delete;

        /**
         * @brief Encrypt key
         * @param containerIn Input container
         * @param containerOut Output contaiiner
         * @return Operation status
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

            common::ByteArray salt;
            HATN_CHECK_RETURN(CipherSuites::instance().defaultPlugin()->randContainer(salt,16,8));
            common::ConstDataBuf saltBuf(salt);

            return m_impl.pack(containerIn,containerOut,saltBuf);
        }

        /**
         * @brief Decrypt key
         * @param containerIn Input container
         * @param containerOut Output contaiiner
         * @return Operation status
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

        //! Check if the HKDF master key is valid
        inline bool isHkdfMasterKeyValid() const noexcept
        {
            return !m_hkdfMasterKey.isNull();
        }

        /**
         * @brief Set passphrase key
         * @param passphrase
         *
         * @throws common::ErrorException if initialization failed
         */
        inline void setPassphrase(common::SharedPtr<PassphraseKey> passphrase)
        {
            m_hkdfMasterKey.reset();
            m_passphrase=std::move(passphrase);
            m_impl.setMasterKey(m_passphrase.get());
            reset();
        }
        //! Get passphrase key
        inline PassphraseKey* passphrase() const noexcept
        {
            return m_passphrase.get();
        }

        /**
         * @brief Set hkdf master key
         * @param key Master key for hkdf algorithm
         *
         * @throws common::ErrorException if initialization failed
         */
        inline void setHkdfMasterKey(common::SharedPtr<SymmetricKey> key)
        {
            m_passphrase.reset();
            m_impl.setMasterKey(key.get());
            m_hkdfMasterKey=std::move(key);
            reset();
        }
        //! Get passphrase key
        inline SymmetricKey* hkdfMasterKey() const noexcept
        {
            return m_hkdfMasterKey.get();
        }

        inline common::Error checkReady() const noexcept
        {
            if (!isPassphraseValid() && !isHkdfMasterKeyValid())
            {
                return makeCryptError(CryptErrorCode::INVALID_KEY_PROTECTION);
            }
            return m_impl.checkState();
        }

        container_descriptor::KdfType kdfType() const noexcept
        {
            return m_passphrase.isNull()?container_descriptor::KdfType::HKDF:container_descriptor::KdfType::PBKDF;
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
        common::SharedPtr<SymmetricKey> m_hkdfMasterKey;

        CryptContainer m_impl;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTKEYPROTECTOR_H
