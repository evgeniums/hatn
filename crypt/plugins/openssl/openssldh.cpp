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

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/openssldh.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

namespace hatn {

using namespace common;

namespace crypt {
namespace openssl {

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
DHAlg::DHAlg(const CryptEngine *engine, const char *name, std::string paramName, std::string paramSha1) noexcept
    : CryptAlgorithm(engine,CryptAlgorithm::Type::DH,name,0,DHMethod::stub()),
      m_paramName(std::move(paramName)),
      m_paramSha1(std::move(paramSha1))
{}

//---------------------------------------------------------------
common::SharedPtr<PrivateKey> DHAlg::createPrivateKey() const
{
    auto key=makeShared<DHPrivateKey>();
    key->setAlg(this);
    return key;
}

//---------------------------------------------------------------
const char* DHAlg::paramStr(size_t index) const
{
    if (index==static_cast<size_t>(DHParamsStorage::AlgParam::Name))
    {
        return m_paramName.c_str();
    }
    if (index==static_cast<size_t>(DHParamsStorage::AlgParam::Sha1))
    {
        return m_paramSha1.c_str();
    }
    return nullptr;
}

/******************* OpenSslDH ********************/

//---------------------------------------------------------------
Error OpenSslDH::parseParameters(Native& dh, const char *data, size_t size) noexcept
{
    dh.reset();

    if (size==0 || data==nullptr)
    {
        return makeSystemError(std::errc::invalid_argument);
    }

    BIO *bio;
    HATN_CHECK_RETURN(createMemBio(bio,data,size))

    dh.handler=::PEM_read_bio_DHparams(bio,NULL,NULL,NULL);
    if (dh.isNull())
    {
        BIO_free(bio);
        return makeLastSslError();
    }
    BIO_free(bio);
    return Error();
}

//---------------------------------------------------------------
Error OpenSslDH::importState(
        common::SharedPtr<PrivateKey> privKey,
        common::SharedPtr<PublicKey> pubKey
    ) noexcept
{
    if (!isValid())
    {
        HATN_CHECK_RETURN(parseParameters())
    }

    BIGNUM* bPubKey=nullptr;
    BIGNUM* bPrivKey=nullptr;

    if (!privKey.isNull())
    {
        if (privKey->format()==ContainerFormat::RAW_PLAIN && !privKey->content().isEmpty())
        {
            bPrivKey=::BN_bin2bn(reinterpret_cast<unsigned const char*>(privKey->content().data()),static_cast<int>(privKey->content().size()),NULL);
        }
        else
        {
            MemoryLockedArray privBuf;
            HATN_CHECK_RETURN(privKey->exportToBuf(privBuf,ContainerFormat::RAW_PLAIN,true))
            if (!privBuf.empty())
            {
                auto privKeyData=reinterpret_cast<unsigned const char*>(privBuf.data());
                int privKeySize=static_cast<int>(privBuf.size());
                bPrivKey=::BN_bin2bn(privKeyData,privKeySize,NULL);
            }
        }
    }

    if (!pubKey.isNull())
    {
        if (pubKey->format()==ContainerFormat::RAW_PLAIN && !pubKey->content().isEmpty())
        {
            bPubKey=::BN_bin2bn(reinterpret_cast<unsigned const char*>(pubKey->content().data()),static_cast<int>(pubKey->content().size()),NULL);
        }
        else
        {
            ByteArray tmpBuf;
            HATN_CHECK_RETURN(pubKey->exportToBuf(tmpBuf,ContainerFormat::RAW_PLAIN));
            if (!tmpBuf.isEmpty())
            {
                bPubKey=::BN_bin2bn(reinterpret_cast<unsigned const char*>(tmpBuf.data()),static_cast<int>(tmpBuf.size()),NULL);
            }
        }
    }

    if (::DH_set0_key(nativeHandler().handler,bPubKey,bPrivKey)!=1)
    {
        return makeLastSslError();
    }

    return Error();
}

//---------------------------------------------------------------
Error OpenSslDH::exportState(common::SharedPtr<PrivateKey> &privKey, common::SharedPtr<PublicKey> &pubKey)
{
    // create or clear keys
    if (privKey.isNull())
    {
        privKey=makeShared<DHPrivateKey>();
        privKey->setAlg(alg());
    }
    else
    {
        privKey->content().clear();
    }
    if (pubKey.isNull())
    {
        pubKey=makeShared<PublicKey>();
        pubKey->setAlg(alg());
    }
    else
    {
        pubKey->content().clear();
    }

    if (!isValid())
    {
        HATN_CHECK_RETURN(parseParameters())
    }

    // generate key by backend (will use exisiting if already set)
    if (::DH_generate_key(nativeHandler().handler)!=1)
    {
        return makeLastSslError();
    }

    const BIGNUM* bPubKey=nullptr;
    const BIGNUM* bPrivKey=nullptr;

    // get native keys from backend
    ::DH_get0_key(nativeHandler().handler,&bPubKey,&bPrivKey);
    if (bPubKey==nullptr || bPrivKey==nullptr)
    {
        return makeLastSslError(CryptError::INVALID_DH_STATE);
    }

    // export private key
    bn2Container(bPrivKey,privKey->content());
    privKey->setContentProtected(false);
    privKey->setFormat(ContainerFormat::RAW_PLAIN);
    if (privKey->protector())
    {
        HATN_CHECK_RETURN(privKey->packContent())
    }

    // export public key
    bn2Container(bPubKey,pubKey->content());
    pubKey->setFormat(ContainerFormat::RAW_PLAIN);

    return Error();
}

//---------------------------------------------------------------
Error OpenSslDH::generateKey(common::SharedPtr<PublicKey> &pubKey)
{
    // generate or clear key
    if (pubKey.isNull())
    {
        pubKey=makeShared<PublicKey>();
        pubKey->setAlg(alg());
    }
    else
    {
        pubKey->content().clear();
    }

    if (!isValid())
    {
        HATN_CHECK_RETURN(parseParameters())
    }

    // generate key by backend (will use exisiting if already set)
    if (::DH_generate_key(nativeHandler().handler)!=1)
    {
        return makeLastSslError();
    }

    const BIGNUM* bPubKey=nullptr;
    const BIGNUM* bPrivKey=nullptr;

    // export public key
    ::DH_get0_key(nativeHandler().handler,&bPubKey,&bPrivKey);
    if (bPubKey==nullptr)
    {
        return makeLastSslError(CryptError::INVALID_DH_STATE);
    }
    bn2Container(bPubKey,pubKey->content());
    pubKey->setFormat(ContainerFormat::RAW_PLAIN);

    return Error();
}

//---------------------------------------------------------------
Error OpenSslDH::computeSecret(const char *peerPubKey, size_t peerPubKeySize, common::SharedPtr<DHSecret>& resultKey)
{
    if (resultKey.isNull())
    {
        resultKey=makeShared<DHSecret>();
        resultKey->setAlg(alg());
    }
    else
    {
        resultKey->content().clear();
    }

    if (!isValid())
    {
        HATN_CHECK_RETURN(parseParameters())
    }

    auto bPubKey=::BN_bin2bn(reinterpret_cast<unsigned const char*>(peerPubKey),static_cast<int>(peerPubKeySize),NULL);
    resultKey->content().resize(::DH_size(nativeHandler().handler));

    if (::DH_compute_key(reinterpret_cast<unsigned char*>(resultKey->content().data()),bPubKey,nativeHandler().handler)!=1)
    {
        ::BN_free(bPubKey);
        return makeLastSslError();
    }
    ::BN_free(bPubKey);

    if (resultKey->protector())
    {
        return resultKey->packContent();
    }
    else
    {
        resultKey->setContentProtected(false);
        resultKey->setFormat(ContainerFormat::RAW_PLAIN);
    }

    return Error();
}

//---------------------------------------------------------------
Error OpenSslDH::importParamsFromBuf(const char *buf, size_t size, ContainerFormat format, bool keepContent)
{
    if (format!=ContainerFormat::PEM)
    {
        return cryptError(CryptError::INVALID_CONTENT_FORMAT);
    }
    HATN_CHECK_RETURN(parseParameters(nativeHandler(),buf,size))
    if (keepContent)
    {
        loadContent(buf,size);
        setFormat(format);
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslDH::findNativeAlgorithm(std::shared_ptr<CryptAlgorithm> &alg, const char *name, CryptEngine *engine)
{
    std::vector<std::string> parts;
    splitAlgName(name,parts);
    if (parts.size()<1)
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }
    auto algName=parts[0];
    if (!boost::iequals(algName,std::string("dh")))
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }

    std::string paramName;
    if (parts.size()>1)
    {
        paramName=parts[1];
        boost::to_lower(paramName);
    }

    std::string paramSha1;
    if (parts.size()>2)
    {
        paramSha1=parts[2];
        boost::to_lower(paramSha1);
    }

    alg=std::make_shared<DHAlg>(engine,name,std::move(paramName),std::move(paramSha1));
    return Error();
}

//---------------------------------------------------------------
} // namespace openssl
HATN_CRYPT_NAMESPACE_END
