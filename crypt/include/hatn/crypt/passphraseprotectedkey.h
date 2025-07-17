/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/passphraseprotectedkeys.h
 *
 *      Handle keys protected with passphrase
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTPASSPHRASEPROTECTEDKEY_H
#define HATNCRYPTPASSPHRASEPROTECTEDKEY_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/keyprotector.h>

HATN_CRYPT_NAMESPACE_BEGIN

struct protectWithPassphraseT
{
    Error operator() (
            SecureKey* key,
            common::SharedPtr<PassphraseKey> passphraseKey,
            common::MemoryLockedArray& targetBuf,
            const CipherSuite *suite,
            const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) const
    {
        return invoke(key,std::move(passphraseKey),targetBuf,suite,factory);
    }

    template <typename PassphraseContainerT, typename SaltContainerT=std::string>
    Error operator() (
        SecureKey* key,
        const PassphraseContainerT& passphrase,
        common::MemoryLockedArray& targetBuf,
        const CipherSuite *suite,
        const SaltContainerT& salt={},
        const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) const
    {
        CryptAlgorithmConstP alg;
        auto ec=suite->aeadAlgorithm(alg);
        HATN_CHECK_EC(ec)
        auto passphraseKey=suite->createPassphraseKey(ec,alg);
        HATN_CHECK_EC(ec)
        passphraseKey->set(passphrase);
        if (!salt.empty())
        {
            passphraseKey->setSalt(salt);
        }
        return invoke(key,std::move(passphraseKey),targetBuf,suite,factory);
    }

    template <typename PassphraseContainerT, typename SaltContainerT=std::string>
    Error operator() (
        lib::string_view keyContent,
        const PassphraseContainerT& passphrase,
        common::MemoryLockedArray& targetBuf,
        const CipherSuite *suite,
        const SaltContainerT& salt={},
        const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) const
    {
        CryptAlgorithmConstP alg;
        auto ec=suite->aeadAlgorithm(alg);
        HATN_CHECK_EC(ec)
        auto key=suite->createPassphraseKey(ec,alg);
        HATN_CHECK_EC(ec)
        key->set(keyContent);
        return protectWithPassphraseT{}(key.get(),passphrase,targetBuf,suite,salt,factory);
    }

    template <typename PassphraseContainerT, typename SaltContainerT=std::string>
    Error operator() (
        common::MemoryLockedArray keyContent,
        const PassphraseContainerT& passphrase,
        common::MemoryLockedArray& targetBuf,
        const CipherSuite *suite,
        const SaltContainerT& salt={},
        const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) const
    {
        CryptAlgorithmConstP alg;
        auto ec=suite->aeadAlgorithm(alg);
        HATN_CHECK_EC(ec)
        auto key=suite->createPassphraseKey(ec,alg);
        HATN_CHECK_EC(ec)
        key->set(std::move(keyContent));
        return protectWithPassphraseT{}(key.get(),passphrase,targetBuf,suite,salt,factory);
    }

    private:

        static Error invoke(
                SecureKey* key,
                common::SharedPtr<PassphraseKey> passphraseKey,
                common::MemoryLockedArray& targetBuf,
                const CipherSuite *suite,
                const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
            )
        {
            KeyProtector protector{std::move(passphraseKey),suite,factory};
            key->setProtector(&protector);
            auto ec=key->exportToBuf(targetBuf,ContainerFormat::RAW_ENCRYPTED);
            HATN_CHECK_EC(ec)
            return OK;
        }
};
constexpr protectWithPassphraseT protectWithPassphrase{};

struct unpackWithPassphraseT
{
    template <typename KeyContainerT>
    Error operator() (
            const KeyContainerT& keyContent,
            common::SharedPtr<PassphraseKey> passphraseKey,
            SymmetricKey* key,
            const CipherSuite *suite,
            const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) const
    {
        return invoke(keyContent,std::move(passphraseKey),key,suite,factory);
    }

    template <typename KeyContainerT, typename PassphraseContainerT, typename SaltContainerT=std::string>
    Error operator() (
        const KeyContainerT& keyContent,
        const PassphraseContainerT& passphrase,
        SymmetricKey* key,
        const CipherSuite *suite,
        const SaltContainerT& salt={},
        const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
        ) const
    {
        CryptAlgorithmConstP alg;
        auto ec=suite->aeadAlgorithm(alg);
        HATN_CHECK_EC(ec)
        auto passphraseKey=suite->createPassphraseKey(ec,alg);
        HATN_CHECK_EC(ec)
        passphraseKey->set(passphrase);
        passphraseKey->setSalt(salt);
        return invoke(keyContent,std::move(passphraseKey),key,suite,factory);
    }

    private:

        template <typename KeyContainerT>
        static Error invoke(
            const KeyContainerT& keyContent,
            common::SharedPtr<PassphraseKey> passphraseKey,
            SymmetricKey* key,
            const CipherSuite *suite,
            const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
            )
        {
            KeyProtector protector{std::move(passphraseKey),suite,factory};
            key->setProtector(&protector);
            auto ec=key->importFromBuf(keyContent,ContainerFormat::RAW_ENCRYPTED);
            HATN_CHECK_EC(ec)
            return OK;
        }
};
constexpr unpackWithPassphraseT unpackWithPassphrase{};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTPASSPHRASEPROTECTEDKEY_H
