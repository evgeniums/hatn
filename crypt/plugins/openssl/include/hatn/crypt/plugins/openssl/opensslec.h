/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslec.h
 *
 * 	Elliptic curves cryptography with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLEC_H
#define HATNOPENSSLEC_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/ec.h>
#include <openssl/evp.h>

#include <hatn/common/nativehandler.h>
#include <hatn/crypt/cryptalgorithm.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslsignature.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Private key for DSA key with OpenSSL backend
class HATN_OPENSSL_EXPORT ECPrivateKey : public OpenSslPrivateKey
{
    public:

        using OpenSslPrivateKey::OpenSslPrivateKey;

        virtual int nativeType() const noexcept override
        {
            return EVP_PKEY_EC;
        }

        void setCurveNid(int nid) noexcept
        {
            m_curveNid.val=nid;
        }

    protected:

        virtual common::Error doGenerate() override;

        common::ValueOrDefault<int,0> m_curveNid;
};

//! Elliptic curves cryptographic algorithm
class ECAlg : public OpenSslSignatureAlg
{
    public:

        /**
         * @brief Ctor
         * @param engine Backend engine to use
         * @param name Algorithm name, can be one of the following forms:
         *             "EC/<curve-name>", e.g. "EC/secp521r1"
         * @param parts Parsed parts from the name as described above
         */
        ECAlg(const CryptEngine* engine, CryptAlgorithm::Type type, const char* name,
               const std::vector<std::string>& parts) noexcept;

        /**
         * @brief Ctor
         * @param engine Backend engine to use
         * @param curveName Algorithm name, which is a direct curve name in this case, e.g. "secp521r1"
         */
        ECAlg(const CryptEngine* engine, CryptAlgorithm::Type type, const char* curveName);

        static std::vector<std::string> listCurves();

        virtual common::SharedPtr<PrivateKey> createPrivateKey() const override;

        int curveNID() const noexcept
        {
            return m_curveNid;
        }

    private:

        int m_curveNid;

        void init(const char* curveName) noexcept;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLEC_H
