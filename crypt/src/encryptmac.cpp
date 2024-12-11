/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/encryptmac.cpp
  *
  *   Implements AEAD using Encrypt-Then-MAC
  *
  */

/****************************************************************************/

#include <boost/endian/conversion.hpp>

#include <hatn/common/makeshared.h>

#include <hatn/crypt/encryptmac.h>
#include <hatn/crypt/aeadworker.ipp>

namespace hatn {

using namespace common;

namespace crypt {

/*********************** EncryptMacAlg **************************/

namespace
{
struct EncryptMacAlgMethod
{
    static EncryptMacAlgMethod& stub()
    {
        static EncryptMacAlgMethod inst;
        return inst;
    }
};
}

//---------------------------------------------------------------
EncryptMacAlg::EncryptMacAlg(
        const CryptEngine *engine,
        const char *name,
        const std::string& cipherName,
        const std::string& macName,
        size_t tagSize
    ) : CryptAlgorithm(engine,CryptAlgorithm::Type::AEAD,name),
        m_tagSize(tagSize)
{
    auto ec1=engine->plugin()->findAlgorithm(m_cipherAlg,CryptAlgorithm::Type::SENCRYPTION,cipherName,engine->name());
    auto ec2=engine->plugin()->findAlgorithm(m_macAlg,CryptAlgorithm::Type::MAC,macName,engine->name());
    if (!ec1 && !ec2 && m_cipherAlg && m_macAlg)
    {
        setHandler(&EncryptMacAlgMethod::stub());
    }
}

//---------------------------------------------------------------
common::SharedPtr<SymmetricKey> EncryptMacAlg::createSymmetricKey() const
{
    auto&& cipherKey=m_cipherAlg->createSymmetricKey();
    auto&& macKey=m_macAlg->createMACKey();
    if (cipherKey && macKey)
    {
        auto key=common::makeShared<EncryptMacKey>(std::move(cipherKey),std::move(macKey));
        key->setAlg(this);
        return key;
    }
    return common::SharedPtr<SymmetricKey>();
}

/*********************** EncryptMacKey ***********************/

//---------------------------------------------------------------
EncryptMacKey::EncryptMacKey(
        common::SharedPtr<SymmetricKey> cipherKey,
        common::SharedPtr<MACKey> macKey
    ) noexcept : m_cipherKey(std::move(cipherKey)),
        m_macKey(std::move(macKey))
{}

//---------------------------------------------------------------
common::Error EncryptMacKey::importFromKDF(const char *buf, size_t size)
{
    Assert(m_cipherKey&&m_cipherKey->alg(),"Cipher algorithm is not set");

    size_t cipkerKeySize=m_cipherKey->alg()->keySize();
    if (size<cipkerKeySize)
    {
        return common::Error(CommonError::INVALID_SIZE);
    }

    HATN_CHECK_RETURN(m_cipherKey->importFromBuf(buf,cipkerKeySize,ContainerFormat::RAW_PLAIN))
    return m_macKey->importFromBuf(buf,size-cipkerKeySize,ContainerFormat::RAW_PLAIN);
}

//---------------------------------------------------------------
common::Error EncryptMacKey::doImportFromBuf(const char *buf, size_t size, ContainerFormat format, bool keepContent)
{
    m_cipherKey->setProtector(protector());
    m_macKey->setProtector(protector());

    if (size==0)
    {
        // pass empty buffer to embedded keys
        HATN_CHECK_RETURN(m_cipherKey->importFromBuf(buf,size,format,keepContent))
        return m_macKey->importFromBuf(buf,size,format,keepContent);
    }

    // first 2 bytes are for cipher key size
    uint16_t cipherKeySize=0;
    if (size<sizeof(cipherKeySize))
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }
    memcpy(&cipherKeySize,buf,sizeof(cipherKeySize));
    boost::endian::little_to_native_inplace(cipherKeySize);
    if (size<(cipherKeySize+sizeof(cipherKeySize)))
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    HATN_CHECK_RETURN(m_cipherKey->importFromBuf(buf+sizeof(cipherKeySize),cipherKeySize,format,keepContent))
    return m_macKey->importFromBuf(buf+sizeof(cipherKeySize)+cipherKeySize,size-sizeof(cipherKeySize)-cipherKeySize,format,keepContent);
}

//---------------------------------------------------------------
common::Error EncryptMacKey::doExportToBuf(MemoryLockedArray &buf, ContainerFormat format, bool unprotected) const
{
    buf.clear();

    m_cipherKey->setProtector(protector());
    m_macKey->setProtector(protector());

    MemoryLockedArray tmpBuf1,tmpBuf2;
    HATN_CHECK_RETURN(m_cipherKey->exportToBuf(tmpBuf1,format,unprotected))
    HATN_CHECK_RETURN(m_macKey->exportToBuf(tmpBuf2,format,unprotected))
    if (tmpBuf1.isEmpty()&&tmpBuf2.isEmpty())
    {
        return common::Error();
    }
    uint16_t cipherKeySize=static_cast<uint16_t>(tmpBuf1.size());
    buf.resize(sizeof(cipherKeySize)+tmpBuf1.size()+tmpBuf2.size());

    boost::endian::native_to_little_inplace(cipherKeySize);
    memcpy(buf.data(),&cipherKeySize,sizeof(cipherKeySize));
    if (!tmpBuf1.isEmpty())
    {
        std::copy(tmpBuf1.data(),tmpBuf1.data()+tmpBuf1.size(),buf.data()+sizeof(cipherKeySize));
    }
    if (!tmpBuf2.isEmpty())
    {
        std::copy(tmpBuf2.data(),tmpBuf2.data()+tmpBuf2.size(),buf.data()+sizeof(cipherKeySize)+tmpBuf1.size());
    }
    return common::Error();
}

//---------------------------------------------------------------
common::Error EncryptMacKey::doGenerate()
{
    HATN_CHECK_RETURN(m_cipherKey->generate())
    return m_macKey->generate();
}

/*********************** EncryptMAC **************************/

//---------------------------------------------------------------
template <bool Encrypt>
void EncryptMAC<Encrypt>::doReset() noexcept
{
    if (!m_cipher.isNull())
    {
        m_cipher->reset();
    }
    if (!m_mac.isNull())
    {
        m_mac->reset();
    }
    m_tag.clear();
    m_authMode=false;
}

namespace
{
template <bool Encrypt>
struct CipherTraits
{
};
template <>
struct CipherTraits<true>
{
    static common::SharedPtr<CipherWorker<true>> create(CryptPlugin* plugin,SymmetricKey* key)
    {
        return plugin->createSEncryptor(key);
    }

    static common::Error process(CipherWorker<true>* cipher,MAC* mac,
                                 const common::ConstDataBuf& dataIn,common::DataBuf& dataOut,
                                 bool authMode,common::ByteArray& tag,bool lastBlock,size_t& sizeOut)
    {
        if (!authMode)
        {
            HATN_CHECK_RETURN(cipher->process(dataIn,dataOut,sizeOut,0,0,0,lastBlock,true));
            common::ConstDataBuf dataMac(dataOut.data(),sizeOut);
            HATN_CHECK_RETURN(mac->process(dataMac));
        }
        else
        {
            HATN_CHECK_RETURN(mac->process(dataIn));
        }
        if (lastBlock)
        {
            HATN_CHECK_RETURN(mac->finalize(tag));
        }
        return common::Error();
    }
};
template <>
struct CipherTraits<false>
{
    static common::SharedPtr<CipherWorker<false>> create(CryptPlugin* plugin,SymmetricKey* key)
    {
        return plugin->createSDecryptor(key);
    }

    static common::Error process(CipherWorker<false>* cipher,MAC* mac,
                                 const common::ConstDataBuf& dataIn,common::DataBuf& dataOut,
                                 bool authMode,common::ByteArray& tag,bool lastBlock,size_t& sizeOut)
    {
        HATN_CHECK_RETURN(mac->process(dataIn));
        if (!authMode)
        {
            HATN_CHECK_RETURN(cipher->process(dataIn,dataOut,sizeOut,0,0,0,lastBlock,true));
        }
        if (lastBlock)
        {
            HATN_CHECK_RETURN(mac->finalizeAndVerify(tag));
        }
        return common::Error();
    }
};
}

//---------------------------------------------------------------
template <bool Encrypt>
void EncryptMAC<Encrypt>::updateKey()
{
    if (this->key()==nullptr)
    {
        return;
    }

    const EncryptMacKey* actualKey=dynamic_cast<const EncryptMacKey*>(this->key());
    if (actualKey==nullptr
       ||
        actualKey->cipherKey()==nullptr
       ||
        actualKey->cipherKey()->alg()==nullptr
       ||
         actualKey->cipherKey()->alg()->engine()==nullptr
       ||
        actualKey->macKey()==nullptr
       ||
        actualKey->macKey()->alg()==nullptr
       ||
        actualKey->macKey()->alg()->engine()==nullptr
       )
    {
        throw common::ErrorException(makeCryptError(CryptErrorCode::INVALID_KEY_TYPE));
    }

    if (m_cipher.isNull())
    {
        m_cipher=CipherTraits<Encrypt>::create(actualKey->cipherKey()->alg()->engine()->plugin(),actualKey->cipherKey());
        if (m_cipher.isNull())
        {
            throw common::ErrorException(makeCryptError(CryptErrorCode::NOT_SUPPORTED_BY_PLUGIN));
        }
    }
    else
    {
        m_cipher->setKey(actualKey->cipherKey());
    }

    if (m_mac.isNull())
    {
        m_mac=actualKey->macKey()->alg()->engine()->plugin()->createMAC(actualKey->macKey());
        if (m_mac.isNull())
        {
            throw common::ErrorException(makeCryptError(CryptErrorCode::NOT_SUPPORTED_BY_PLUGIN));
        }
    }
    else
    {
        m_mac->setKey(actualKey->macKey());
    }
}

//---------------------------------------------------------------
template <bool Encrypt>
common::Error EncryptMAC<Encrypt>::doGenerateIV(char* iv) const
{
    if (m_cipher.isNull())
    {
        return makeCryptError(CryptErrorCode::INVALID_CIPHER_STATE);
    }
    common::DataBuf data(iv,ivSize());
    return m_cipher->generateIV(data);
}

//---------------------------------------------------------------
template <bool Encrypt>
size_t EncryptMAC<Encrypt>::blockSize() const noexcept
{
    if (!m_cipher.isNull())
    {
        return m_cipher->blockSize();
    }
    return 16;
}

//---------------------------------------------------------------
template <bool Encrypt>
size_t EncryptMAC<Encrypt>::ivSize() const noexcept
{
    if (!m_cipher.isNull())
    {
        return m_cipher->ivSize();
    }
    return 16;
}

//---------------------------------------------------------------
template <bool Encrypt>
common::Error EncryptMAC<Encrypt>::doInit(const char* iv)
{
    if (m_cipher.isNull() || m_mac.isNull())
    {
        return makeCryptError(CryptErrorCode::INVALID_CIPHER_STATE);
    }
    common::ConstDataBuf data(iv,ivSize());
    HATN_CHECK_RETURN(m_cipher->init(data));
    HATN_CHECK_RETURN(m_mac->init());
    return m_mac->process(data);
}

//---------------------------------------------------------------
template <bool Encrypt>
common::Error EncryptMAC<Encrypt>::doSetTag(const char *data) noexcept
{
    m_tag.load(data,this->getTagSize());
    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt>
common::Error EncryptMAC<Encrypt>::doGetTag(char* data) noexcept
{
    if (m_tag.isEmpty())
    {
        return makeCryptError(CryptErrorCode::INVALID_MAC_STATE);
    }
    size_t size=(std::min)(m_tag.size(),this->getTagSize());
    std::copy(m_tag.data(),m_tag.data()+size,data);
    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt>
common::Error EncryptMAC<Encrypt>::doProcess(
        const char* bufIn,
        size_t sizeIn,
        char* bufOut,
        size_t& sizeOut,
        bool lastBlock
    )
{
    sizeOut=0;
    common::ConstDataBuf dataIn(bufIn,sizeIn);
    common::DataBuf dataOut(bufOut,sizeIn);
    return CipherTraits<Encrypt>::process(m_cipher.get(),m_mac.get(),dataIn,dataOut,m_authMode,m_tag,lastBlock,sizeOut);
}

//---------------------------------------------------------------

template class HATN_CRYPT_EXPORT EncryptMAC<true>;
template class HATN_CRYPT_EXPORT EncryptMAC<false>;

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
