/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file crypt/plugins/openssl/opensslpbkdf.h
 *
 * 	PBKDF using OpenSSL backend
 *
 */

/****************************************************************************/

#ifndef HATNOPENSSLPBKDF_H
#define HATNOPENSSLPBKDF_H

#include <boost/algorithm/string.hpp>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>

#include <hatn/crypt/pbkdf.h>

#include <hatn/crypt/plugins/openssl/opensslplugin.h>
#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Base method for PBKDF classes of OpenSSL backend
class PBKDFMethod
{
    public:

        PBKDFMethod()=default;
        virtual ~PBKDFMethod()=default;
        PBKDFMethod(const PBKDFMethod&)=delete;
        PBKDFMethod(PBKDFMethod&&) =delete;
        PBKDFMethod& operator=(const PBKDFMethod&)=delete;
        PBKDFMethod& operator=(PBKDFMethod&&) =delete;

        virtual common::Error derive(
            const char* passwdData,
            size_t passwdLength,
            const char* saltData,
            size_t saltLength,
            char* keyData,
            size_t keyLength
        ) const =0;
};

//! PBKDF2 method of OpenSSL backend
class HATN_OPENSSL_EXPORT PBKDF2Method : public PBKDFMethod
{
    public:

        PBKDF2Method(
            const EVP_MD* digest=::EVP_sha1(),
            size_t iterationCount=1000
        );

        void setIterationCount(size_t count) noexcept
        {
            m_iterationCount=count;
        }
        size_t interationCount() const noexcept
        {
            return m_iterationCount;
        }

        void setDigest(const EVP_MD* digest) noexcept
        {
            m_digest=digest;
        }
        const EVP_MD* digest() const noexcept
        {
            return m_digest;
        }

        virtual common::Error derive(
            const char* passwdData,
            size_t passwdLength,
            const char* saltData,
            size_t saltLength,
            char* keyData,
            size_t keyLength
        ) const override;

        static std::shared_ptr<PBKDFMethod> createMethod(const std::vector<std::string>& parts);

    private:

        size_t m_iterationCount=1000;
        const EVP_MD* m_digest;
};

//! SCrypt method of OpenSSL backend
class HATN_OPENSSL_EXPORT SCryptMethod : public PBKDFMethod
{
    public:

        SCryptMethod(
            uint64_t N,
            uint64_t r,
            uint64_t p,
            uint64_t maxMemBytes
        );

        virtual common::Error derive(
            const char* passwdData,
            size_t passwdLength,
            const char* saltData,
            size_t saltLength,
            char* keyData,
            size_t keyLength
        ) const override;

        static std::shared_ptr<PBKDFMethod> createMethod(const std::vector<std::string>& parts);

    private:

        uint64_t N;
        uint64_t r;
        uint64_t p;
        uint64_t maxMemBytes;
};

//! PBKDF algorithm
class HATN_OPENSSL_EXPORT PBKDFAlg : public CryptAlgorithm
{
    public:

        PBKDFAlg(const CryptEngine* engine, const char* name) noexcept;

        common::Error derive(
            const char* passwdData,
            size_t passwdLength,
            const char* saltData,
            size_t saltLength,
            char* keyData,
            size_t keyLength
        ) const
        {
            return m_method->derive(passwdData,passwdLength,saltData,saltLength,keyData,keyLength);
        }

        virtual common::SharedPtr<SymmetricKey> createSymmetricKey() const override;

    private:

        std::shared_ptr<PBKDFMethod> m_method;
};

//! Password based key derivation function by OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslPBKDF : public PBKDF
{
    public:

        using PBKDF::PBKDF;

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        );

        /**
         * @brief Derive key from data buffer with password
         * @param passwdData Data buffer with password
         * @param passwdLength Length of password
         * @param key Derived key
         * @param alg Agorithm to use, if null then default algorithm must be used by backend
         * @return Operation status
         */
        virtual common::Error doDerive(
            const char* passwdData,
            size_t passwdLength,
            common::SharedPtr<SymmetricKey>& key,
            const char* saltData,
            size_t saltLength
        );

        static std::vector<std::string> listAlgs();
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLPBKDF_H
