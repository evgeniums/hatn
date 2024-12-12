/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslhmac.cpp
 *
 * 	HMAC (Hash Message Authentication Code) implementation with OpenSSL backend
 *
 */
/****************************************************************************/

#include <hatn/common/meta/dynamiccastwithsample.h>

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/openssldigestsign.ipp>
#include <hatn/crypt/plugins/openssl/opensslsignature.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslSignatureAlg ********************/

//---------------------------------------------------------------
common::SharedPtr<SignatureSign> OpenSslSignatureAlg::createSignatureSign() const
{
    auto p=common::makeShared<OpenSslSignatureSign>();
    p->setAlg(this);
    return p;
}

//---------------------------------------------------------------
common::SharedPtr<SignatureVerify> OpenSslSignatureAlg::createSignatureVerify() const
{
    auto p=common::makeShared<OpenSslSignatureVerify>();
    p->setAlg(this);
    return p;
}

/******************* OpenSslSignatureSign ********************/

//---------------------------------------------------------------
size_t OpenSslSignatureSign::resultSize() const
{
    EVP_PKEY* pkeyNative=detail::DigestSignPkeyTraits<CryptAlgorithm::Type::SIGNATURE>::nativeKey(this->key());
    if (pkeyNative==nullptr)
    {
        throw common::ErrorException(cryptError(CryptError::INVALID_SIGNATURE_STATE));
    }
    return static_cast<size_t>(EVP_PKEY_size(pkeyNative));
}

//---------------------------------------------------------------
size_t OpenSslSignatureSign::actualSignatureSize() const
{
    return m_sigSize.val;
}

//---------------------------------------------------------------
common::Error OpenSslSignatureSign::doProcess(
    const char* buf,
    size_t size
) noexcept
{
    if (::EVP_DigestSignUpdate(this->nativeHandler().handler,buf,size) != 1)
    {
        return makeLastSslError(CryptError::SIGN_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------
common::Error OpenSslSignatureSign::doFinalize(
        char* buf
    ) noexcept
{
    try {
        size_t len=0;
        if (::EVP_DigestSignFinal(this->nativeHandler().handler,NULL,&len) != 1)
        {
            return makeLastSslError(CryptError::SIGN_FAILED);
        }
        if (len>this->resultSize())
        {
            return cryptError(CryptError::INVALID_SIGNATURE_SIZE);
        }
        if (::EVP_DigestSignFinal(this->nativeHandler().handler,reinterpret_cast<unsigned char*>(buf),&len) != 1)
        {
            return makeLastSslError(CryptError::SIGN_FAILED);
        }
        m_sigSize.val=len;
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return Error();
}

//---------------------------------------------------------------
common::Error OpenSslSignatureSign::doInit() noexcept
{
    HATN_CHECK_RETURN(detail::DigestTraits::init(this))

    const EVP_MD* digest=nullptr;
    ENGINE* engine=this->alg()->engine()->template nativeHandler<ENGINE>();
    if (this->digestAlg()!=nullptr)
    {
        digest=this->digestAlg()->template nativeHandler<EVP_MD>();
    }

    EVP_PKEY* pkeyNative=detail::DigestSignPkeyTraits<CryptAlgorithm::Type::SIGNATURE>::nativeKey(this->key());
    if (pkeyNative==nullptr)
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }

    if (::EVP_DigestSignInit(this->nativeHandler().handler,NULL,
                       digest,
                       engine,
                       pkeyNative
                        ) != 1)
    {
        return makeLastSslError(CryptError::SIGN_FAILED);
    }

    return Error();
}

//---------------------------------------------------------------
void OpenSslSignatureSign::doReset() noexcept
{
    detail::DigestTraits::reset(this);
}

/******************* OpenSslSignatureVerify ********************/

//---------------------------------------------------------------
common::Error OpenSslSignatureVerify::doProcess(const char *buf, size_t size) noexcept
{
    if (::EVP_DigestVerifyUpdate(this->nativeHandler().handler,buf,size) != 1)
    {
        return makeLastSslError(CryptError::VERIFY_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------
common::Error OpenSslSignatureVerify::doVerify(const char *signature, size_t signatureLength) noexcept
{
    if (::EVP_DigestVerifyFinal(this->nativeHandler().handler,reinterpret_cast<const unsigned char*>(signature),signatureLength) != 1)
    {
        return makeLastSslError(CryptError::VERIFY_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------
common::Error OpenSslSignatureVerify::doInit() noexcept
{
    HATN_CHECK_RETURN(detail::DigestTraits::init(this))

    const EVP_MD* digest=nullptr;
    ENGINE* engine=this->alg()->engine()->template nativeHandler<ENGINE>();
    if (this->digestAlg()!=nullptr)
    {
        digest=this->digestAlg()->template nativeHandler<EVP_MD>();
    }

    if (!this->key()->isBackendKey())
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }
    const OpenSslPublicKey* pkey=common::dynamicCastWithSample(this->key(),&OpenSslPublicKey::masterReference());

    if (::EVP_DigestVerifyInit(this->nativeHandler().handler,NULL,
                       digest,
                       engine,
                       pkey->nativeHandler().handler
                      ) != 1)
    {
        return makeLastSslError(CryptError::VERIFY_FAILED);
    }

    return Error();
}

//---------------------------------------------------------------
void OpenSslSignatureVerify::doReset() noexcept
{
    detail::DigestTraits::reset(this);
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
