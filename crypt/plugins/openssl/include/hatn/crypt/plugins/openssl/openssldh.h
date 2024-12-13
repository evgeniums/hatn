/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssldh.h
 *
 * 	Diffie-Hellmann routines and data
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

using DHPrivateKey = OpenSslSecretKey<PrivateKey>;

//! DH algorithm (just a stub)
class HATN_OPENSSL_EXPORT DHAlg : public CryptAlgorithm
{
    public:

        /**
         * @brief Ctor
         * @param engine Backend engine to use
         * @param name Algorithm name
         */
        DHAlg(const CryptEngine* engine, const char* name, std::string paramName, std::string paramSha1) noexcept;

        virtual common::SharedPtr<PrivateKey> createPrivateKey() const override;

        virtual const char* paramStr(size_t index=0) const override;

    private:

        std::string m_paramName;
        std::string m_paramSha1;
};

namespace detail
{
struct DHTraits
{
    static void free(::DH* dh)
    {
        ::DH_free(dh);
    }
};
}

//! Diffie-Hellmann routines and data
class HATN_OPENSSL_EXPORT OpenSslDH :
            public common::NativeHandlerContainer<::DH,detail::DHTraits,DH,OpenSslDH>
{
    public:

        using common::NativeHandlerContainer<::DH,detail::DHTraits,DH,OpenSslDH>::NativeHandlerContainer;

        /**
         * @brief Parse DH parameters to native object
         * @param dh Parsed native object
         * @param data Pointer to buffer
         * @param size Size of data
         * @return Operation status
         */

        static Error parseParameters(
            Native& dh,
            const char* data,
            size_t size
        ) noexcept;

        /**
         * @brief Parse DH parameters from content
         * @return Operation status
         */
        inline Error parseParameters() noexcept
        {
            return parseParameters(nativeHandler(),content().constData(),content().size());
        }

        /**
         * @brief Import DH state
         * @param pubKey Public key
         * @param privKey Private key
         * @return Operation status
         */
        virtual Error importState(
            common::SharedPtr<PrivateKey> privKey,
            common::SharedPtr<PublicKey> pubKey=common::SharedPtr<PublicKey>()
        ) noexcept override;

        /**
         * @brief Export DH state for long-term use
         * @param pubKey Public key
         * @param privKey Private key
         * @return Operation status
         */
        virtual common::Error exportState(
            common::SharedPtr<PrivateKey>& privKey,
            common::SharedPtr<PublicKey>& pubKey
        ) override;

        //! Generate key on this side for further processing
        virtual Error generateKey(
            common::SharedPtr<PublicKey>& pubKey //!< Public key to be sent to peer
        ) override;

        /**
         * @brief Derive secret data
         * @param peerPubKey Buffer with peer public key
         * @param peerPubKeySize Size of peer public key
         * @param unprotected Keep DH secret unprotected.
         *      If protector is set in the key the the key's content will be protected.
         * @param result Computed result
         * @return Operation status
         */
        Error computeSecret(
            const char* peerPubKey,
            size_t peerPubKeySize,
            common::SharedPtr<DHSecret>& result
        );

        /**
         * @brief Derive secret data with DH algorithm
         * @param peerPubKey Public key of peer side
         * @param result Computed result
         * @return Operation status
         */
        virtual Error computeSecret(
            const common::SharedPtr<PublicKey>& peerPubKey,
            common::SharedPtr<DHSecret>& result
        ) override
        {
            Assert(!peerPubKey.isNull(),"Invalid peer public key");
            return computeSecret(peerPubKey->content().data(),peerPubKey->content().size(),result);
        }

        /**
         * @brief Import parameters from buffer
         * @param buf Buffer to import from
         * @param size Size of the buffer
         * @param format Data format
         * @return Operation status
         */
        virtual common::Error importParamsFromBuf(
            const char* buf,
            size_t size,
            ContainerFormat format=ContainerFormat::PEM,
            bool keepContent=true
        ) override;

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        );
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLDH_H
