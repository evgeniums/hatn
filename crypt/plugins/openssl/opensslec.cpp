/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslec.cpp
 *
 * 	Elliptic curves cryptography with OpenSSL backend
 *
 */
/****************************************************************************/

#include <hatn/common/makeshared.h>

#include <hatn/crypt/encrypthmac.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>

#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.ipp>
#include <hatn/crypt/plugins/openssl/opensslec.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* ECPrivateKey ********************/

//---------------------------------------------------------------
Error ECPrivateKey::doGenerate()
{
    // generate parameters
    common::NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> pctx(::EVP_PKEY_CTX_new_id(EVP_PKEY_EC,alg()->engine()->nativeHandler<ENGINE>()));
    if (pctx.isNull())
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_paramgen_init(pctx.handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx.handler,m_curveNid.val)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    common::NativeHandler<EVP_PKEY,detail::PkeyTraits> params;
    if (::EVP_PKEY_paramgen(pctx.handler,&params.handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }

    // generate key
    common::NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> ctx(::EVP_PKEY_CTX_new(params.handler,alg()->engine()->nativeHandler<ENGINE>()));
    if (ctx.isNull())
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_keygen_init(ctx.handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }
    if (::EVP_PKEY_keygen(ctx.handler,&nativeHandler().handler)!=1)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }

    return Error();
}

/******************* ECAlg ********************/

//! Elliptic Curves method stub
class ECMethod
{
    public:
        static const ECMethod* stub()
        {
            static ECMethod m;
            return &m;
        }
};

//---------------------------------------------------------------
ECAlg::ECAlg(const CryptEngine* engine, CryptAlgorithm::Type type, const char* name,
             const std::vector<std::string>& parts) noexcept
    : OpenSslSignatureAlg(engine,type,name),
      m_curveNid(0)
{
    if (parts.size()==1)
    {
        const auto& curveName=parts[0];
        init(curveName.c_str());
    }
    else
    {
        const auto& curveName=parts[1];
        init(curveName.c_str());
    }
}

//---------------------------------------------------------------
ECAlg::ECAlg(const CryptEngine* engine, CryptAlgorithm::Type type, const char* curveName)
    : OpenSslSignatureAlg(engine,type,curveName),
      m_curveNid(0)
{
    init(curveName);
}

//---------------------------------------------------------------
common::SharedPtr<PrivateKey> ECAlg::createPrivateKey() const
{
    auto key=common::makeShared<ECPrivateKey>();
    key->setCurveNid(m_curveNid);
    key->setAlg(this);
    return key;
}

//---------------------------------------------------------------
std::vector<std::string> ECAlg::listCurves()
{
    std::vector<std::string> curves;
    size_t count=::EC_get_builtin_curves(NULL,0);
    if (count>0)
    {
        curves.reserve(count);
        std::vector<EC_builtin_curve> nativeCurves(count);
        size_t count1=EC_get_builtin_curves(nativeCurves.data(),count);
        Assert(count==count1,"Invalid number of built-in elliptic currves");
        for (size_t i=0;i<count;i++)
        {
            curves.emplace_back(::OBJ_nid2sn(nativeCurves[i].nid));
        }
    }
    return curves;
}

//---------------------------------------------------------------
void ECAlg::init(const char *curveName) noexcept
{
    m_curveNid=::OBJ_txt2nid(curveName);
    if (m_curveNid<=0)
    {
        m_curveNid=::EC_curve_nist2nid(curveName);
        if (m_curveNid<=0)
        {
            return;
        }
    }
    setHandler(ECMethod::stub());
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
