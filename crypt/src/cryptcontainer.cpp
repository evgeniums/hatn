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

HATN_CRYPT_NAMESPACE_BEGIN

/*********************** CryptContainer **************************/

//---------------------------------------------------------------
CryptContainer::CryptContainer(
        const SymmetricKey* masterKey,
        const CipherSuite* suite,
        const common::pmr::AllocatorFactory* factory
    ) noexcept
        : m_masterKey(masterKey),
          m_encryptionKey(nullptr),
          m_aeadAlg(nullptr),
          m_streamAlg(nullptr),
          m_cipherSuite(suite),
          m_descriptor(factory),
          m_attachSuite(false),
          m_factory(factory),
          m_autoSalt(true),
          m_streamingMode(false),
          m_suites(&CipherSuitesGlobal::instance())
{
    if (suite!=nullptr)
    {
        m_suites=suite->suites();
    }
}

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
        if (isStreamingMode())
        {
            if (m_streamAlg==nullptr)
            {
                HATN_CHECK_RETURN(m_cipherSuite->cipherAlgorithm(m_streamAlg))
            }
            alg=m_streamAlg;
        }
        else
        {
            if (m_aeadAlg==nullptr)
            {
                HATN_CHECK_RETURN(m_cipherSuite->aeadAlgorithm(m_aeadAlg))
            }
            alg=m_aeadAlg;
        }
    }

    common::Error ec;
    key=m_masterKey;
    auto kdfType=m_descriptor.fieldValue(container_descriptor::kdf_type);
    if (m_encryptionKey!=nullptr || kdfType==container_descriptor::KdfType::HKDF)
    {
        // for HKDF mode default encryption key is a master key
        if (m_encryptionKey==nullptr)
        {
            m_encryptionKey=m_masterKey;
        }
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
        HATN_CHECK_RETURN(m_hkdf->init(m_encryptionKey,salt()))
        HATN_CHECK_RETURN(m_hkdf->derive(derivedKey,info))
        key=derivedKey.get();
    }
    else if (kdfType==container_descriptor::KdfType::PBKDF ||
             kdfType==container_descriptor::KdfType::PbkdfThenHkdf
            )
    {
        if (m_pbkdf.isNull())
        {
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

        if (kdfType==container_descriptor::KdfType::PbkdfThenHkdf)
        {
            // for PbkdfThenHkdf mode first derive m_encryptionKey and then go to HKDF
            HATN_CHECK_RETURN(m_pbkdf->derive(m_masterKey,m_encryptionKeyHolder,salt()))
            Assert(m_encryptionKeyHolder.get(),"Invalid derived PKDF key");
            m_encryptionKey=m_encryptionKeyHolder.get();
            return deriveKey(key,derivedKey,info,alg);
        }
        else
        {
            // for PBKDF mode always derive key from master key using PBKDF
            if (!info.isEmpty())
            {
                // append info to salt
                auto buf=salt();
                common::ByteArray sbuf(buf.data(),buf.size());
                sbuf.append(info);
                HATN_CHECK_RETURN(m_pbkdf->derive(m_masterKey,derivedKey,sbuf))
            }
            else
            {
                HATN_CHECK_RETURN(m_pbkdf->derive(m_masterKey,derivedKey,salt()))
            }
            key=derivedKey.get();
        }
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
    m_maxPackedChunkSize.reset();
    m_maxPackedFirstChunkSize.reset();
    m_encryptionKey=nullptr;
    m_encryptionKeyHolder.reset();
    m_streamKeyHolder.reset();
    m_aeadAlg=nullptr;
    m_streamAlg=nullptr;
    if (m_hkdf) m_hkdf->reset();
    if (m_enc) m_enc->reset();
    if (m_dec) m_dec->reset();
    m_streamEnc.reset();
    m_streamDec.reset();
    if (withDescriptor)
    {
        m_descriptor.reset();
    }
}

//---------------------------------------------------------------
void CryptContainer::hardReset(bool withDescriptor)
{
    m_maxPackedChunkSize.reset();
    m_maxPackedFirstChunkSize.reset();
    m_encryptionKey=nullptr;
    m_aeadAlg=nullptr;
    m_streamAlg=nullptr;
    m_encryptionKeyHolder.reset();
    m_streamKeyHolder.reset();
    m_pbkdf.reset();
    m_hkdf.reset();
    m_enc.reset();
    m_dec.reset();
    m_streamEnc.reset();
    m_streamDec.reset();
    if (withDescriptor)
    {
        m_descriptor.reset();
    }
}

//---------------------------------------------------------------

Result<size_t> CryptContainer::streamPrefixSize() const
{
    if (!m_streamingMode)
    {
        return cryptError(CryptError::INVALID_CRYPT_CONTAINER_STREAM_MODE);
    }

    if (m_streamEnc)
    {
        return m_streamEnc->streamPrefixSize();
    }

    Error ec;
    auto* self=const_cast<CryptContainer*>(this);
    self->m_streamEnc=m_cipherSuite->createSEncryptor(ec);
    HATN_CHECK_EC(ec)
    if (m_streamEnc.isNull())
    {
        return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
    }
    return m_streamEnc->streamPrefixSize();
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
