/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslpbkdf.cpp
 *
 * 	PBKDF using OpenSSL backend
 *
 */
/****************************************************************************/

#include <openssl/kdf.h>

#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslpbkdf.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* PBKDF2Method ********************/

//---------------------------------------------------------------
PBKDF2Method::PBKDF2Method(
        const EVP_MD *digest,
        size_t iterationCount
    ) : m_iterationCount(iterationCount),
        m_digest(digest)
{}

//---------------------------------------------------------------
common::Error PBKDF2Method::derive(
        const char *passwdData,
        size_t passwdLength,
        const char *saltData,
        size_t saltLength,
        char *keyData,
        size_t keyLength
    ) const
{
    if (PKCS5_PBKDF2_HMAC(passwdData, static_cast<int>(passwdLength),
                       reinterpret_cast<const unsigned char*>(saltData), static_cast<int>(saltLength),
                       static_cast<int>(m_iterationCount),
                       m_digest,
                       static_cast<int>(keyLength),
                       reinterpret_cast<unsigned char*>(keyData)
        )!=1)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------
std::shared_ptr<PBKDFMethod> PBKDF2Method::createMethod(const std::vector<std::string> &parts)
{
    const EVP_MD* digest=::EVP_sha1();
    size_t iterationCount=1000;
    if (parts.size()>1)
    {
        digest=::EVP_get_digestbyname(parts[1].c_str());
        if (digest==nullptr)
        {
            return std::shared_ptr<PBKDFMethod>();
        }
    }
    if (parts.size()>2)
    {
        try
        {
            int iterCount=std::stoi(parts[2]);
            if (iterCount<=0)
            {
                return std::shared_ptr<PBKDFMethod>();
            }
            iterationCount=static_cast<size_t>(iterCount);
        }
        catch (...)
        {
            return std::shared_ptr<PBKDFMethod>();
        }
    }
    return std::make_shared<PBKDF2Method>(digest,iterationCount);
}

/******************* SCryptMethod ********************/

//---------------------------------------------------------------
SCryptMethod::SCryptMethod(uint64_t N, uint64_t r, uint64_t p, uint64_t maxMemBytes)
    : N(N), r(r), p(p), maxMemBytes(maxMemBytes)
{}

//---------------------------------------------------------------
common::Error SCryptMethod::derive(
        const char *passwdData,
        size_t passwdLength,
        const char *saltData,
        size_t saltLength,
        char *keyData,
        size_t keyLength
    ) const
{
    if (saltLength==0)
    {
        return cryptError(CryptError::SALT_REQUIRED);
    }

    common::NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> pctx(::EVP_PKEY_CTX_new_id(EVP_PKEY_SCRYPT, NULL));
    if (pctx.isNull())
    {
        return makeLastSslError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }

    if (::EVP_PKEY_derive_init(pctx.handler) <= 0)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }
    if (::EVP_PKEY_CTX_set1_pbe_pass(pctx.handler,passwdData,static_cast<int>(passwdLength)) <= 0)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }
    if (::EVP_PKEY_CTX_set1_scrypt_salt(pctx.handler,reinterpret_cast<const unsigned char*>(saltData),static_cast<int>(saltLength)) <= 0)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }
    if (::EVP_PKEY_CTX_set_scrypt_N(pctx.handler,N) <= 0)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }
    if (::EVP_PKEY_CTX_set_scrypt_r(pctx.handler,r) <= 0)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }
    if (::EVP_PKEY_CTX_set_scrypt_p(pctx.handler,p) <= 0)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }
    if (maxMemBytes!=0)
    {
        if (::EVP_PKEY_CTX_set_scrypt_maxmem_bytes(pctx.handler,maxMemBytes) <= 0)
        {
            return makeLastSslError(CryptError::KDF_FAILED);
        }
    }

    if (::EVP_PKEY_derive(pctx.handler,reinterpret_cast<unsigned char*>(keyData),&keyLength) <= 0)
    {
        return makeLastSslError(CryptError::KDF_FAILED);
    }

    return Error();
}

//---------------------------------------------------------------
std::shared_ptr<PBKDFMethod> SCryptMethod::createMethod(const std::vector<std::string> &parts)
{
    uint64_t N=16384;
    uint64_t r=8;
    uint64_t p=1;
    uint64_t maxMemBytes=0;

    try
    {
        if (parts.size()>1)
        {
            N=std::stoul(parts[1]);
        }
        if (parts.size()>2)
        {
            r=std::stoul(parts[2]);
        }
        if (parts.size()>3)
        {
            p=std::stoul(parts[3]);
        }
        if (parts.size()>4)
        {
            maxMemBytes=std::stoul(parts[4]);
        }
    }
    catch (...)
    {
        return std::shared_ptr<PBKDFMethod>();
    }
    return std::make_shared<SCryptMethod>(N,r,p,maxMemBytes);
}

/******************* PBKDFAlg ********************/

//! Find PBKDF method by name
inline std::shared_ptr<PBKDFMethod> pbkdfByName(const char* name)
{
    std::vector<std::string> parts;
    splitAlgName(name,parts);
    if (parts.size()<1)
    {
        return std::shared_ptr<PBKDFMethod>();
    }
    const auto& nameStr=parts[0];

    if (boost::iequals(nameStr,std::string("PBKDF2")))
    {
        return PBKDF2Method::createMethod(parts);
    }
    else if (boost::iequals(nameStr,std::string("SCrypt")))
    {
        return SCryptMethod::createMethod(parts);
    }

    return std::shared_ptr<PBKDFMethod>();
}

//---------------------------------------------------------------
PBKDFAlg::PBKDFAlg(const CryptEngine *engine, const char *name) noexcept
    : CryptAlgorithm(engine,CryptAlgorithm::Type::PBKDF,name),
      m_method(pbkdfByName(name))
{
    setHandler(m_method.get());
}

//---------------------------------------------------------------
common::SharedPtr<SymmetricKey> PBKDFAlg::createSymmetricKey() const
{
    auto key=common::makeShared<OpenSslPassphraseKey>();
    key->setKdfAlg(this);
    return key;
}

/******************* OpenSslPBKDF ********************/

//---------------------------------------------------------------
common::Error OpenSslPBKDF::findNativeAlgorithm(std::shared_ptr<CryptAlgorithm> &alg, const char *name, CryptEngine *engine)
{
    alg=std::make_shared<PBKDFAlg>(engine,name);
    return common::Error();
}

//---------------------------------------------------------------
common::Error OpenSslPBKDF::doDerive(
        const char *passwdData,
        size_t passwdLength,
        common::SharedPtr<SymmetricKey> &key,
        const char *saltData,
        size_t saltLength
    )
{
    const PBKDFAlg* alg=dynamic_cast<const PBKDFAlg*>(kdfAlg());
    if (alg==nullptr)
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }

    size_t keySize=0;
    bool keyCreated=key.isNull();
    HATN_CHECK_RETURN(resetKey(key,keySize))

    common::MemoryLockedArray buf;
    buf.resize(keySize);

    auto ec=alg->derive(passwdData,passwdLength,saltData,saltLength,buf.data(),keySize);
    if (!ec)
    {
        ec=key->importFromKDF(buf.data(),buf.size());
    }
    if (ec && keyCreated)
    {
        key.reset();
    }
    return ec;
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPBKDF::listAlgs()
{
    std::vector<std::string> algs;
    algs.push_back("pbkdf2[/digest=sha1/iteration_count=1000]");
    algs.push_back("scrypt[/N=16384/r=8/p=1/maxMemBytes=0]");
    return algs;
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
