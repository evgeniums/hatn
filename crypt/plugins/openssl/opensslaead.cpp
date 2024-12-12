/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslcipher.cpp
 *
 * 	Implementation of AEAD with OpenSSL EVP backend
 *
 */
/****************************************************************************/

#include <boost/algorithm/string/predicate.hpp>

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslsecretkey.h>
#include <hatn/crypt/plugins/openssl/opensslaead.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslAeadWorker ********************/

//---------------------------------------------------------------
template <bool Encrypt,typename DerivedT>
common::Error OpenSslAeadWorker<Encrypt,DerivedT>::doSetTag(const char* data) noexcept
{
    if (::EVP_CIPHER_CTX_ctrl(this->nativeHandler().handler,EVP_CTRL_AEAD_SET_TAG, static_cast<int>(this->getTagSize()), const_cast<char*>(data))!=1)
    {
        return makeLastSslError(CryptError::ENCRYPTION_FAILED);
    }
    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt,typename DerivedT>
common::Error OpenSslAeadWorker<Encrypt,DerivedT>::doGetTag(char* data) noexcept
{
    if (::EVP_CIPHER_CTX_ctrl(this->nativeHandler().handler,EVP_CTRL_AEAD_GET_TAG, static_cast<int>(this->getTagSize()), data)!=1)
    {
        return makeLastSslError(CryptError::ENCRYPTION_FAILED);
    }
    return common::Error();
}

#if 0
// seems like OpenSSL doesn't support well variable IV length
// there is interface but it doesn't work correctly with test vectors
// so stick to 12 bytes of IV length and use default initialization in symmetric cipher

//---------------------------------------------------------------
template <bool Encrypt,typename DerivedT>
common::Error OpenSslAeadWorker<Encrypt,DerivedT>::subInit(const char* iv)
{
    if (Encrypt)
    {
        if (::EVP_EncryptInit_ex(this->nativeHandler().handler,
                          this->cipher(),
                          this->key()->alg()->engine()->template nativeHandler<ENGINE>(),
                          NULL,
                          NULL
                          ) != 1)
        {
            return makeLastSslError(CryptError::ENCRYPTION_FAILED);
        }

        int ivLen=static_cast<int>(this->ivSize());
        if (::EVP_CIPHER_CTX_ctrl(this->nativeHandler().handler, EVP_CTRL_AEAD_SET_IVLEN, ivLen, NULL)!=1)
        {
            return makeLastSslError(CryptError::ENCRYPTION_FAILED);
        }

        if (::EVP_EncryptInit_ex(this->nativeHandler().handler,
                          this->cipher(),
                          NULL,
                          reinterpret_cast<const unsigned char*>(this->key()->content().data()),
                          reinterpret_cast<const unsigned char*>(iv)
                          ) != 1)
        {
            return makeLastSslError(CryptError::ENCRYPTION_FAILED);
        }
    }
    else
    {
        if (::EVP_DecryptInit_ex(this->nativeHandler().handler,
                          this->cipher(),
                          this->key()->alg()->engine()->template nativeHandler<ENGINE>(),
                          NULL,
                          NULL
                          ) != 1)
        {
            return makeLastSslError(CryptError::DECRYPTION_FAILED);
        }

        if (::EVP_CIPHER_CTX_ctrl(this->nativeHandler().handler, EVP_CTRL_AEAD_SET_IVLEN, static_cast<int>(this->ivSize()), NULL)!=1)
        {
            return makeLastSslError(CryptError::DECRYPTION_FAILED);
        }

        if (::EVP_DecryptInit_ex(this->nativeHandler().handler,
                          this->cipher(),
                          NULL,
                          reinterpret_cast<const unsigned char*>(this->key()->content().data()),
                          reinterpret_cast<const unsigned char*>(iv)
                          ) != 1)
        {
            return makeLastSslError(CryptError::DECRYPTION_FAILED);
        }
    }

    return Error();
}
#endif
/******************* OpenSslAeadEncryptor ********************/

//---------------------------------------------------------------
common::Error OpenSslAeadEncryptor::doProcess(
        const char* bufIn,
        size_t sizeIn,
        char* bufOut,
        size_t& sizeOut,
        bool lastBlock
    )
{
    int resultSize=0;
    if (!lastBlock)
    {
        unsigned char* toBuf=reinterpret_cast<unsigned char*>(bufOut);
        if (authNotCipher())
        {
            toBuf=NULL;
        }
        if (::EVP_EncryptUpdate(nativeHandler().handler,
                          toBuf,
                          &resultSize,
                          reinterpret_cast<const unsigned char*>(bufIn),
                          static_cast<int>(sizeIn)
                          ) != 1)
        {
            return makeLastSslError(CryptError::ENCRYPTION_FAILED);
        }
        if (authNotCipher())
        {
            resultSize=0;
        }
    }
    else
    {
        if (::EVP_EncryptFinal_ex(nativeHandler().handler,
                          reinterpret_cast<unsigned char*>(bufOut),
                          &resultSize
                          ) != 1)
        {
            return makeLastSslError(CryptError::ENCRYPTION_FAILED);
        }
    }
    sizeOut=static_cast<size_t>(resultSize);
    return Error();
}

/******************* OpenSslAeadDecryptor ********************/

//---------------------------------------------------------------
common::Error OpenSslAeadDecryptor::doProcess(
        const char* bufIn,
        size_t sizeIn,
        char* bufOut,
        size_t& sizeOut,
        bool lastBlock
    )
{
    int resultSize=0;
    if (!lastBlock)
    {
        unsigned char* toBuf=reinterpret_cast<unsigned char*>(bufOut);
        if (authNotCipher())
        {
            toBuf=NULL;
        }
        if (::EVP_DecryptUpdate(nativeHandler().handler,
                          toBuf,
                          &resultSize,
                          reinterpret_cast<const unsigned char*>(bufIn),
                          static_cast<int>(sizeIn)
                          ) != 1)
        {
            return makeLastSslError(CryptError::DECRYPTION_FAILED);
        }
        if (authNotCipher())
        {
            resultSize=0;
        }
    }
    else
    {
        if (::EVP_DecryptFinal_ex(nativeHandler().handler,
                          reinterpret_cast<unsigned char*>(bufOut),
                          &resultSize
                          ) != 1)
        {
            return makeLastSslError(CryptError::DECRYPTION_FAILED);
        }
    }
    sizeOut=static_cast<size_t>(resultSize);
    return Error();
}

/******************* OpenSslAEAD ********************/

namespace
{
    struct Ctx
    {
        explicit Ctx(std::vector<std::string>& v):v(v)
        {}
        std::vector<std::string>& v;
    };
}

static void eachObj(const OBJ_NAME *obj, void *arg)
{
    auto v=reinterpret_cast<std::vector<std::string>*>(arg);
    if (OpenSslAEAD::checkCipherName(obj->name))
    {
        v->push_back(obj->name);
    }
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslAEAD::listCiphers()
{
    std::vector<std::string> result;
    ::OBJ_NAME_do_all_sorted(OBJ_NAME_TYPE_CIPHER_METH, eachObj, &result);

    result.emplace_back("encryptmac:<cipher>:<mac>[:<tag_size>]");
    result.emplace_back("encryptmac:chacha20:poly1305");
    result.emplace_back("encryptmac:aes-256-cbc:hmac/sha512");
    result.emplace_back("encryptmac:aes-256-cbc:hmac/sha384");
    result.emplace_back("encryptmac:aes-192-cbc:hmac/sha384");
    result.emplace_back("encryptmac:aes-128-cbc:hmac/sha256");

    return result;
}

//---------------------------------------------------------------
AeadAlg::AeadAlg(const CryptEngine *engine, const char *name, const char *nativeName, size_t tagSize, bool padding) noexcept
    : CipherAlg(engine,name,nativeName,CryptAlgorithm::Type::AEAD,padding),
      m_tagSize(tagSize)
{
    if (!OpenSslAEAD::checkCipherName(name))
    {
        setHandler(nullptr);
    }
}

//---------------------------------------------------------------
inline bool OpenSslAEAD::checkCipherName(const std::string &name) noexcept
{
    if (
        // only AES GCM
        (
                boost::algorithm::icontains(name,"aes")
                &&
                boost::algorithm::icontains(name,"gcm")
         )
         || // or chacha20_poly1305 are supported
        (
            boost::algorithm::icontains(name,"chacha20")
            &&
            boost::algorithm::icontains(name,"poly1305")
        )
       )
    {
        return true;
    }
    return false;
}

//---------------------------------------------------------------
common::Error OpenSslAEAD::findNativeAlgorithm(std::shared_ptr<CryptAlgorithm> &alg, const char *name, CryptEngine *engine) noexcept
{
    std::vector<std::string> parts;
    splitAlgName(name,parts);
    if (parts.size()<1)
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }
    auto algName=parts[0];

    size_t tagSize=0;
    if (parts.size()>1)
    {
        auto tagSizeStr=parts[1];
        try
        {
            tagSize=std::stoi(tagSizeStr);
        }
        catch (...)
        {
            return cryptError(CryptError::INVALID_ALGORITHM);
        }
    }

    bool padding=true;
    if (parts.size()>2)
    {
        auto paddingStr=parts[2];
        if (boost::iequals(paddingStr,std::string("no-padding")))
        {
            padding=false;
        }
    }

    alg=std::make_shared<AeadAlg>(engine,name,algName.c_str(),tagSize,padding);
    return common::Error();
}

//---------------------------------------------------------------

#ifndef _WIN32
template class OpenSslAeadWorker<true,OpenSslAeadEncryptor>;
template class OpenSslAeadWorker<false,OpenSslAeadDecryptor>;
#endif

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
