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

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTPASSPHRASEPROTECTEDKEY_H
