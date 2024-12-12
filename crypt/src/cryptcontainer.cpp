/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/cryptcontainer.cpp
  *
  *   Encrypted container
  *
  */

/****************************************************************************/

#include <hatn/crypt/cryptcontainer.h>

namespace hatn {

using namespace common;

namespace crypt {

/*********************** CryptContainer **************************/

//---------------------------------------------------------------
CryptContainer::CryptContainer(
        const SymmetricKey* masterKey,
        const CipherSuite* suite,
        common::pmr::AllocatorFactory* factory
    ) noexcept
        : m_masterKey(masterKey),
          m_cipherSuite(suite),
          m_descriptor(factory),
          m_attachSuite(false),
          m_factory(factory)
{}

//---------------------------------------------------------------
common::Error CryptContainer::deriveKey(
        SymmetricKeyConstPtr& key,
        common::SharedPtr<SymmetricKey>& derivedKey,
        const common::ConstDataBuf& info,
        const CryptAlgorithm* alg
    )
{
    if (alg==nullptr)
    {
        HATN_CHECK_RETURN(m_cipherSuite->aeadAlgorithm(alg))
    }

    common::Error ec;
    key=m_masterKey;
    auto kdfType=m_descriptor.fieldValue(container_descriptor::kdf_type);
    if (kdfType==container_descriptor::KdfType::HKDF)
    {
        if (m_hkdf.isNull())
        {
            m_hkdf=m_cipherSuite->createHKDF(ec,alg);
            HATN_CHECK_EC(ec)
            if (m_hkdf.isNull())
            {
                return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
            }
        }
        else
        {
            m_hkdf->setTargetKeyAlg(alg);
        }
        HATN_CHECK_RETURN(m_hkdf->init(m_masterKey,salt()))
        HATN_CHECK_RETURN(m_hkdf->derive(derivedKey,info))
        key=derivedKey.get();
    }
    else if (kdfType==container_descriptor::KdfType::PBKDF)
    {
        if (m_pbkdf.isNull())
        {
            HATN_CHECK_EC(ec)
            m_pbkdf=m_cipherSuite->createPBKDF(ec,alg);
            if (m_pbkdf.isNull())
            {
                return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
            }
        }
        else
        {
            m_pbkdf->setTargetKeyAlg(alg);
        }

        HATN_CHECK_RETURN(m_pbkdf->derive(m_masterKey,derivedKey,salt()))
        key=derivedKey.get();
    }
    else
    {
        return cryptError(CryptError::INVALID_KDF_TYPE);
    }
    return common::Error();
}

//---------------------------------------------------------------
void CryptContainer::reset(bool withDescriptor)
{
    if (m_hkdf) m_hkdf->reset();
    if (m_enc) m_enc->reset();
    if (m_dec) m_dec->reset();
    if (withDescriptor)
    {
        m_descriptor.reset();
    }
}

//---------------------------------------------------------------
void CryptContainer::hardReset(bool withDescriptor)
{
    m_pbkdf.reset();
    m_hkdf.reset();
    m_enc.reset();
    m_dec.reset();
    if (withDescriptor)
    {
        m_descriptor.reset();
    }
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
