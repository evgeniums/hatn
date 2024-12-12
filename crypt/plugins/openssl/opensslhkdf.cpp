/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslhkdf.cpp
 *
 * 	Hash Key Derivation Functions (HKDF) using OpenSSL backend
 *
 */
/****************************************************************************/

#include <hatn/crypt/plugins/openssl/opensslhkdf.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslHKDF ********************/

//---------------------------------------------------------------
void OpenSslHKDF::doReset() noexcept
{
    // EVP_PKEY_CTX can not be reused, just destroy it
    resetNative();
}

//---------------------------------------------------------------
common::Error OpenSslHKDF::doInit(const SecureKey *masterKey, const char *saltData, size_t saltSize)
{
    if (nativeHandler().isNull())
    {
        nativeHandler().handler = ::EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, hashAlg()->engine()->nativeHandler<ENGINE>());
        if (nativeHandler().isNull())
        {
            return makeLastSslError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
        }
    }

    if (::EVP_PKEY_derive_init(nativeHandler().handler)!=1)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }
    if (::EVP_PKEY_CTX_set_hkdf_md(nativeHandler().handler,hashAlg()->nativeHandler<EVP_MD>())!=1)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }

    int nativeMode=EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND;
    switch (mode())
    {
        case(HKDF::Mode::First_Extract_Then_Expand):
            nativeMode=EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY;
        break;

        case(HKDF::Mode::Extract_Expand_All):
            nativeMode=EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND;
        break;

        case(HKDF::Mode::Extract_All):
            nativeMode=EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY;
        break;

        case(HKDF::Mode::Expand_All):
        {
            auto keySize=hashAlg()->hashSize();
            if (keySize!=masterKey->content().size())
            {
                return cryptError(CryptError::INVALID_KEY_LENGTH);
            }
            nativeMode=EVP_PKEY_HKDEF_MODE_EXPAND_ONLY;
        }
        break;
    }

    if (::EVP_PKEY_CTX_hkdf_mode(nativeHandler().handler, nativeMode)!=1)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }

    if (::EVP_PKEY_CTX_set1_hkdf_key(nativeHandler().handler,
                                    reinterpret_cast<const unsigned char *>(masterKey->content().data()),
                                    static_cast<int>(masterKey->content().size()))!=1)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }

    if (::EVP_PKEY_CTX_set1_hkdf_salt(nativeHandler().handler,
                                    reinterpret_cast<const unsigned char *>(saltData),
                                    static_cast<int>(saltSize))!=1)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }

    if (mode()==HKDF::Mode::First_Extract_Then_Expand)
    {
        size_t length=0;
        if (::EVP_PKEY_derive(nativeHandler().handler,NULL,&length) <= 0)
        {
            return makeLastSslError(CryptError::KDF_FAILED);
        }
        common::MemoryLockedArray buf;
        buf.resize(length);
        if (::EVP_PKEY_derive(nativeHandler().handler,reinterpret_cast<unsigned char*>(buf.data()),&length) <= 0)
        {
            return makeLastSslError(CryptError::KDF_FAILED);
        }

        if (::EVP_PKEY_CTX_set1_hkdf_key(nativeHandler().handler,
                                        reinterpret_cast<const unsigned char *>(buf.data()),
                                        static_cast<int>(length))!=1)
        {
            return makeLastSslError(CryptError::KDF_FAILED);
        }

        if (::EVP_PKEY_CTX_hkdf_mode(nativeHandler().handler, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY)!=1)
        {
            return makeLastSslError(CryptError::KDF_FAILED);
        }
    }

    return common::Error();
}

//---------------------------------------------------------------
common::Error OpenSslHKDF::doDerive(common::SharedPtr<SymmetricKey> &derivedKey, const char *infoData, size_t infoSize)
{
    size_t keySize=0;
    HATN_CHECK_RETURN(resetKey(derivedKey,keySize))

    if (::EVP_PKEY_CTX_add1_hkdf_info(nativeHandler().handler,
                                    reinterpret_cast<const unsigned char *>(infoData),
                                    static_cast<int>(infoSize))!=1)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }

    common::MemoryLockedArray buf;
    buf.resize((keySize));
    if (::EVP_PKEY_derive(nativeHandler().handler,reinterpret_cast<unsigned char*>(buf.data()),&keySize) <= 0)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }
    return derivedKey->importFromKDF(buf.data(),buf.size());
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
