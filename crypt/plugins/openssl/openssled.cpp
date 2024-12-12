/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssled.cpp
 *
 * 	Edwards DSA signature algorithms with OpenSSL backend
 *
 */
/****************************************************************************/

#include <boost/algorithm/string.hpp>

#include <hatn/common/meta/dynamiccastwithsample.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>

#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.ipp>
#include <hatn/crypt/plugins/openssl/openssled.h>

#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* EDPrivateKey ********************/

//---------------------------------------------------------------
template <int NativeType>
Error EDPrivateKey<NativeType>::doGenerate()
{
    common::NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> ctx(::EVP_PKEY_CTX_new_id(NativeType,alg()->engine()->template nativeHandler<ENGINE>()));
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

/******************* EDAlg ********************/

//! Edwards DSA method stub
class EDMethod
{
    public:

        static const EDMethod* stub()
        {
            static EDMethod m;
            return &m;
        }
};

//---------------------------------------------------------------
EDAlg::EDAlg(const CryptEngine* engine, CryptAlgorithm::Type type, const char* algName)
    : OpenSslSignatureAlg(engine,type,algName),
      m_nativeType(
            boost::iequals(std::string(algName),std::string("ED/448"))?EVP_PKEY_ED448:
            boost::iequals(std::string(algName),std::string("ED/25519"))?EVP_PKEY_ED25519:
            -1
          )
{
    if (m_nativeType<0)
    {
        return;
    }
    setHandler(EDMethod::stub());
}

//---------------------------------------------------------------
common::SharedPtr<SignatureSign> EDAlg::createSignatureSign() const
{
    auto p=common::makeShared<OpenSslEdSign>();
    p->setAlg(this);
    return p;
}

//---------------------------------------------------------------
common::SharedPtr<SignatureVerify> EDAlg::createSignatureVerify() const
{
    auto p=common::makeShared<OpenSslEdVerify>();
    p->setAlg(this);
    return p;
}

//---------------------------------------------------------------

template class HATN_OPENSSL_EXPORT EDPrivateKey<EVP_PKEY_ED448>;
template class HATN_OPENSSL_EXPORT EDPrivateKey<EVP_PKEY_ED25519>;

//---------------------------------------------------------------
common::SharedPtr<PrivateKey> EDAlg::createPrivateKey() const
{
    common::SharedPtr<PrivateKey> key;
    if (m_nativeType==EVP_PKEY_ED448)
    {
        key=common::makeShared<EDPrivateKey<EVP_PKEY_ED448>>();
    }
    else
    {
        key=common::makeShared<EDPrivateKey<EVP_PKEY_ED25519>>();
    }
    key->setAlg(this);
    return key;
}

/*********************** OpenSslEdSign *************************/

//---------------------------------------------------------------
Error OpenSslEdSign::doInit() noexcept
{
    if (digestAlg()!=nullptr)
    {
        HATN_CHECK_RETURN(m_digest.init(digestAlg()))
    }

    HATN_CHECK_RETURN(detail::DigestTraits::init(this))

    EVP_PKEY* pkeyNative=detail::DigestSignPkeyTraits<CryptAlgorithm::Type::SIGNATURE>::nativeKey(this->key());
    if (pkeyNative==nullptr)
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }
    ENGINE* engine=this->alg()->engine()->template nativeHandler<ENGINE>();
    if (::EVP_DigestSignInit(this->nativeHandler().handler,NULL,
                       NULL,
                       engine,
                       pkeyNative
                        ) != 1)
    {
        return makeLastSslError(CryptError::SIGN_FAILED);
    }

    return Error();
}

//---------------------------------------------------------------
void OpenSslEdSign::doReset() noexcept
{
    m_sig.clear();
    m_digest.reset();
    OpenSslSignatureSign::doReset();
}

//---------------------------------------------------------------
Error OpenSslEdSign::doFinalize(char *buf) noexcept
{
    if (digestAlg()!=nullptr)
    {
        common::ByteArray hash;
        HATN_CHECK_RETURN(m_digest.finalize(hash))
        HATN_CHECK_RETURN(calcSignature(hash.data(),hash.size()))
    }

    if (m_sig.isEmpty())
    {
        return cryptError(CryptError::INVALID_SIGNATURE_STATE);
    }
    std::copy(m_sig.data(),m_sig.data()+m_sig.size(),buf);
    return Error();
}

//---------------------------------------------------------------
size_t OpenSslEdSign::actualSignatureSize() const
{
    return m_sig.size();
}

//---------------------------------------------------------------
Error OpenSslEdSign::doProcess(const char *buf, size_t size) noexcept
{
    if (digestAlg()!=nullptr)
    {
        return m_digest.process(common::ConstDataBuf(buf,size));
    }
    return calcSignature(buf,size);
}

//---------------------------------------------------------------
Error OpenSslEdSign::calcSignature(const char *buf, size_t size) noexcept
{
    if (!m_sig.isEmpty())
    {
        return cryptError(CryptError::INVALID_SIGNATURE_STATE);
    }
    try
    {
        m_sig.resize(resultSize());
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }

    size_t sigLen=m_sig.size();
    if (::EVP_DigestSign(this->nativeHandler().handler,reinterpret_cast<unsigned char*>(m_sig.data()),&sigLen,
                         reinterpret_cast<const unsigned char*>(buf),size) != 1)
    {
        return makeLastSslError(CryptError::SIGN_FAILED);
    }
    m_sig.resize(sigLen);

    return Error();
}

/*********************** OpenSslEdVerify *************************/

//---------------------------------------------------------------
Error OpenSslEdVerify::doInit() noexcept
{
    if (digestAlg()!=nullptr)
    {
        HATN_CHECK_RETURN(m_digest.init(digestAlg()))
    }

    HATN_CHECK_RETURN(detail::DigestTraits::init(this))

    if (!this->key()->isBackendKey())
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }
    const OpenSslPublicKey* pkey=common::dynamicCastWithSample(this->key(),&OpenSslPublicKey::masterReference());
    ENGINE* engine=this->alg()->engine()->template nativeHandler<ENGINE>();
    if (::EVP_DigestVerifyInit(this->nativeHandler().handler,NULL,
                       NULL,
                       engine,
                       pkey->nativeHandler().handler
                      ) != 1)
    {
        return makeLastSslError(CryptError::VERIFY_FAILED);
    }

    return Error();
}

//---------------------------------------------------------------
void OpenSslEdVerify::doReset() noexcept
{
    m_digest.reset();
    m_buf.reset();
    OpenSslSignatureVerify::doReset();
}

//---------------------------------------------------------------
Error OpenSslEdVerify::doProcess(const char *buf, size_t size) noexcept
{
    if (digestAlg()!=nullptr)
    {
        return m_digest.process(common::ConstDataBuf(buf,size));
    }

    if (!m_buf.isEmpty())
    {
        return cryptError(CryptError::INVALID_SIGNATURE_STATE);
    }
    m_buf.set(buf,size);
    return Error();
}

//---------------------------------------------------------------
Error OpenSslEdVerify::doVerify(const char *signature, size_t signatureLength) noexcept
{
    const unsigned char* data=reinterpret_cast<const unsigned char*>(m_buf.data());
    auto size=m_buf.size();
    common::ByteArray hash;
    if (digestAlg()!=nullptr)
    {
        HATN_CHECK_RETURN(m_digest.finalize(hash))
        data=reinterpret_cast<const unsigned char*>(hash.data());
        size=hash.size();
    }
    else if (m_buf.isEmpty())
    {
        return cryptError(CryptError::INVALID_SIGNATURE_STATE);
    }

    if (::EVP_DigestVerify(this->nativeHandler().handler,reinterpret_cast<const unsigned char*>(signature),signatureLength,
                           data,size) != 1)
    {
        return makeLastSslError(CryptError::VERIFY_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
