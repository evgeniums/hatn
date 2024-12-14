/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/dhlegacy.cpp
 *
 * 	Legacy DH types.
 *
 */
/****************************************************************************/

#include <boost/algorithm/string/case_conv.hpp>

#include <hatn/thirdparty/sha1/sha1.h>
#include <hatn/common/containerutils.h>
#include <hatn/crypt/dhlegacy.h>

HATN_CRYPT_NAMESPACE_BEGIN
HATN_COMMON_USING

#ifdef HATN_CRYPT_LEGACY_DH

static const char g_dh1024_sz[] =
    "-----BEGIN DH PARAMETERS-----\n"
    "MIGHAoGBAP//////////yQ/aoiFowjTExmKLgNwc0SkCTgiKZ8x0Agu+pjsTmyJR\n"
    "Sgh5jjQE3e+VGbPNOkMbMCsKbfJfFDdP4TVtbVHCReSFtXZiXn7G9ExC6aY37WsL\n"
    "/1y29Aa37e44a/taiZ+lrp8kEXxLH+ZJKGZR7OZTgf//////////AgEC\n"
    "-----END DH PARAMETERS-----";
common::ByteArray DHRfc3526::group1024() noexcept
{
    return common::ByteArray(&g_dh1024_sz[0],sizeof(g_dh1024_sz)-1,true);
}

static const char g_dh1536_sz[] = "-----BEGIN DH PARAMETERS-----\n"
    "MIHHAoHBAP//////////yQ/aoiFowjTExmKLgNwc0SkCTgiKZ8x0Agu+pjsTmyJR\n"
    "Sgh5jjQE3e+VGbPNOkMbMCsKbfJfFDdP4TVtbVHCReSFtXZiXn7G9ExC6aY37WsL\n"
    "/1y29Aa37e44a/taiZ+lrp8kEXxLH+ZJKGZR7ORbPcIAfLihY78FmNpINhxV05pp\n"
    "Fj+o/STPX4NlXSPco62WHGLzViCFUrue1SkHcJaWbWcMNU5KvJgE8XRsCMojcyf/\n"
    "/////////wIBAg==\n"
    "-----END DH PARAMETERS-----";
common::ByteArray DHRfc3526::group1536() noexcept
{
    return common::ByteArray(&g_dh1536_sz[0],sizeof(g_dh1536_sz)-1,true);
}

static const char g_dh2048_sz[] =
    "-----BEGIN DH PARAMETERS-----\n"
    "MIIBCAKCAQEA///////////JD9qiIWjCNMTGYouA3BzRKQJOCIpnzHQCC76mOxOb\n"
    "IlFKCHmONATd75UZs806QxswKwpt8l8UN0/hNW1tUcJF5IW1dmJefsb0TELppjft\n"
    "awv/XLb0Brft7jhr+1qJn6WunyQRfEsf5kkoZlHs5Fs9wgB8uKFjvwWY2kg2HFXT\n"
    "mmkWP6j9JM9fg2VdI9yjrZYcYvNWIIVSu57VKQdwlpZtZww1Tkq8mATxdGwIyhgh\n"
    "fDKQXkYuNs474553LBgOhgObJ4Oi7Aeij7XFXfBvTFLJ3ivL9pVYFxg5lUl86pVq\n"
    "5RXSJhiY+gUQFXKOWoqsqmj//////////wIBAg==\n"
    "-----END DH PARAMETERS-----";
common::ByteArray DHRfc3526::group2048() noexcept
{
    return common::ByteArray(&g_dh2048_sz[0],sizeof(g_dh2048_sz)-1,true);
}

static const char g_dh3072_sz[] =
    "-----BEGIN DH PARAMETERS-----\n"
    "MIIBiAKCAYEA///////////JD9qiIWjCNMTGYouA3BzRKQJOCIpnzHQCC76mOxOb\n"
    "IlFKCHmONATd75UZs806QxswKwpt8l8UN0/hNW1tUcJF5IW1dmJefsb0TELppjft\n"
    "awv/XLb0Brft7jhr+1qJn6WunyQRfEsf5kkoZlHs5Fs9wgB8uKFjvwWY2kg2HFXT\n"
    "mmkWP6j9JM9fg2VdI9yjrZYcYvNWIIVSu57VKQdwlpZtZww1Tkq8mATxdGwIyhgh\n"
    "fDKQXkYuNs474553LBgOhgObJ4Oi7Aeij7XFXfBvTFLJ3ivL9pVYFxg5lUl86pVq\n"
    "5RXSJhiY+gUQFXKOWoqqxC2tMxcNBFB6M6hVIavfHLpk7PuFBFjb7wqK6nFXXQYM\n"
    "fbOXD4Wm4eTHq/WujNsJM9cejJTgSiVhnc7j0iYa0u5r8S/6BtmKCGTYdgJzPshq\n"
    "ZFIfKxgXeyAMu+EXV3phXWx3CYjAutlG4gjiT6B05asxQ9tb/OD9EI5LgtEgqTrS\n"
    "yv//////////AgEC\n"
    "-----END DH PARAMETERS-----";
common::ByteArray DHRfc3526::group3072() noexcept
{
    return common::ByteArray(&g_dh3072_sz[0],sizeof(g_dh3072_sz)-1,true);
}

static const char g_dh4096_sz[] =
    "-----BEGIN DH PARAMETERS-----\n"
    "MIICCAKCAgEA///////////JD9qiIWjCNMTGYouA3BzRKQJOCIpnzHQCC76mOxOb\n"
    "IlFKCHmONATd75UZs806QxswKwpt8l8UN0/hNW1tUcJF5IW1dmJefsb0TELppjft\n"
    "awv/XLb0Brft7jhr+1qJn6WunyQRfEsf5kkoZlHs5Fs9wgB8uKFjvwWY2kg2HFXT\n"
    "mmkWP6j9JM9fg2VdI9yjrZYcYvNWIIVSu57VKQdwlpZtZww1Tkq8mATxdGwIyhgh\n"
    "fDKQXkYuNs474553LBgOhgObJ4Oi7Aeij7XFXfBvTFLJ3ivL9pVYFxg5lUl86pVq\n"
    "5RXSJhiY+gUQFXKOWoqqxC2tMxcNBFB6M6hVIavfHLpk7PuFBFjb7wqK6nFXXQYM\n"
    "fbOXD4Wm4eTHq/WujNsJM9cejJTgSiVhnc7j0iYa0u5r8S/6BtmKCGTYdgJzPshq\n"
    "ZFIfKxgXeyAMu+EXV3phXWx3CYjAutlG4gjiT6B05asxQ9tb/OD9EI5LgtEgqSEI\n"
    "ARpyPBKnh+bXiHGaEL26WyaZwycYavTiPBqUaDS2FQvaJYPpyirUTOjbu8LbBN6O\n"
    "+S6O/BQfvsqmKHxZR05rwF2ZspZPoJDDoiM7oYZRW+ftH2EpcM7i16+4G912IXBI\n"
    "HNAGkSfVsFqpk7TqmI2P3cGG/7fckKbAj030Nck0BjGZ//////////8CAQI=\n"
    "-----END DH PARAMETERS-----";
common::ByteArray DHRfc3526::group4096() noexcept
{
    return common::ByteArray(&g_dh4096_sz[0],sizeof(g_dh4096_sz)-1,true);
}

std::string DHRfc3526::group1024Name()
{
    return std::string("rfc4526-g1024");
}
std::string DHRfc3526::group1536Name()
{
    return std::string("rfc4526-g1536");
}
std::string DHRfc3526::group2048Name()
{
    return std::string("rfc4526-g2048");
}
std::string DHRfc3526::group3072Name()
{
    return std::string("rfc4526-g3072");
}
std::string DHRfc3526::group4096Name()
{
    return std::string("rfc4526-g4096");
}

/*********************** DH **************************/

//---------------------------------------------------------------
common::Error DH::importParamsFromStorage(const std::string &name, const std::string &sha1, bool keepContent)
{
    auto params=DHParamsStorage::instance()->findParamsByName(name);
    if (!params)
    {
        params=DHParamsStorage::instance()->findParamsBySha1(sha1);
    }
    if (!params)
    {
        return cryptError(CryptError::DH_PARAMS_NOT_FOUND);
    }
    return importParamsFromBuf(params->data,params->format,keepContent);
}

//---------------------------------------------------------------
common::Error DH::init(const CryptAlgorithm *alg)
{
    if (alg!=nullptr)
    {
        m_alg.ptr=alg;
    }
    if (m_alg.isNull() || !m_alg->isType(CryptAlgorithm::Type::DH))
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }
    return importParamsFromStorage(
                    std::string(m_alg->paramStr(static_cast<size_t>(DHParamsStorage::AlgParam::Name))),
                    std::string(m_alg->paramStr(static_cast<size_t>(DHParamsStorage::AlgParam::Sha1)))
                );
}

/*********************** DHParamsStorage **************************/

HATN_SINGLETON_INIT(DHParamsStorage)

static DHParamsStorage* Instance=nullptr;

//---------------------------------------------------------------
DHParamsStorage* DHParamsStorage::instance()
{
    if (Instance==nullptr)
    {
        Instance=new DHParamsStorage();
        Instance->addParams(DHRfc3526::group1024Name(),DHRfc3526::group1024());
        Instance->addParams(DHRfc3526::group1536Name(),DHRfc3526::group1536());
        Instance->addParams(DHRfc3526::group2048Name(),DHRfc3526::group2048());
        Instance->addParams(DHRfc3526::group3072Name(),DHRfc3526::group3072());
        Instance->addParams(DHRfc3526::group4096Name(),DHRfc3526::group4096());
    }
    return Instance;
}

//---------------------------------------------------------------
void DHParamsStorage::free()
{
    delete Instance;
    Instance=nullptr;
}

//---------------------------------------------------------------
void DHParamsStorage::addParams(const std::shared_ptr<Params>& params)
{
    if (params->name.empty())
    {
        params->name=params->sha1;
    }
    boost::to_lower(params->name);
    boost::to_lower(params->sha1);

    Assert(!params->name.empty() && !params->sha1.empty() && !params->data.empty(),"Invalid DH parameters");

    m_paramsByName[params->name]=params;
    m_paramsBySha1[params->sha1]=params;
}

//---------------------------------------------------------------
void DHParamsStorage::addParams(std::string name, common::ByteArray content, ContainerFormat format)
{
    std::string sha1;
    if (format==ContainerFormat::PEM)
    {
        ByteArray tmpRaw;
        common::ContainerUtils::base64ToRaw(content,tmpRaw);
        sha1=common::SHA1::containerHash(tmpRaw);
    }
    else
    {
        sha1=common::SHA1::containerHash(content);
    }

    auto params=std::make_shared<Params>(
                        std::move(content),
                        std::move(name),
                        std::move(sha1),
                        format
                );
    addParams(params);
}

//---------------------------------------------------------------
void DHParamsStorage::addParams(std::string name, const std::string &fileName, ContainerFormat format)
{
    common::ByteArray content;
    HATN_CHECK_THROW(content.loadFromFile(fileName))
    addParams(std::move(name),std::move(content),format);
}

//---------------------------------------------------------------
std::shared_ptr<DHParamsStorage::Params> DHParamsStorage::findParamsByName(const std::string& name) const noexcept
{
    auto it=m_paramsByName.find(name);
    if (it!=m_paramsByName.end())
    {
        return it->second;
    }
    return std::shared_ptr<DHParamsStorage::Params>();
}

//---------------------------------------------------------------
std::shared_ptr<DHParamsStorage::Params> DHParamsStorage::findParamsBySha1(const std::string& sha1) const noexcept
{
    auto it=m_paramsBySha1.find(sha1);
    if (it!=m_paramsBySha1.end())
    {
        return it->second;
    }
    return std::shared_ptr<DHParamsStorage::Params>();
}

//---------------------------------------------------------------
void DHParamsStorage::removeParamsByName(const std::string &name)
{
    auto params=findParamsByName(name);
    if (params)
    {
        m_paramsBySha1.erase(params->sha1);
        m_paramsByName.erase(params->name);
    }
}

//---------------------------------------------------------------
void DHParamsStorage::removeParamsBySha1(const std::string &sha1)
{
    auto params=findParamsBySha1(sha1);
    if (params)
    {
        m_paramsBySha1.erase(params->sha1);
        m_paramsByName.erase(params->name);
    }
}
#endif

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
