/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/ecdhkeys.h
 *
 *  Helper for creating, exporting and implorting with ECDH keys
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTECDHKEYS_H
#define HATNCRYPTECDHKEYS_H

#include <hatn/common/sharedptr.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/ecdh.h>
#include <hatn/crypt/passphrasekey.h>
#include <hatn/crypt/keyprotector.h>

HATN_CRYPT_NAMESPACE_BEGIN

class CipherSuite;

/**
 * @brief Helper for creating, exporting and implorting with ECDH keys
 */
class HATN_CRYPT_EXPORT ECDHKeys
{
    public:

        Error init(const CipherSuite* suite, common::SharedPtr<KeyProtector> keyProtector, const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault());

        Error init(const CipherSuite* suite, common::SharedPtr<PassphraseKey> passphrase, const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault());

        Error init(const CipherSuite* suite, lib::string_view passphrase, const common::pmr::AllocatorFactory *factory=common::pmr::AllocatorFactory::getDefault());

        /**
         * @brief Generate key pair if ECDH state is not initialized yet
         * @return Operation status
         */
        Error generateKeys();

        Error importPrivateKey(lib::string_view keyData, ContainerFormat keyFormat=ContainerFormat::PEM);

        Result<common::SharedPtr<PrivateKey>> getPrivateKey();
        Result<common::SharedPtr<PublicKey>> getPublicKey();

        Result<common::ByteArrayShared> exportPublicKey(ContainerFormat keyFormat=ContainerFormat::PEM);
        Result<common::ByteArrayShared> exportPrivateKey(ContainerFormat keyFormat=ContainerFormat::PEM);

        Result<common::SharedPtr<DHSecret>> computeSecret(const common::SharedPtr<PublicKey>& peerPubKey);

    private:

        const CipherSuite* m_suite=nullptr;
        common::SharedPtr<ECDH> m_processor;
        common::SharedPtr<KeyProtector> m_keyProtector;
        common::SharedPtr<PublicKey> m_pubKey;
        common::SharedPtr<PrivateKey> m_privKey;

        bool m_ecdhReady=false;
        const common::pmr::AllocatorFactory *m_factory=nullptr;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTECDHKEYS_H
