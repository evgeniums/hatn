/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssldsa.cpp
 *
 * 	DSA algorithm with OpenSSL backend
 *
 */
/****************************************************************************/

#include <boost/algorithm/string.hpp>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>

#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.ipp>
#include <hatn/crypt/plugins/openssl/opensslrsa.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! RSA method stub
class RSAMethod
{
public:
    static const RSAMethod* stub()
    {
        static RSAMethod m;
        return &m;
    }
};

/******************* RSAPrivateKey ********************/

//---------------------------------------------------------------
Error RSAPrivateKey::doGenerate()
{
    auto nK=this;

    common::NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> ctx(::EVP_PKEY_CTX_new_id(EVP_PKEY_RSA,alg()->engine()->nativeHandler<ENGINE>()));
    if (ctx.isNull())
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_keygen_init(ctx.handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.handler,static_cast<int>(alg()->keySize()*8))!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_keygen(ctx.handler,&nK->nativeHandler().handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }

    return Error();
}

//---------------------------------------------------------------
RSAAlg::RSAAlg(const CryptEngine* engine, CryptAlgorithm::Type type, const char* name,
               const std::vector<std::string>& parts) noexcept
    : OpenSslSignatureAlg(engine,type,name),
      m_keySize(2048/8)
{
    try
    {
        if (parts.size()>1)
        {
            int length=std::stoi(parts[1]);
            if (length>0)
            {
                m_keySize=length/8;
            }
        }
        setHandler(RSAMethod::stub());
    }
    catch (...)
    {
    }
}

//---------------------------------------------------------------
common::SharedPtr<PrivateKey> RSAAlg::createPrivateKey() const
{
    auto key=common::makeShared<RSAPrivateKey>();
    key->setAlg(this);
    return key;
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
