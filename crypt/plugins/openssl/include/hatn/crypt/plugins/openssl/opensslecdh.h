/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslecdh.h
 *
 * 	Elliptic curves Diffie-Hellmann processing
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLECDH_H
#define HATNOPENSSLECDH_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/dh.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/securekey.h>
#include <hatn/crypt/publickey.h>

#include <hatn/crypt/ecdh.h>
#include <hatn/crypt/plugins/openssl/opensslec.h>

namespace hatn {
using namespace common;
namespace crypt {
namespace openssl {

//! Elliptic curves Diffie-Hellmann processing
class HATN_OPENSSL_EXPORT OpenSslECDH : public ECDH
{
    public:

        using ECDH::ECDH;

        /**
         * @brief Import DH state
         * @param pubKey Public key
         * @param privKey Private key
         * @return Operation status
         */
        virtual Error importState(
            common::SharedPtr<PrivateKey> privKey,
            common::SharedPtr<PublicKey> pubKey=common::SharedPtr<PublicKey>()
        ) noexcept override
        {
            if (!privKey.isNull())
            {
                if (privKey->alg()!=alg())
                {
                    return cryptError(CryptError::INVALID_KEY_TYPE);
                }
            }
            m_privKey=std::move(privKey);
            if (!m_privKey.isNull() && !m_privKey->isNativeValid())
            {
                return m_privKey->unpackContent();
            }

            std::ignore=pubKey;
            return Error();
        }

        /**
         * @brief Export DH state for long-term use
         * @param pubKey Public key
         * @param privKey Private key
         * @return Operation status
         */
        virtual common::Error exportState(
            common::SharedPtr<PrivateKey>& privKey,
            common::SharedPtr<PublicKey>& pubKey
        ) override
        {
            HATN_CHECK_RETURN(generateKey(pubKey))
            privKey=m_privKey;
            return Error();
        }

        //! Generate key on this side for further processing
        virtual Error generateKey(
            common::SharedPtr<PublicKey>& pubKey //!< Public key to be sent to peer
        ) override;

        /**
         * @brief Derive secret data with DH algorithm
         * @param peerPubKey Public key of peer side
         * @param result Computed result
         * @return Operation status
         */
        virtual Error computeSecret(
            const common::SharedPtr<PublicKey>& peerPubKey,
            common::SharedPtr<DHSecret>& result
        ) override;

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept
        {
            alg=std::make_shared<ECAlg>(engine,CryptAlgorithm::Type::ECDH,name);
            return Error();
        }

    private:

        common::SharedPtr<OpenSslPrivateKey> m_privKey;
};

} // namespace openssl
HATN_CRYPT_NAMESPACE_END

#endif // HATNOPENSSLECDH_H
