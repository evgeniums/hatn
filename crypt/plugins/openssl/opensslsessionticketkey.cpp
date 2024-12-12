/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file crypt/plugins/openssl/opensslsessionticketkey.cpp
  *
  *   Key to encrypt session tickets according to RFC5077 with OpenSSL backend
  *
  */

/****************************************************************************/

#include <hatn/crypt/keyprotector.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslsessionticketkey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/*********************** OpenSslSessionTicketKey **************************/

//---------------------------------------------------------------
Error OpenSslSessionTicketKey::load(const char *data, size_t size, KeyProtector* protector)
{
    common::MemoryLockedArray buf;
    if (protector!=nullptr)
    {
        common::ConstDataBuf dataIn(data,size);
        HATN_CHECK_RETURN(protector->unpack(dataIn,buf))
        data=buf.data();
        size=buf.size();
    }

    size_t requiredSize=mutableName().capacity()+CIPHER_NAME_MAXLEN+HMAC_NAME_MAXLEN+HMAC_KEY_LEN;
    if (size<requiredSize)
    {
        return Error(CommonError::INVALID_SIZE);
    }

    mutableName().load(data,mutableName().capacity());
    size_t offset=mutableName().capacity();

    common::FixedByteArrayThrow32 cipherName;
    cipherName.load(data+offset,CIPHER_NAME_MAXLEN);
    offset+=CIPHER_NAME_MAXLEN;

    common::FixedByteArrayThrow32 hmacName;
    hmacName.load(data+offset,HMAC_KEY_LEN);
    offset+=HMAC_NAME_MAXLEN;

    m_hmac=::EVP_get_digestbyname(hmacName.c_str());
    if (m_hmac==NULL)
    {
        doReset();
        return makeLastSslError(CryptError::INVALID_DIGEST);
    }
    m_macKey.load(data+offset,HMAC_KEY_LEN);
    offset+=HMAC_KEY_LEN;

    m_cipher=::EVP_get_cipherbyname(cipherName.c_str());
    if (m_cipher==NULL)
    {
        doReset();
        return makeLastSslError(CryptError::INVALID_CIPHER);
    }
    int cipherKeyLength=::EVP_CIPHER_key_length(m_cipher);
    requiredSize+=cipherKeyLength;
    if (size<requiredSize)
    {
        doReset();
        return Error(CommonError::INVALID_SIZE);
    }
    m_cipherKey.load(data+offset,cipherKeyLength);
    offset+=cipherKeyLength;

    std::ignore=offset;
    setValid();
    return Error();
}

//---------------------------------------------------------------
Error OpenSslSessionTicketKey::store(common::MemoryLockedArray &content, KeyProtector* protector) const
{
    //! @todo Is cipher name size always limited to 32?

    content.clear();
    if (!isValid())
    {
        // empty result is ok
        return Error();
    }

    size_t requiredSize=name().capacity()+CIPHER_NAME_MAXLEN+HMAC_NAME_MAXLEN+HMAC_KEY_LEN;
    int cipherKeyLength=::EVP_CIPHER_key_length(m_cipher);
    requiredSize+=cipherKeyLength;

    content.resize(requiredSize);
    content.fill('\0');
    size_t offset=0;

    std::copy(name().data(),name().data()+name().size(),content.data()+offset);
    offset+=name().capacity();

    common::FixedByteArrayThrow32 cipherName(::EVP_CIPHER_name(m_cipher));
    std::copy(cipherName.data(),cipherName.data()+cipherName.size(),content.data()+offset);
    offset+=cipherName.size();

    common::FixedByteArrayThrow32 hmacName(::EVP_MD_name(m_hmac));
    std::copy(hmacName.data(),hmacName.data()+cipherName.size(),content.data()+offset);
    offset+=hmacName.size();

    std::copy(m_macKey.data(),m_macKey.data()+HMAC_KEY_LEN,content.data()+offset);
    offset+=HMAC_KEY_LEN;

    std::copy(m_cipherKey.data(),m_cipherKey.data()+m_cipherKey.size(),content.data()+offset);
    offset+=m_cipherKey.size();

    if (protector!=nullptr)
    {
        common::MemoryLockedArray tmp;
        auto ec=protector->pack(content,tmp);
        if (ec)
        {
            content.clear();
            return ec;
        }
        content=std::move(tmp);
    }

    std::ignore=offset;
    return Error();
}

//---------------------------------------------------------------
Error OpenSslSessionTicketKey::generate(const common::FixedByteArray16 &name, const char *cipherName, const char *hmacName)
{
    mutableName()=name;

    m_hmac=::EVP_get_digestbyname(hmacName);
    if (m_hmac==NULL)
    {
        doReset();
        return cryptError(CryptError::INVALID_DIGEST);
    }

    m_cipher=::EVP_get_cipherbyname(cipherName);
    if (m_cipher==NULL)
    {
        doReset();
        return cryptError(CryptError::INVALID_CIPHER);
    }

    auto ec=genRandData(m_macKey,HMAC_KEY_LEN);
    if (ec)
    {
        doReset();
        return ec;
    }

    ec=genRandData(m_cipherKey,::EVP_CIPHER_key_length(m_cipher));
    if (ec)
    {
        doReset();
        return ec;
    }

    setValid();
    return Error();
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
