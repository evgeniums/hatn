/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/cryptplugin.cpp
  *
  *   Base class for encryption plugins
  *
  */

/****************************************************************************/

#include <iostream>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>

#include <hatn/crypt/encryptmac.h>
#include <hatn/crypt/ciphernonealgorithm.h>

#include <hatn/crypt/cryptplugin.h>

namespace hatn {

using namespace common;

namespace crypt {

/*********************** CryptPlugin **************************/

//---------------------------------------------------------------
CryptPlugin::CryptPlugin(
        const common::PluginInfo* pluginInfo
    ) noexcept : common::Plugin(pluginInfo),
                m_defaultEngine(std::make_shared<CryptEngine>(this))
{
}

//---------------------------------------------------------------
int CryptPlugin::constTimeMemCmp(const void *in_a, const void *in_b, size_t len) const
{
    size_t i=0;
    const volatile unsigned char *a = reinterpret_cast<const volatile unsigned char *>(in_a);
    const volatile unsigned char *b = reinterpret_cast<const volatile unsigned char *>(in_b);
    unsigned char x = 0;

    for (i = 0; i < len; i++)
        x |= a[i] ^ b[i];

    return x;
}

//---------------------------------------------------------------
common::Error CryptPlugin::findEngine(const char *engineName, std::shared_ptr<CryptEngine> &engine)
{
    if (engineName==nullptr)
    {
        engine=m_defaultEngine;
        return common::Error();
    }
    {
        common::MutexScopedLock l(m_algMutex);

        auto it=m_engines.find(engineName);
        if (it==m_engines.end())
        {
            HATN_CHECK_RETURN(doFindEngine(engineName,engine))
            m_engines[engine->name()]=engine;
        }
        else
        {
            engine=it->second;
        }
    }
    return common::Error();
}

//---------------------------------------------------------------
common::Error CryptPlugin::findAlgorithm(CryptAlgorithmConstP &alg, CryptAlgorithm::Type type, const char *name, const char *engineName)
{
    std::shared_ptr<CryptEngine> engine;
    HATN_CHECK_RETURN(findEngine(engineName,engine));

    {
        m_algMutex.lock();

        CryptAlgorithmMapKey key{name,engineName};
        auto it=m_algs.find(type);
        if (it!=m_algs.end())
        {
            const auto& algs=it->second;
            auto it1=algs->find(key);
            if (it1!=algs->end())
            {
                alg=it1->second.get();
                m_algMutex.unlock();
                return common::Error();
            }
        }
        else
        {
            m_algs[type]=std::make_shared<AlgTable>();
            it=m_algs.find(type);
        }

        std::shared_ptr<CryptAlgorithm> algP;

        if (type==CryptAlgorithm::Type::SENCRYPTION)
        {
            // special case for none cipher
            std::string algName(name);
            if (
                    algName=="-"
                    ||
                    boost::algorithm::iequals(algName,"none")
                )
            {
                algP=std::make_shared<CipherNoneAlgorithm>(engine.get(),name);
            }
        }
        else if (type==CryptAlgorithm::Type::AEAD)
        {
            // special case for compound AEAD algorithms

            std::string algName(name);
            if (boost::algorithm::istarts_with(algName,"encryptmac:"))
            {
                std::vector<std::string> parts;
                std::string nameStr(name);
                Utils::trimSplit(parts,nameStr,':');
                if (parts.size()>2)
                {
                    auto cipher=parts[1];
                    auto mac=parts[2];

                    size_t tagSize=0;
                    if (parts.size()>3)
                    {
                        auto tagSizeStr=parts[3];
                        try
                        {
                            tagSize=std::stoi(tagSizeStr);
                        }
                        catch (...)
                        {
                            m_algMutex.unlock();
                            return cryptError(CryptError::INVALID_ALGORITHM);
                        }
                    }

                    m_algMutex.unlock();

                    algP=std::make_shared<EncryptMacAlg>(engine.get(),name,cipher,mac,tagSize);

                    m_algMutex.lock();
                    if (!algP->isValid())
                    {
                        algP.reset();
                    }
                }
            }
        }
        if (!algP)
        {
            m_algMutex.unlock();
            HATN_CHECK_RETURN(doFindAlgorithm(algP,type,name,engine.get()))
            m_algMutex.lock();
        }

        alg=algP.get();
        auto& algs=it->second;
        (*algs)[std::move(key)]=std::move(algP);

        m_algMutex.unlock();
    }

    return common::Error();
}

//---------------------------------------------------------------
common::Error CryptPlugin::randBytes(char *data, size_t size) const
{
    if (!m_randGen)
    {
        const_cast<CryptPlugin*>(this)->m_randGen=createRandomGenerator();
    }
    if (!m_randGen)
    {
        return cryptError(CryptError::GENERAL_FAIL);
    }
    return m_randGen->randBytes(data,size);
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
