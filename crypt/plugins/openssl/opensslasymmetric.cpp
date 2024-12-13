/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslasymmetric.cpp
 *
 * 	Asymmetric cryptography with OpenSSL backend
 *
 */
/****************************************************************************/

#include <openssl/evp.h>
#include <openssl/core_names.h>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <boost/algorithm/string.hpp>

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslpublickey.h>
#include <hatn/crypt/plugins/openssl/opensslasymmetric.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslAsymmetric ********************/

//---------------------------------------------------------------
common::Error OpenSslAsymmetric::findNativeAlgorithm(
        CryptAlgorithm::Type type,
        std::shared_ptr<CryptAlgorithm> &alg,
        const char *name,
        CryptEngine *engine
    ) noexcept
{
    std::vector<std::string> parts;
    splitAlgName(name,parts);

    if (parts.size()<1)
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }

    const auto& algName=parts[0];
    if (boost::iequals(algName,std::string("ED")))
    {
        if (type!=CryptAlgorithm::Type::SIGNATURE)
        {
            return cryptError(CryptError::INVALID_ALGORITHM);
        }
        alg=std::make_shared<EDAlg>(engine,type,name);
    }
    else if (boost::iequals(algName,std::string("EC")))
    {
        alg=std::make_shared<ECAlg>(engine,type,name,parts);
    }
    if (boost::iequals(algName,std::string("RSA")))
    {
        alg=std::make_shared<RSAAlg>(engine,type,name,parts);
    }
    else if (boost::iequals(algName,std::string("DSA")))
    {
        if (type!=CryptAlgorithm::Type::SIGNATURE)
        {
            return cryptError(CryptError::INVALID_ALGORITHM);
        }
        alg=std::make_shared<DSAAlg>(engine,name,parts);
    }

    if (!alg || !alg->isValid())
    {
        alg.reset();
        return cryptError(CryptError::INVALID_ALGORITHM);
    }

    return common::Error();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslAsymmetric::listSignatures()
{
    std::vector<std::string> algs;

    algs.push_back("EC/<curve_name>");
    algs.push_back("ED/448");
    algs.push_back("ED/25519");
    algs.push_back("RSA/<key-bits>");
    algs.push_back("DSA/<n-bits=2048|3072|1024>[/<q-bits=256|224|160>]");

    return algs;
}

#if OPENSSL_API_LEVEL >= 30100

//---------------------------------------------------------------
static Error nativeToAlgorithm(CryptAlgorithmConstP &alg, EVP_PKEY* nativeKey, CryptPlugin* plugin, const char* engineName)
{
    std::string algName;
    int keyID=::EVP_PKEY_base_id(nativeKey);

    if (keyID==EVP_PKEY_ED25519)
    {
        algName="ED/25519";
    }
    else if (keyID==EVP_PKEY_ED448)
    {
        algName="ED/448";
    }
    else
    {
        OSSL_PARAM *keyParams;
        if (EVP_PKEY_todata(nativeKey, EVP_PKEY_PUBLIC_KEY, &keyParams) != OPENSSL_OK)
        {
            return makeLastSslError(CryptError::INVALID_KEY_TYPE);
        }

        for (size_t i=0; keyParams[i].key != NULL; i++)
        {
            const auto* param=&keyParams[i];
            const auto* key=param->key;
            bool found=false;
            switch (keyID)
            {
                case (EVP_PKEY_EC):
                {
                    if (strcmp(key, OSSL_PKEY_PARAM_GROUP_NAME) == 0)
                    {
                        const char* curveName=NULL;
                        if (OSSL_PARAM_get_utf8_string_ptr(&keyParams[i],&curveName)!=OPENSSL_OK || curveName==NULL)
                        {
                            OSSL_PARAM_free(keyParams);
                            return makeLastSslError(CryptError::INVALID_KEY_TYPE);
                        }
                        algName=fmt::format("EC/{}",curveName);
                        found=true;
                    }
                }
                break;

                case (EVP_PKEY_RSA):
                {
                    if (strcmp(key, OSSL_PKEY_PARAM_RSA_N) == 0)
                    {
                        // calculate bits from data_size that is presented in bytes
                        auto bits=keyParams[i].data_size*8;
                        algName=fmt::format("RSA/{}",bits);
                        found=true;
                    }
                }
                break;

                case (EVP_PKEY_DSA):
                {
                    if (strcmp(key, OSSL_PKEY_PARAM_FFC_P) == 0)
                    {
                        // params do not contain OSSL_PKEY_PARAM_FFC_PBITS as expected
                        // calculate bits from data_size of OSSL_PKEY_PARAM_FFC_P that is presented in bytes
                        auto bits=keyParams[i].data_size*8;
                        algName=fmt::format("DSA/{}",bits);
                        found=true;
                    }
                }
                break;

                default:
                    break;
            }
            if (found)
            {
                break;
            }
        }
        OSSL_PARAM_free(keyParams);
    }

    return plugin->findAlgorithm(alg,CryptAlgorithm::Type::SIGNATURE,algName,engineName);
}

#else

//---------------------------------------------------------------
static Error nativeToAlgorithm(CryptAlgorithmConstP &alg, EVP_PKEY* nativeKey, CryptPlugin* plugin, const char* engineName)
{
    std::string algName;
    int keyID=::EVP_PKEY_base_id(nativeKey);
    switch (keyID)
    {
    case (EVP_PKEY_ED25519):
    {
        algName="ED/25519";
    }
    break;

    case (EVP_PKEY_ED448):
    {
        algName="ED/448";
    }
    break;

    case (EVP_PKEY_EC):
    {
        EC_KEY* ecKey=::EVP_PKEY_get0_EC_KEY(nativeKey);
        if (ecKey)
        {
            auto group=::EC_KEY_get0_group(ecKey);
            if (group)
            {
                auto nid=::EC_GROUP_get_curve_name(group);
                auto curveName=::OBJ_nid2sn(nid);
                algName=fmt::format("EC/{}",curveName);
            }
        }
    }
    break;

    case (EVP_PKEY_RSA):
    {
        RSA* rsaKey=::EVP_PKEY_get0_RSA(nativeKey);
        if (rsaKey)
        {
            auto bits=::RSA_bits(rsaKey);
            algName=fmt::format("RSA/{}",bits);
        }
    }
    break;

    case (EVP_PKEY_DSA):
    {
        DSA* dsaKey=::EVP_PKEY_get0_DSA(nativeKey);
        if (dsaKey)
        {
            auto bits=::DSA_bits(dsaKey);
            algName=fmt::format("DSA/{}",bits);
        }
    }
    break;

    default:
        break;
    }

    return plugin->findAlgorithm(alg,CryptAlgorithm::Type::SIGNATURE,algName,engineName);
}

#endif


//---------------------------------------------------------------
Error OpenSslAsymmetric::createPrivateKeyFromContent(        
        common::SharedPtr<PrivateKey>& pkey,
        const char* buf,
        size_t size,
        ContainerFormat format,
        CryptPlugin* plugin,
        KeyProtector* protector,
        const char* engineName
    )
{
    pkey.reset();
    OpenSslPrivateKey tmpPkey;
    tmpPkey.setProtector(protector);
    HATN_CHECK_RETURN(tmpPkey.importFromBuf(buf,size,format));

    const CryptAlgorithm* alg=nullptr;
    HATN_CHECK_RETURN(nativeToAlgorithm(alg,tmpPkey.nativeHandler().handler,plugin,engineName));

    if (alg)
    {
        pkey=alg->createPrivateKey();
        if (pkey)
        {
            return pkey->importFromBuf(buf,size,format);
        }
    }

    return cryptError(CryptError::INVALID_KEY_TYPE);
}

//---------------------------------------------------------------
Error OpenSslAsymmetric::publicKeyAlgorithm(CryptAlgorithmConstP &alg, const PublicKey *key, CryptPlugin *plugin, const char *engineName)
{
    const OpenSslPublicKey* pkey=dynamic_cast<const OpenSslPublicKey*>(key);
    if (pkey)
    {
        return nativeToAlgorithm(alg,pkey->nativeHandler().handler,plugin,engineName);
    }
    return cryptError(CryptError::INVALID_KEY_TYPE);
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
