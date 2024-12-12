/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslhmac.cpp
 *
 * 	HMAC (Hash Message Authentication Code) implementation with OpenSSL backend
 *
 */
/****************************************************************************/

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslhmac.h>

#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* HMACAlg ********************/

//---------------------------------------------------------------
HMACAlg::HMACAlg(const CryptEngine *engine, const char *name)
    : CryptAlgorithm(engine,CryptAlgorithm::Type::HMAC,name)
{
    std::vector<std::string> parts;
    splitAlgName(name,parts);
    if (parts.size()<2)
    {
        throw ErrorException(cryptError(CryptError::INVALID_ALGORITHM));
    }

    const auto& nameStr=parts[0];
    if (!boost::iequals(nameStr,std::string("HMAC")))
    {
        throw ErrorException(cryptError(CryptError::INVALID_ALGORITHM));
    }

    const auto& digestName=parts[1];
    const auto* digest=::EVP_get_digestbyname(digestName.c_str());
    if (digest==nullptr)
    {
        throw ErrorException(cryptError(CryptError::INVALID_DIGEST));
    }

    setHandler(digest);
}

//---------------------------------------------------------------
common::SharedPtr<MACKey> HMACAlg::createMACKey() const
{
    auto key=makeShared<OpenSslHMACKey>();
    key->setAlg(this);
    return key;
}

/******************* OpenSslHMAC ********************/

//---------------------------------------------------------------
common::Error OpenSslHMAC::findNativeAlgorithm(std::shared_ptr<CryptAlgorithm> &alg, const char *name, CryptEngine *engine) noexcept
{
    try
    {
        alg=std::make_shared<HMACAlg>(engine,name);
    }
    catch (const ErrorException& ec)
    {
        return ec.error();
    }
    return Error();
}

//---------------------------------------------------------------
common::Error OpenSslHMAC::doProcess(const char *buf, size_t size) noexcept
{
    if (::HMAC_Update(nativeHandler().handler,reinterpret_cast<const unsigned char*>(buf),size) != 1)
    {
        return makeLastSslError(CryptError::MAC_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------
common::Error OpenSslHMAC::doFinalize(char *buf) noexcept
{
    if (::HMAC_Final(nativeHandler().handler,reinterpret_cast<unsigned char*>(buf),nullptr) != 1)
    {
        return makeLastSslError(CryptError::MAC_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------
common::Error OpenSslHMAC::doInit() noexcept
{
    {
        if (nativeHandler().isNull())
        {
            nativeHandler().handler = ::HMAC_CTX_new();
            if (nativeHandler().isNull())
            {
                return makeLastSslError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
            }
        }
        if (
                this->key()->isBackendKey()
                ||
                key()->content().isEmpty()
            )
        {
            return cryptError(CryptError::INVALID_MAC_KEY);
        }

        if (::HMAC_Init_ex(nativeHandler().handler,
                           key()->content().data(),
                           static_cast<int>(key()->content().size()),
                           alg()->nativeHandler<EVP_MD>(),
                           alg()->engine()->nativeHandler<ENGINE>()
                            ) != 1)
        {
            return makeLastSslError(CryptError::MAC_FAILED);
        }
        return Error();
    }
}

//---------------------------------------------------------------
void OpenSslHMAC::doReset() noexcept
{
    if (!nativeHandler().isNull())
    {
        ::HMAC_CTX_reset(nativeHandler().handler);
    }
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
