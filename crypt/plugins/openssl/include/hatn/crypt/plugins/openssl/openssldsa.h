/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssldsa.h
 *
 * 	DSA algorithm with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLDSA_H
#define HATNOPENSSLDSA_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/dsa.h>
#include <openssl/evp.h>

#include <hatn/common/nativehandler.h>
#include <hatn/crypt/cryptalgorithm.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslsignature.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! DSA algorithm with OpenSSL backend
class HATN_OPENSSL_EXPORT DSAAlg : public OpenSslSignatureAlg
{
    public:

        /**
         * @brief Ctor
         * @param engine Backend engine to use
         * @param name Algorithm name, can be one of the following:
         * <pre>
         *             "DSA/1024/160","DSA/2048/224","DSA/2048/256","DSA/3072/224"
         *             "DSA" - default the same as "DSA/2048/256"
         * </pre>
         * @param parts Parsed parts from the name as described above
         */
        DSAAlg(const CryptEngine* engine, const char* name,
               const std::vector<std::string>& parts) noexcept;

        //! Get key size in bytes
        virtual size_t keySize() const override
        {
            return static_cast<size_t>(m_N/8);
        }

        virtual common::SharedPtr<PrivateKey> createPrivateKey() const override;

    private:

        int m_N;
        int m_Q;

        friend class DSAPrivateKey;
};

//! Private key for DSA key with OpenSSL backend
class HATN_OPENSSL_EXPORT DSAPrivateKey : public OpenSslPrivateKey
{
    public:

        using OpenSslPrivateKey::OpenSslPrivateKey;

        virtual int nativeType() const noexcept override
        {
            return EVP_PKEY_DSA;
        }

        void setNativeAlg(const DSAAlg* alg) noexcept
        {
            m_nativeAlg.ptr=alg;
            setAlg(alg);
        }

    protected:

        virtual Error doGenerate() override;

    private:

        common::ConstPointerWithInit<DSAAlg*> m_nativeAlg;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLDSA_H
