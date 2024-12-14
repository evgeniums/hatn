/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssldh.h
 *
 * 	DH implementation with Openssl 3 backend.
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLDH_H
#define HATNOPENSSLDH_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/dh.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/securekey.h>
#include <hatn/crypt/publickey.h>

#include <hatn/crypt/dh.h>

#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

#ifndef HATN_CRYPT_LEGACY_DH

//! Private key for DH with OpenSSL backend
class HATN_OPENSSL_EXPORT DHPrivateKey : public OpenSslPrivateKey
{
public:

    using OpenSslPrivateKey::OpenSslPrivateKey;

    virtual int nativeType() const noexcept override
    {
        return EVP_PKEY_DH;
    }

protected:

    virtual common::Error doGenerate() override;
};

//! DH algorithm (just a stub)
class HATN_OPENSSL_EXPORT DHAlg : public CryptAlgorithm
{
    public:

        /**
         * @brief Ctor
         * @param engine Backend engine to use
         * @param name Algorithm name
         */
        DHAlg(const CryptEngine* engine, const char* name) noexcept;

        virtual common::SharedPtr<PrivateKey> createPrivateKey() const override;
};

//! Base class implementing DH key generation and derivation.
template <typename BaseT>
class OpenSslDHT : public BaseT
{
    public:

        using BaseT::BaseT;

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
                if (privKey->alg()!=this->alg())
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

    private:

        common::SharedPtr<OpenSslPrivateKey> m_privKey;
};

#ifdef _WIN32
template class HATN_OPENSSL_EXPORT OpenSslDHT<DH>;
#endif

//! DH implementation with Openssl 3 backend.
class HATN_OPENSSL_EXPORT OpenSslDH : public OpenSslDHT<DH>
{
    public:

        using OpenSslDHT<DH>::OpenSslDHT;

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        );

        static std::vector<std::string> listDHs();
};

#endif

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLDH_H
