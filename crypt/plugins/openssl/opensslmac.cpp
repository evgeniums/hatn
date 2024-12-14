/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslmac.cpp
 *
 * 	MAC (Message Authentication Code) implementation with OpenSSL backend
 *
 */
/****************************************************************************/

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <boost/algorithm/string.hpp>

#include <hatn/common/meta/dynamiccastwithsample.h>

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/openssldigestsign.ipp>
#include <hatn/crypt/plugins/openssl/opensslmac.h>

HATN_OPENSSL_NAMESPACE_BEGIN

#if OPENSSL_API_LEVEL >= 30100

/******************* MACAlg ********************/

MACAlg& MACAlg::masterReference() noexcept
{
    static MACAlg inst(nullptr,nullptr,std::string{},0);
    return inst;
}

/******************* OpenSslMACT ********************/

//---------------------------------------------------------------

template <typename BaseT>
OpenSslMACT<BaseT>::OpenSslMACT(const SymmetricKey *key)
    : BaseT(key),
      m_mac(nullptr),
      m_macCtx(nullptr),
      m_resultSize(0)
{}

//---------------------------------------------------------------

template <typename BaseT>
OpenSslMACT<BaseT>::~OpenSslMACT()
{
    freeHandlers();
}

//---------------------------------------------------------------

template <typename BaseT>
void OpenSslMACT<BaseT>::freeHandlers()
{
    if (m_macCtx!=nullptr)
    {
        EVP_MAC_CTX_free(m_macCtx);
        m_macCtx=nullptr;
    }
    if (m_macCtx!=nullptr)
    {
        EVP_MAC_free(m_mac);
        m_mac=nullptr;
    }
}

//---------------------------------------------------------------

template <typename BaseT>
Error OpenSslMACT<BaseT>::findNativeAlgorithm(std::shared_ptr<CryptAlgorithm> &alg, const char *name, CryptEngine *engine) noexcept
{
    std::vector<std::string> parts;
    splitAlgName(name,parts);

    if (parts.size()<1)
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }

    const auto& algName=parts[0];
    if (boost::iequals(algName,std::string("poly1305")))
    {
        alg=std::make_shared<MACAlg_Poly1305>(engine,name);
    }
    else if (boost::iequals(algName,std::string("hmac")))
    {
        if (parts.size()<2)
        {
            return cryptError(CryptError::INVALID_ALGORITHM);
        }
        alg=std::make_shared<MACAlg_HMAC>(engine,name,parts[1].c_str());
    }
    else if (boost::iequals(algName,std::string("cmac")))
    {
        alg=std::make_shared<MACAlg_CMAC>(engine,name);
    }
    else if (boost::iequals(algName,std::string("siphash")))
    {
        size_t keySize=16;
        if (parts.size()==2)
        {
            try
            {
                keySize=std::stoi(parts[1]);
                if (keySize!=16 && keySize!=8)
                {
                    return cryptError(CryptError::INVALID_ALGORITHM);
                }
            }
            catch (...)
            {
                return cryptError(CryptError::INVALID_ALGORITHM);
            }
        }
        alg=std::make_shared<MACAlg_SIPHASH>(engine,name,keySize);
    }

    if (!alg || !alg->isValid())
    {
        alg.reset();
        return cryptError(CryptError::INVALID_ALGORITHM);
    }
    return common::Error();
}

//---------------------------------------------------------------

template <typename BaseT>
std::vector<std::string> OpenSslMACT<BaseT>::listMACs()
{
    std::vector<std::string> macs;

    macs.push_back("poly1305");
    macs.push_back("siphash/8");
    macs.push_back("siphash/16");
    macs.push_back("cmac");
    auto digests=OpenSslDigest::listDigests();
    for (auto&& it: digests)
    {
        macs.push_back(fmt::format("hmac/{}",it));
    }

    return macs;
}

//---------------------------------------------------------------

template <typename BaseT>
const CryptAlgorithm* OpenSslMACT<BaseT>::digestAlg() const noexcept
{
    const MACAlg* macAlg=common::dynamicCastWithSample(this->alg(),&MACAlg::masterReference());
    if (macAlg->nativeType()==EVP_PKEY_HMAC)
    {
        return this->alg();
    }
    return nullptr;
}

//---------------------------------------------------------------

template <typename BaseT>
common::Error OpenSslMACT<BaseT>::doProcess(const char *buf, size_t size) noexcept
{
    if (EVP_MAC_update(m_macCtx,reinterpret_cast<const unsigned char*>(buf),size) != OPENSSL_OK)
    {
        return makeLastSslError(CryptError::MAC_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------

template <typename BaseT>
common::Error OpenSslMACT<BaseT>::doFinalize(char *buf) noexcept
{
    size_t outL=0;
    if (EVP_MAC_final(m_macCtx,reinterpret_cast<unsigned char*>(buf),&outL,m_resultSize) != OPENSSL_OK)
    {
        return makeLastSslError(CryptError::MAC_FAILED);
    }
    if (m_resultSize!=outL)
    {
        return cryptError(CryptError::INVALID_MAC_SIZE);
    }
    return Error();
}

//---------------------------------------------------------------

template <typename BaseT>
common::Error OpenSslMACT<BaseT>::prepare() noexcept
{
    freeHandlers();

    if (
        this->key()->isBackendKey()
        ||
        this->key()->content().isEmpty()
        )
    {
        return cryptError(CryptError::INVALID_MAC_KEY);
    }

    const MACAlg* macAlg=common::dynamicCastWithSample(this->alg(),&MACAlg::masterReference());
    m_resultSize=macAlg->hashSize();

    // create handlers
    m_mac=EVP_MAC_fetch(NULL, macAlg->algName(), NULL);
    if (m_mac==nullptr)
    {
        //! @todo Implement more verbose error logging
        return cryptError(CryptError::INVALID_ALGORITHM);
    }
    m_macCtx=EVP_MAC_CTX_new(m_mac);
    if (m_macCtx==nullptr)
    {
        freeHandlers();
        return cryptError(CryptError::GENERAL_FAIL);
    }

    // prepare parameters
    constexpr const size_t MaxParamsCount=8;
    OSSL_PARAM params[MaxParamsCount+1];
    auto paramsCount=macAlg->prepareParams(params,MaxParamsCount);

    // init context
    int status=0;
    if (paramsCount==0)
    {
        status=EVP_MAC_init(m_macCtx, reinterpret_cast<const unsigned char*>(this->key()->content().data()), this->key()->content().size(),NULL);
    }
    else
    {
        params[paramsCount] = OSSL_PARAM_construct_end();
        status=EVP_MAC_init(m_macCtx, reinterpret_cast<const unsigned char*>(this->key()->content().data()), this->key()->content().size(),params);
    }
    if (status!=OPENSSL_OK)
    {
        auto ec=makeLastSslError(CryptError::MAC_FAILED);
        freeHandlers();
        return ec;
    }

    // done
    return OK;
}

//---------------------------------------------------------------

Error OpenSslHMAC::findNativeAlgorithm(std::shared_ptr<CryptAlgorithm> &alg, const char *name, CryptEngine *engine) noexcept
{
    std::vector<std::string> parts;
    splitAlgName(name,parts);

    if (parts.size()<1)
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }

    const auto& algName=parts[0];
    if (boost::iequals(algName,std::string("hmac")))
    {
        if (parts.size()<2)
        {
            return cryptError(CryptError::INVALID_ALGORITHM);
        }
        alg=std::make_shared<MACAlg_HMAC>(engine,name,parts[1].c_str(),CryptAlgorithm::Type::HMAC);
    }

    if (!alg || !alg->isValid())
    {
        alg.reset();
        return cryptError(CryptError::INVALID_ALGORITHM);
    }
    return common::Error();
}

//---------------------------------------------------------------

#ifndef _WIN32
template class OpenSslMACT<MAC>;
template class OpenSslMACT<HMAC>;
#endif

#endif

HATN_OPENSSL_NAMESPACE_END
