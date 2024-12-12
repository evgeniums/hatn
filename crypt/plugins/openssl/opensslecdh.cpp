/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslecdh.cpp
 *
 * 	Elliptic curves Diffie-Hellmann processing
 *
 */
/****************************************************************************/

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslecdh.h>
#include <hatn/crypt/plugins/openssl/opensslpublickey.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

namespace hatn {

using namespace common;

namespace crypt {
namespace openssl {

/******************* OpenSslECDH ********************/

//---------------------------------------------------------------
Error OpenSslECDH::generateKey(common::SharedPtr<PublicKey> &pubKey)
{
    if (pubKey.isNull())
    {
        pubKey=makeShared<OpenSslPublicKey>();
        pubKey->setAlg(alg());
    }

    if (m_privKey.isNull() || m_privKey->isNull())
    {
        m_privKey=alg()->createPrivateKey();
        if (!m_privKey)
        {
            return cryptError(CryptError::INVALID_ALGORITHM);
        }
        HATN_CHECK_RETURN(m_privKey->generate())
    }

    return pubKey->derive(*m_privKey);
}

//---------------------------------------------------------------
Error OpenSslECDH::computeSecret(const common::SharedPtr<PublicKey> &peerPubKey, common::SharedPtr<DHSecret> &result)
{
    if (m_privKey.isNull())
    {
        return cryptError(CryptError::INVALID_DH_STATE);
    }

    if (peerPubKey.isNull() || peerPubKey->alg()!=alg())
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }
    // slow dynamic cast is ok in long cryptographic functions
    OpenSslPublicKey* pubKey=dynamic_cast<OpenSslPublicKey*>(peerPubKey.get());
    if (pubKey==nullptr)
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }

    if (!peerPubKey->isNativeValid())
    {
        HATN_CHECK_RETURN(peerPubKey->unpackContent())
    }

    NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> ctx(::EVP_PKEY_CTX_new(m_privKey->nativeHandler().handler, NULL));
    if (ctx.isNull())
    {
        return makeLastSslError(CryptError::DH_FAILED);
    }
    if (::EVP_PKEY_derive_init(ctx.handler)!=1)
    {
        return makeLastSslError(CryptError::DH_FAILED);
    }
    if (::EVP_PKEY_derive_set_peer(ctx.handler,pubKey->nativeHandler().handler)!=1)
    {
        return makeLastSslError(CryptError::DH_FAILED);
    }
    size_t secretLen=0;
    if (::EVP_PKEY_derive(ctx.handler,NULL,&secretLen)!=1)
    {
        return makeLastSslError(CryptError::DH_FAILED);
    }
    if (secretLen==0)
    {
        return makeLastSslError(CryptError::DH_FAILED);
    }

    if (result.isNull())
    {
        result=makeShared<DHSecret>();
        result->setAlg(alg());
    }
    else
    {
        result->content().clear();
    }
    result->content().resize(secretLen);
    if (::EVP_PKEY_derive(ctx.handler,reinterpret_cast<unsigned char*>(result->content().data()),&secretLen)!=1)
    {
        result->content().clear();
        return makeLastSslError(CryptError::DH_FAILED);
    }
    result->setContentProtected(false);
    result->setFormat(ContainerFormat::RAW_PLAIN);

    return Error();
}

//---------------------------------------------------------------
} // namespace openssl
HATN_CRYPT_NAMESPACE_END
