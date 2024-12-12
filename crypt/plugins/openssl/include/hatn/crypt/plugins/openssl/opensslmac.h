/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslhmac.h
 *
 * 	HMAC (Hash Message Authentication Code) implementation with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLMAC_H
#define HATNOPENSSLMAC_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>

#include <hatn/common/nativehandler.h>
#include <hatn/common/utils.h>
#include <hatn/common/makeshared.h>

#include <hatn/crypt/mac.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslmackey.h>
#include <hatn/crypt/plugins/openssl/openssldigestsign.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Any MAC method stub (except for HMAC where EVP_MD must be used)
class MacMethod
{
    public:

        static const MacMethod* stub()
        {
            static MacMethod m;
            return &m;
        }
};

//! MAC algorithm
class MACAlg : public CryptAlgorithm
{
    public:

        MACAlg(const CryptEngine* engine, const char* name, int nativeType) noexcept
            : CryptAlgorithm(engine,CryptAlgorithm::Type::MAC,name),
              m_nativeType(nativeType)
        {
        }

        static MACAlg& masterReference() noexcept;

        int nativeType() const noexcept
        {
            return m_nativeType;
        }

    private:

        int m_nativeType;
};

//! MAC algorithm based on HMAC
class MACAlg_HMAC : public MACAlg
{
    public:

        MACAlg_HMAC(const CryptEngine* engine, const char* name, const char* digestName) noexcept
            : MACAlg(engine,name,EVP_PKEY_HMAC)
        {
            setHandler(EVP_get_digestbyname(digestName));
        }

        virtual size_t keySize() const override
        {
            // can be of any size, let it be the size of hash result
            return EVP_MD_size(nativeHandler<EVP_MD>());
        }

        virtual size_t hashSize(bool) const override
        {
            return static_cast<size_t>(::EVP_MD_size(nativeHandler<EVP_MD>()));
        }

        virtual size_t blockSize() const override
        {
            return static_cast<size_t>(::EVP_MD_block_size(nativeHandler<EVP_MD>()));
        }

        virtual common::SharedPtr<MACKey> createMACKey() const override
        {
            auto key=common::makeShared<MACKey_HMAC>();
            key->setAlg(this);
            return key;
        }
};

//! MAC algorithm based on Poly1305
class MACAlg_Poly1305 : public MACAlg
{
    public:

        MACAlg_Poly1305(const CryptEngine* engine, const char* name) noexcept
            : MACAlg(engine,name,EVP_PKEY_POLY1305)
        {
            setHandler(MacMethod::stub());
        }

        virtual size_t keySize() const override
        {
            return 32;
        }

        virtual size_t hashSize(bool) const override
        {
            return 16;
        }

        virtual size_t blockSize() const override
        {
            return 16;
        }

        virtual common::SharedPtr<MACKey> createMACKey() const override
        {
            auto key=common::makeShared<MACKey_Poly1305>();
            key->setAlg(this);
            return key;
        }
};

//! MAC algorithm based on SIPHASH
class MACAlg_SIPHASH : public MACAlg
{
    public:

        MACAlg_SIPHASH(const CryptEngine* engine, const char* name, size_t hashSize=16) noexcept
            : MACAlg(engine,name,EVP_PKEY_SIPHASH),
              m_hashSize(hashSize)
        {
            setHandler(MacMethod::stub());
        }

        virtual size_t keySize() const override
        {
            return 16;
        }

        virtual size_t hashSize(bool) const override
        {
            return m_hashSize;
        }

        virtual size_t blockSize() const override
        {
            return 16;
        }

        virtual common::SharedPtr<MACKey> createMACKey() const override
        {
            auto key=common::makeShared<MACKey_SIPHASH>();
            key->setAlg(this);
            return key;
        }

    private:

        size_t m_hashSize;
};

//! MAC algorithm based on CMAC
class MACAlg_CMAC : public MACAlg
{
    public:

        MACAlg_CMAC(const CryptEngine* engine, const char* name) noexcept
            : MACAlg(engine,name,EVP_PKEY_CMAC)
        {
            setHandler(MacMethod::stub());
        }

        virtual size_t keySize() const override
        {
            return 16;
        }

        virtual size_t hashSize(bool) const override
        {
            return 16;
        }

        virtual size_t blockSize() const override
        {
            return 16;
        }

        virtual common::SharedPtr<MACKey> createMACKey() const override
        {
            auto key=common::makeShared<MACKey_CMAC>();
            key->setAlg(this);
            return key;
        }
};

#ifdef _WIN32
template class HATN_OPENSSL_EXPORT OpenSslDigestSign<MAC,CryptAlgorithm::Type::MAC>;
#endif

//! MAC processor by OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslMAC : public OpenSslDigestSign<MAC,CryptAlgorithm::Type::MAC>
{
    public:

        using OpenSslDigestSign<MAC,CryptAlgorithm::Type::MAC>::OpenSslDigestSign;

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept;

        static std::vector<std::string> listMACs();

        virtual const CryptAlgorithm* digestAlg() const noexcept override;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLMAC_H
