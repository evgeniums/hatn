/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/checksharedsecret.h
 *
 *      Check shared secret using MAC
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTCHECKSHAREDSECRET_H
#define HATNCRYPTCHECKSHAREDSECRET_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/passphrasekey.h>

HATN_CRYPT_NAMESPACE_BEGIN

struct checkSharedSecretT
{
    Error operator() (
        common::SharedPtr<PassphraseKey> passphraseKey1,
        common::SharedPtr<PassphraseKey> passphraseKey2,
        const CipherSuite *suite,
        const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
    ) const
    {
        return invoke(std::move(passphraseKey1),std::move(passphraseKey2),suite,factory);
    }

    template <typename CheckContainerT>
    Error operator() (
        common::SharedPtr<PassphraseKey> passphraseKey1,
        const CheckContainerT& secret2,
        const CipherSuite *suite,
        const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
    ) const
    {
        return invoke(std::move(passphraseKey1),secret2,suite,factory);
    }

    template <typename SampleContainerT, typename CheckContainerT>
    Error operator() (
        const SampleContainerT& secret1,
        const CheckContainerT& secret2,
        const CipherSuite *suite,
        const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
    ) const
    {
        return invoke(secret1,secret2,suite,factory);
    }

    private:

        static Error invoke(
                common::SharedPtr<PassphraseKey> passphraseKey1,
                common::SharedPtr<PassphraseKey> passphraseKey2,
                const CipherSuite *suite,
                const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
            )
        {
            CryptAlgorithmConstP alg;
            auto ec=suite->macAlgorithm(alg);
            HATN_CHECK_EC(ec)

            common::ByteArray buf{factory};
            auto randGen=suite->suites()->randomGenerator();
            if (!randGen)
            {
                return cryptError(CryptError::RANDOM_GENERATOR_NOT_DEFINED);
            }
            ec=randGen->randContainer(buf,16,8);
            HATN_CHECK_EC(ec)

            passphraseKey1->resetDerivedKey();
            passphraseKey2->resetDerivedKey();

            auto mac1=suite->createMAC(ec);
            HATN_CHECK_EC(ec)
            passphraseKey1->setAlg(alg);
            ec=passphraseKey1->deriveKey();
            HATN_CHECK_EC(ec)
            mac1->setKey(passphraseKey1->derivedKey());
            common::ByteArray tag{factory};
            ec=mac1->runSign(buf,tag);
            HATN_CHECK_EC(ec)

            auto mac2=suite->createMAC(ec);
            HATN_CHECK_EC(ec)
            passphraseKey2->setAlg(alg);
            ec=passphraseKey2->deriveKey();
            HATN_CHECK_EC(ec)
            mac2->setKey(passphraseKey2->derivedKey());
            ec=mac2->runVerify(buf,tag);
            HATN_CHECK_EC(ec)

            passphraseKey1->resetDerivedKey();
            passphraseKey2->resetDerivedKey();

            return OK;
        }

        template <typename CheckContainerT>
        static Error invoke(
                common::SharedPtr<PassphraseKey> passphraseKey1,
                const CheckContainerT& secret2,
                const CipherSuite *suite,
                const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
            )
        {
            CryptAlgorithmConstP alg;
            auto ec=suite->macAlgorithm(alg);
            HATN_CHECK_EC(ec)
            auto passphraseKey=suite->createPassphraseKey(ec,alg);
            HATN_CHECK_EC(ec)
            passphraseKey->set(secret2);

            return invoke(std::move(passphraseKey1),passphraseKey,suite,factory);
        }

        template <typename SampleContainerT, typename CheckContainerT>
        static Error invoke(
            const SampleContainerT& secret1,
            const CheckContainerT& secret2,
            const CipherSuite *suite,
            const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault()
            )
        {
            CryptAlgorithmConstP alg;
            auto ec=suite->macAlgorithm(alg);
            HATN_CHECK_EC(ec)
            auto passphraseKey=suite->createPassphraseKey(ec,alg);
            HATN_CHECK_EC(ec)
            passphraseKey->set(secret1);

            return invoke(std::move(passphraseKey),secret2,suite,factory);
        }
};
constexpr checkSharedSecretT checkSharedSecret{};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTCHECKSHAREDSECRET_H
