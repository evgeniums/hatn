/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslrsa.h
 *
 * 	RSA algorithm with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLRSA_H
#define HATNOPENSSLRSA_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/rsa.h>
#include <openssl/evp.h>

#include <hatn/common/nativehandler.h>
#include <hatn/crypt/cryptalgorithm.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslsignature.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Private key for RSA key with OpenSSL backend
class HATN_OPENSSL_EXPORT RSAPrivateKey : public OpenSslPrivateKey
{
    public:

        using OpenSslPrivateKey::OpenSslPrivateKey;

        virtual int nativeType() const noexcept override
        {
            return EVP_PKEY_RSA;
        }

    protected:

        virtual Error doGenerate() override;
};

//! RSA algorithm with OpenSSL backend
class HATN_OPENSSL_EXPORT RSAAlg : public OpenSslSignatureAlg
{
    public:

        /**
         * @brief Ctor
         * @param engine Backend engine to use
         * @param name Algorithm name, can be one of the following forms:
         * <pre>
         *             "RSA", then default 2048 bits length will be used
         *             "RSA/<bits-length>", e.g. "RSA/2048"
         * </pre>
         * @param parts Parsed parts from the name as described above
         */
        RSAAlg(const CryptEngine* engine, CryptAlgorithm::Type type, const char* name,
               const std::vector<std::string>& parts) noexcept;

        //! Get key size in bytes
        virtual size_t keySize() const
        {
            return m_keySize;
        }

        virtual common::SharedPtr<PrivateKey> createPrivateKey() const;

    private:

        size_t m_keySize;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLRSA_H
