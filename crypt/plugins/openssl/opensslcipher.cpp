/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslcipher.cpp
 *
 * 	Implementation of symmetric cipher with OpenSSL EVP backend
 *
 */
/****************************************************************************/

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslsecretkey.h>
#include <hatn/crypt/plugins/openssl/opensslcipher.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslSymmetricEncryptor ********************/

//---------------------------------------------------------------
common::Error OpenSslSymmetricEncryptor::doProcess(
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
        if (::EVP_EncryptUpdate(nativeHandler().handler,
                          reinterpret_cast<unsigned char*>(bufOut),
                          &resultSize,
                          reinterpret_cast<const unsigned char*>(bufIn),
                          static_cast<int>(sizeIn)
                          ) != 1)
        {
            return makeLastSslError(CryptError::ENCRYPTION_FAILED);
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

/******************* OpenSslSymmetricDecryptor ********************/

//---------------------------------------------------------------
common::Error OpenSslSymmetricDecryptor::doProcess(
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
        if (::EVP_DecryptUpdate(nativeHandler().handler,
                          reinterpret_cast<unsigned char*>(bufOut),
                          &resultSize,
                          reinterpret_cast<const unsigned char*>(bufIn),
                          static_cast<int>(sizeIn)
                          ) != 1)
        {
            return makeLastSslError(CryptError::DECRYPTION_FAILED);
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

/******************* Cipher ********************/

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
    v->push_back(obj->name);
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslCipher::listCiphers()
{
    std::vector<std::string> result={"-","none"};
    ::OBJ_NAME_do_all_sorted(OBJ_NAME_TYPE_CIPHER_METH, eachObj, &result);
    return result;
}

//---------------------------------------------------------------
common::SharedPtr<SymmetricKey> CipherAlg::createSymmetricKey() const
{
    auto key=common::makeShared<OpenSslSymmetricKey>();
    key->setAlg(this);
    return key;
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
