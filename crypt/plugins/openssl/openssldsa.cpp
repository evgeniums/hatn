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

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>

#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.ipp>
#include <hatn/crypt/plugins/openssl/openssldsa.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* DSAPrivateKey ********************/

//---------------------------------------------------------------
Error DSAPrivateKey::doGenerate()
{
    auto nK=this;

    // generate parameters
    common::NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> pctx(::EVP_PKEY_CTX_new_id(EVP_PKEY_DSA,m_nativeAlg->engine()->nativeHandler<ENGINE>()));
    if (pctx.isNull())
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_paramgen_init(pctx.handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_CTX_set_dsa_paramgen_bits(pctx.handler,m_nativeAlg->m_N)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_CTX_ctrl(pctx.handler, EVP_PKEY_DSA, EVP_PKEY_OP_PARAMGEN, EVP_PKEY_CTRL_DSA_PARAMGEN_Q_BITS, m_nativeAlg->m_Q, NULL)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    common::NativeHandler<EVP_PKEY,detail::PkeyTraits> params;
    if (::EVP_PKEY_paramgen(pctx.handler,&params.handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }

    // generate key
    common::NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> ctx(::EVP_PKEY_CTX_new(params.handler,m_nativeAlg->engine()->nativeHandler<ENGINE>()));
    if (ctx.isNull())
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_keygen_init(ctx.handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_keygen(ctx.handler,&nK->nativeHandler().handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    return Error();
}

/******************* DSAAlg ********************/

//! DSA method stub
class DSAMethod
{
    public:
        static const DSAMethod* stub()
        {
            static DSAMethod m;
            return &m;
        }
};

//---------------------------------------------------------------
DSAAlg::DSAAlg(const CryptEngine *engine, const char *name, const std::vector<std::string> &parts) noexcept
    : OpenSslSignatureAlg(engine,CryptAlgorithm::Type::SIGNATURE,name),
      m_N(2048),
      m_Q(256)
{
    if (parts.size()>1)
    {
        try
        {
            int m_N=std::stoi(parts[1]);
            if (m_N!=1024 && m_N!=2048 && m_N!=3072)
            {
                return;
            }

            if (parts.size()>2)
            {
                m_Q=std::stoi(parts[2]);
                if (m_Q!=256 && m_Q!=224 && m_Q!=160)
                {
                    return;
                }
            }
        }
        catch (...)
        {
            return;
        }
    }

    setHandler(DSAMethod::stub());
}

//---------------------------------------------------------------
common::SharedPtr<PrivateKey> DSAAlg::createPrivateKey() const
{
    auto key=common::makeShared<DSAPrivateKey>();
    key->setNativeAlg(this);
    return key;
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
