/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslasymmetric.h
 *
 * 	Asymmetric cryptography with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLASYMMETRIC_H
#define HATNOPENSSLASYMMETRIC_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <boost/algorithm/string.hpp>

#include <hatn/crypt/cryptalgorithm.h>
#include <hatn/crypt/cryptplugin.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslec.h>
#include <hatn/crypt/plugins/openssl/opensslrsa.h>
#include <hatn/crypt/plugins/openssl/openssldsa.h>
#include <hatn/crypt/plugins/openssl/openssled.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Asymmetric cryptography with OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslAsymmetric
{
    public:

        static Error findNativeAlgorithm(
            CryptAlgorithm::Type type,
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept;

        static std::vector<std::string> listSignatures();

        /**
         * @brief Create private key from content
         * @param pkey Target key
         * @param buf Buffer pointer
         * @param size Buffer size
         * @param format Data format
         * @param protector Key protector
         * @param plugin Backend plugin
         * @param engineName Name of backend engine
         * @return Operation status
         *
         */
        static Error createPrivateKeyFromContent(
            common::SharedPtr<PrivateKey>& pkey,
            const char* buf,
            size_t size,
            ContainerFormat format,
            CryptPlugin* plugin,
            KeyProtector* protector=nullptr,
            const char* engineName=nullptr
        );

        /**
         * @brief Get algorithm of public key
         * @param alg Target algorithm
         * @param key Public key
         * @param plugin Backend plugin
         * @param engineName Name of backend engine
         * @return Operation status
         */
        static Error publicKeyAlgorithm(
            CryptAlgorithmConstP &alg,
            const PublicKey* key,
            CryptPlugin* plugin,
            const char* engineName=nullptr
        );
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLASYMMETRIC_H
