/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssldh.cpp
 *
 * 	Diffie-Hellmann routines and data
 *
 */
/****************************************************************************/

#include <openssl/core_names.h>

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslpublickey.h>
#include <hatn/crypt/plugins/openssl/openssldh.h>
#include <hatn/crypt/plugins/openssl/opensslecdh.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* DHAlg ********************/

//! DH method stub
class DHMethod
{
    public:
        static const DHMethod* stub()
        {
            static DHMethod m;
            return &m;
        }
};

//---------------------------------------------------------------
Error DHPrivateKey::doGenerate()
{
    // create ctx
    common::NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> pctx(
        EVP_PKEY_CTX_new_from_name(NULL,"DH",NULL)
        );
    if (pctx.isNull())
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }

    // init ctx
    if (EVP_PKEY_keygen_init(pctx.handler) != OPENSSL_OK)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }

    // load group name to context
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, const_cast<char*>(alg()->name()), 0);
    params[1] = OSSL_PARAM_construct_end();
    if (EVP_PKEY_CTX_set_params(pctx.handler, params) != OPENSSL_OK)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }

    // generate key
    EVP_PKEY *pkey = NULL;
    if (EVP_PKEY_generate(pctx.handler, &pkey) != OPENSSL_OK)
    {
        return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
    }

    // done
    setNativeHandler(pkey);
    return OK;
}

//---------------------------------------------------------------
DHAlg::DHAlg(const CryptEngine *engine, const char *name) noexcept
    : CryptAlgorithm(engine,CryptAlgorithm::Type::DH,name,0,DHMethod::stub())
{}

//---------------------------------------------------------------
common::SharedPtr<PrivateKey> DHAlg::createPrivateKey() const
{
    auto key=common::makeShared<DHPrivateKey>();
    key->setAlg(this);
    return key;
}

/******************* OpenSslDHT *******************/

//---------------------------------------------------------------

template <typename BaseT>
Error OpenSslDHT<BaseT>::generateKey(common::SharedPtr<PublicKey> &pubKey)
{
    if (pubKey.isNull())
    {
        pubKey=common::makeShared<OpenSslPublicKey>();
        pubKey->setAlg(this->alg());
    }

    if (m_privKey.isNull() || m_privKey->isNull())
    {
        m_privKey=this->alg()->createPrivateKey();
        HATN_CHECK_RETURN(m_privKey->generate())
    }

    return pubKey->derive(*m_privKey);
}

//---------------------------------------------------------------

template <typename BaseT>
Error OpenSslDHT<BaseT>::computeSecret(const common::SharedPtr<PublicKey> &peerPubKey, common::SharedPtr<DHSecret> &result)
{
    if (m_privKey.isNull())
    {
        return cryptError(CryptError::INVALID_DH_STATE);
    }

    if (peerPubKey.isNull() || peerPubKey->alg()!=this->alg())
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

    common::NativeHandler<EVP_PKEY_CTX,detail::PkeyCtxTraits> ctx(::EVP_PKEY_CTX_new(m_privKey->nativeHandler().handler, NULL));
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
        result=common::makeShared<DHSecret>();
        result->setAlg(this->alg());
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

/******************* OpenSslDH ********************/

static const std::set<std::string>& dhGroups()
{
    static std::set<std::string> groups{
        "ffdhe2048", "ffdhe3072", "ffdhe4096", "ffdhe6144", "ffdhe8192",
        "modp_2048", "modp_3072", "modp_4096", "modp_6144", "modp_8192",
        "modp_1536", "dh_1024_160", "dh_2048_224", "dh_2048_256"
    };
    return groups;
}

//---------------------------------------------------------------
Error OpenSslDH::findNativeAlgorithm(std::shared_ptr<CryptAlgorithm> &alg, const char *name, CryptEngine *engine)
{
    // check if name is in the list of supported safe prime groups.
    auto it=dhGroups().find(name);
    if (it==dhGroups().end())
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }

    // create and return alg
    alg=std::make_shared<DHAlg>(engine,name);
    return Error();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslDH::listDHs()
{
    std::vector<std::string> v(dhGroups().begin(),dhGroups().end());
    return v;
}

//---------------------------------------------------------------

#ifndef _WIN32
template class OpenSslDHT<DH>;
template class OpenSslDHT<ECDH>;
#endif

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
