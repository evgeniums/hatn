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

#ifndef HATNOPENSSLDIGESTSIGN_IPP
#define HATNOPENSSLDIGESTSIGN_IPP

#include <hatn/crypt/plugins/openssl/openssldigestsign.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//---------------------------------------------------------------
template <typename BaseT,CryptAlgorithm::Type AlgT>
common::Error OpenSslDigestSign<BaseT,AlgT>::doProcess(
    const char* buf,
    size_t size
) noexcept
{
    if (::EVP_DigestSignUpdate(this->nativeHandler().handler,buf,size) != 1)
    {
        return makeLastSslError(detail::DigestSignPkeyTraits<AlgT>::FailedCode);
    }
    return Error();
}

//---------------------------------------------------------------
template <typename BaseT,CryptAlgorithm::Type AlgT>
common::Error OpenSslDigestSign<BaseT,AlgT>::doFinalize(
        char* buf
    ) noexcept
{
    size_t len=0;
    if (::EVP_DigestSignFinal(this->nativeHandler().handler,NULL,&len) != 1)
    {
        return makeLastSslError(detail::DigestSignPkeyTraits<AlgT>::FailedCode);
    }
    len=(std::min)(len,this->resultSize());
    if (::EVP_DigestSignFinal(this->nativeHandler().handler,reinterpret_cast<unsigned char*>(buf),&len) != 1)
    {
        return makeLastSslError(detail::DigestSignPkeyTraits<AlgT>::FailedCode);
    }
    return Error();
}

//---------------------------------------------------------------
template <typename BaseT,CryptAlgorithm::Type AlgT>
common::Error OpenSslDigestSign<BaseT,AlgT>::doInit() noexcept
{
    HATN_CHECK_RETURN(detail::DigestTraits::init(this))

    const EVP_MD* digest=nullptr;
    ENGINE* engine=this->alg()->engine()->template nativeHandler<ENGINE>();
    if (this->digestAlg()!=nullptr)
    {
        digest=this->digestAlg()->template nativeHandler<EVP_MD>();
        if (digest!=nullptr)
        {
            if (::EVP_DigestInit_ex(this->nativeHandler().handler,
                               digest,
                               engine
                                ) != 1)
            {
                return makeLastSslError(detail::DigestSignPkeyTraits<AlgT>::FailedCode);
            }
        }
    }

    EVP_PKEY* pkeyNative=detail::DigestSignPkeyTraits<AlgT>::nativeKey(this->key());
    if (pkeyNative==nullptr)
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }
    EVP_PKEY_CTX * pctx=nullptr;
    if (::EVP_DigestSignInit(this->nativeHandler().handler,&pctx,
                       digest,
                       engine,
                       pkeyNative
                        ) != 1)
    {
        return makeLastSslError(detail::DigestSignPkeyTraits<AlgT>::FailedCode);
    }

    return detail::DigestSignPkeyTraits<AlgT>::additionalInit(this,pctx);
}

//---------------------------------------------------------------
template <typename BaseT,CryptAlgorithm::Type AlgT>
void OpenSslDigestSign<BaseT,AlgT>::doReset() noexcept
{
    detail::DigestTraits::reset(this);
}

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLDIGESTSIGN_IPP
