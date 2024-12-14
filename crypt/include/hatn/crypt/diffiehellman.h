/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/diffiehellman.h
 *
 *      Diffie-Hellmann types.
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTDIFFIEHELLMAN_H
#define HATNCRYPTDIFFIEHELLMAN_H

#include <memory>

#include <hatn/common/singleton.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/fixedbytearray.h>
#include <hatn/common/memorylockeddata.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/keycontainer.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/publickey.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Base class for DH-generated keys
class DHSecret : public SecureKey
{
    public:

        using SecureKey::SecureKey;

        virtual uint32_t role() const noexcept override
        {
            return roleInt(Role::DH_SECRET);
        }

        virtual bool canKeepUnprotectedContent() const noexcept override
        {
            return true;
        }
};

/**
 * @brief Base class for DH and ECDH implementations
 */
class DiffieHellman
{
    public:

        DiffieHellman()=default;
        virtual ~DiffieHellman()=default;
        DiffieHellman(const DiffieHellman&)=delete;
        DiffieHellman(DiffieHellman&&) =delete;
        DiffieHellman& operator=(const DiffieHellman&)=delete;
        DiffieHellman& operator=(DiffieHellman&&) =delete;

        //! Generate key on this side for further processing
        virtual common::Error generateKey(
            common::SharedPtr<PublicKey>& pubKey //!< Public key to be sent to peer
        ) =0;

        /**
         * @brief Derive secret data
         * @param peerPubKey Public key of peer side
         * @param result Computed secret
         * @return Operation status
         */
        virtual common::Error computeSecret(
            const common::SharedPtr<PublicKey>& peerPubKey,
            common::SharedPtr<DHSecret>& result
        ) =0;

        /**
         * @brief Export DH state for long-term use
         * @param pubKey Public key
         * @param privKey Private key
         * @return Operation status
         *
         * The keys can be empty if it they were not set or generated yet.
         */
        virtual common::Error exportState(
            common::SharedPtr<PrivateKey>& privKey,
            common::SharedPtr<PublicKey>& pubKey
        ) =0;

        /**
         * @brief Import DH state
         * @param pubKey Public key
         * @param privKey Private key
         * @return Operation status
         *
         * Any of the keys can be empty.
         */
        virtual common::Error importState(
            common::SharedPtr<PrivateKey> privKey,
            common::SharedPtr<PublicKey> pubKey=common::SharedPtr<PublicKey>()
        ) noexcept =0;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTDIFFIEHELLMAN_H
