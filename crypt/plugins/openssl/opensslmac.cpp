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

/******************* MACAlg ********************/

MACAlg& MACAlg::masterReference() noexcept
{
    static MACAlg inst(nullptr,nullptr,0);
    return inst;
}

/******************* OpenSslMAC ********************/

#ifndef _WIN32
template class OpenSslDigestSign<MAC,CryptAlgorithm::Type::MAC>;
#endif

template <typename DigestT>
Error detail::DigestSignPkeyTraits<CryptAlgorithm::Type::MAC>::additionalInit(DigestT* digest,EVP_PKEY_CTX * pctx) noexcept
{
    // special case for 8-byte SIPHASH
    const MACAlg* macAlg=common::dynamicCastWithSample(digest->alg(),&MACAlg::masterReference());
    if (macAlg->nativeType()==EVP_PKEY_SIPHASH && digest->resultSize()==8)
    {
        if (EVP_PKEY_CTX_ctrl(pctx, -1, EVP_PKEY_OP_SIGNCTX, EVP_PKEY_CTRL_SET_DIGEST_SIZE, 8, NULL)!=1)
        {
            return makeLastSslError(FailedCode);
        }
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslMAC::findNativeAlgorithm(std::shared_ptr<CryptAlgorithm> &alg, const char *name, CryptEngine *engine) noexcept
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
std::vector<std::string> OpenSslMAC::listMACs()
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
const CryptAlgorithm* OpenSslMAC::digestAlg() const noexcept
{
    const MACAlg* macAlg=common::dynamicCastWithSample(alg(),&MACAlg::masterReference());
    if (macAlg->nativeType()==EVP_PKEY_HMAC)
    {
        return alg();
    }
    return nullptr;
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
