/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslplugin.cpp
  *
  *   Encryption plugin that uses OpenSSL as backend
  *
  */

/****************************************************************************/

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/engine.h>

#include <hatn/common/makeshared.h>

#include <hatn/crypt/plugins/openssl/opensslx509.h>
#include <hatn/crypt/plugins/openssl/openssldigest.h>
#include <hatn/crypt/plugins/openssl/opensslcontext.h>
#include <hatn/crypt/plugins/openssl/opensslmackey.h>
#include <hatn/crypt/plugins/openssl/opensslsecretkey.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslsymmetric.h>
#include <hatn/crypt/plugins/openssl/opensslpbkdf.h>
#include <hatn/crypt/plugins/openssl/opensslhkdf.h>
#include <hatn/crypt/plugins/openssl/opensslsignature.h>
#include <hatn/crypt/plugins/openssl/opensslasymmetric.h>
#include <hatn/crypt/plugins/openssl/opensslrandomgenerator.h>
#include <hatn/crypt/plugins/openssl/opensslpasswordgenerator.h>
#include <hatn/crypt/plugins/openssl/opensslecdh.h>
#include <hatn/crypt/plugins/openssl/opensslaead.h>
#include <hatn/crypt/plugins/openssl/opensslmac.h>
#include <hatn/crypt/plugins/openssl/openssled.h>
#include <hatn/crypt/plugins/openssl/opensslx509.h>
#include <hatn/crypt/plugins/openssl/opensslx509chain.h>
#include <hatn/crypt/plugins/openssl/opensslx509certificatestore.h>
#include <hatn/crypt/plugins/openssl/opensslhmac.h>
#include <hatn/crypt/plugins/openssl/openssldh.h>

#include <hatn/crypt/plugins/openssl/opensslplugin.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/*********************** OpenSslPlugin **************************/

//---------------------------------------------------------------
common::SharedPtr<SecureStreamContext> OpenSslPlugin::createSecureStreamContext(
        SecureStreamContext::EndpointType endpointType,
        SecureStreamContext::VerifyMode verifyMode
    ) const
{
    return common::makeShared<OpenSslContext>(endpointType,verifyMode);
}

//---------------------------------------------------------------
common::SharedPtr<Digest> OpenSslPlugin::createDigest(const CryptAlgorithm* alg) const
{
    auto obj=common::makeShared<OpenSslDigest>();
    obj->setAlgorithm(alg);
    return obj;
}

//---------------------------------------------------------------
common::SharedPtr<MAC> OpenSslPlugin::createMAC(const SymmetricKey* key) const
{
    return common::makeShared<OpenSslMAC>(key);
}

//---------------------------------------------------------------
common::SharedPtr<HMAC> OpenSslPlugin::createHMAC(const CryptAlgorithm* alg) const
{
    common::SharedPtr<OpenSslHMAC> obj(new OpenSslHMAC());
    obj->setAlgorithm(alg);
    return obj;
}

//---------------------------------------------------------------
Error OpenSslPlugin::doFindEngine(
        const char *engineName,
        std::shared_ptr<CryptEngine> &engine
    )
{
#if OPENSSL_API_LEVEL >= 30100

    //! @todo Implement explicit loading of OpenSSL providers

    std::ignore=engine;
    std::ignore=engineName;
    engine=std::make_shared<CryptEngine>(this,nullptr);

#else

    auto nativeEngine=ENGINE_by_id(engineName);
    if (nativeEngine)
    {
        return cryptError(CryptError::INVALID_ENGINE);
    }
    auto freeFn=[nativeEngine]()
    {
        ::ENGINE_free(nativeEngine);
    };
    engine=std::make_shared<CryptEngine>(this,nativeEngine);
    engine->setFreeNativeFn(freeFn);

#endif
    return common::Error();
}

//---------------------------------------------------------------
Error OpenSslPlugin::doFindAlgorithm(
        std::shared_ptr<CryptAlgorithm> &alg,
        CryptAlgorithm::Type type,
        const char *name,
        CryptEngine* engine
    )
{
    Error ec;
    switch (type)
    {
        case (CryptAlgorithm::Type::AEAD):
        {
            ec=OpenSslAEAD::findNativeAlgorithm(alg,name,engine);
        }
        break;

        case (CryptAlgorithm::Type::SENCRYPTION):
        {
            ec=OpenSslSymmetric::findNativeAlgorithm(alg,name,engine);
        }
        break;

        case (CryptAlgorithm::Type::DIGEST):
        {
            ec=OpenSslDigest::findNativeAlgorithm(alg,name,engine);
        }
        break;

        case (CryptAlgorithm::Type::PBKDF):
        {
            ec=OpenSslPBKDF::findNativeAlgorithm(alg,name,engine);
        }
        break;

        case (CryptAlgorithm::Type::HMAC):
        {
            ec=OpenSslHMAC::findNativeAlgorithm(alg,name,engine);
        }
        break;

        case (CryptAlgorithm::Type::DH):
        {
            ec=OpenSslDH::findNativeAlgorithm(alg,name,engine);
        }
        break;

        case (CryptAlgorithm::Type::SIGNATURE): HATN_FALLTHROUGH
        case (CryptAlgorithm::Type::AENCRYPTION):
        {
            ec=OpenSslAsymmetric::findNativeAlgorithm(type,alg,name,engine);
        }
        break;

        case (CryptAlgorithm::Type::ECDH):
        {
            ec=OpenSslECDH::findNativeAlgorithm(alg,name,engine);
        }
        break;

        case (CryptAlgorithm::Type::MAC):
        {
            ec=OpenSslMAC::findNativeAlgorithm(alg,name,engine);
        }
        break;

        default:
        {
            ec=cryptError(CryptError::INVALID_ALGORITHM);
        }
            break;
    }

    if (!ec)
    {
        if (!alg || !alg->isValid())
        {
            alg.reset();
            ec=cryptError(CryptError::INVALID_ALGORITHM);
        }
    }
    return ec;
}

//---------------------------------------------------------------
Error OpenSslPlugin::init(const CryptPluginConfig *cfg)
{
    std::ignore=cfg;
    if (OPENSSL_init_crypto(
                OPENSSL_INIT_ADD_ALL_DIGESTS
                | OPENSSL_INIT_ADD_ALL_CIPHERS
                | OPENSSL_INIT_LOAD_CRYPTO_STRINGS
                | OPENSSL_INIT_NO_LOAD_CONFIG
                , NULL)!=1)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslPlugin::doCleanup(std::map<CryptAlgorithm::Type, std::shared_ptr<AlgTable>> &algs)
{
    std::ignore=algs;
    OPENSSL_cleanup();
    return Error();
}

//---------------------------------------------------------------
common::SharedPtr<SEncryptor> OpenSslPlugin::createSEncryptor(const SymmetricKey* key) const
{
    return common::makeShared<OpenSslSymmetricEncryptor>(key);
}

//---------------------------------------------------------------
common::SharedPtr<SDecryptor> OpenSslPlugin::createSDecryptor(const SymmetricKey* key) const
{
    return common::makeShared<OpenSslSymmetricDecryptor>(key);
}

//---------------------------------------------------------------
common::SharedPtr<AEncryptor> OpenSslPlugin::createAEncryptor() const
{
    return common::makeShared<OpenSslAencryptor>();
}

//---------------------------------------------------------------
common::SharedPtr<ADecryptor> OpenSslPlugin::createADecryptor(const PrivateKey* key) const
{
    return common::makeShared<OpenSslAdecryptor>(key);
}

//---------------------------------------------------------------
common::SharedPtr<AEADEncryptor> OpenSslPlugin::createAeadEncryptor(const SymmetricKey* key) const
{
    return common::makeShared<OpenSslAeadEncryptor>(key);
}

//---------------------------------------------------------------
common::SharedPtr<AEADDecryptor> OpenSslPlugin::createAeadDecryptor(const SymmetricKey* key) const
{
    return common::makeShared<OpenSslAeadDecryptor>(key);
}

//---------------------------------------------------------------
common::SharedPtr<PassphraseKey> OpenSslPlugin::createPassphraseKey(const CryptAlgorithm *cipherAlg, const CryptAlgorithm *kdfAlg) const
{
    auto key=common::makeShared<OpenSslPassphraseKey>();
    key->setAlg(cipherAlg);
    if (kdfAlg==nullptr)
    {
        auto ec=const_cast<OpenSslPlugin*>(this)->findAlgorithm(kdfAlg,CryptAlgorithm::Type::DIGEST,"sha1");
        if (ec)
        {
            return common::SharedPtr<PassphraseKey>();
        }
    }
    key->setKdfAlg(kdfAlg);
    return key;
}

//---------------------------------------------------------------
common::SharedPtr<PublicKey> OpenSslPlugin::createPublicKey(const CryptAlgorithm *algorithm) const
{
    auto key=common::makeShared<OpenSslPublicKey>();
    key->setAlg(algorithm);
    return key;
}

//---------------------------------------------------------------
common::SharedPtr<PBKDF> OpenSslPlugin::createPBKDF(const CryptAlgorithm *cipherAlg, const CryptAlgorithm *kdfAlg) const
{
    return common::makeShared<OpenSslPBKDF>(cipherAlg,kdfAlg);
}

//---------------------------------------------------------------
common::SharedPtr<HKDF> OpenSslPlugin::createHKDF(const CryptAlgorithm *cipherAlg, const CryptAlgorithm *hashAlg) const
{
    return common::makeShared<OpenSslHKDF>(cipherAlg,hashAlg);
}

//---------------------------------------------------------------
common::SharedPtr<DH> OpenSslPlugin::createDH(const CryptAlgorithm* alg) const
{
    auto dh=common::makeShared<OpenSslDH>();
    dh->setAlg(alg);
    return dh;
}

//---------------------------------------------------------------
common::SharedPtr<ECDH> OpenSslPlugin::createECDH(const CryptAlgorithm *alg) const
{
    return common::makeShared<OpenSslECDH>(alg);
}

//---------------------------------------------------------------
int OpenSslPlugin::constTimeMemCmp(const void *a, const void *b, size_t len) const
{
   return CRYPTO_memcmp(a,b,len);
}

//---------------------------------------------------------------
common::SharedPtr<RandomGenerator> OpenSslPlugin::createRandomGenerator() const
{
    return common::makeShared<OpenSslRandomGenerator>();
}

//---------------------------------------------------------------
common::SharedPtr<PasswordGenerator> OpenSslPlugin::createPasswordGenerator() const
{
    return common::makeShared<OpenSslPasswordGenerator>();
}

//---------------------------------------------------------------
common::SharedPtr<X509Certificate> OpenSslPlugin::createX509Certificate() const
{
    return common::makeShared<OpenSslX509>();
}

//---------------------------------------------------------------
common::SharedPtr<X509CertificateStore> OpenSslPlugin::createX509CertificateStore() const
{
    return common::makeShared<OpenSslX509CertificateStore>();
}

//---------------------------------------------------------------
common::SharedPtr<X509CertificateChain> OpenSslPlugin::createX509CertificateChain() const
{
    return common::makeShared<OpenSslX509Chain>();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPlugin::listDigests() const
{
    return OpenSslDigest::listDigests();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPlugin::listCiphers() const
{
    return OpenSslSymmetric::listCiphers();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPlugin::listEllipticCurves() const
{
    return ECAlg::listCurves();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPlugin::listAEADs() const
{
    return OpenSslAEAD::listCiphers();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPlugin::listMACs() const
{
    return OpenSslMAC::listMACs();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPlugin::listPBKDFs() const
{
    return OpenSslPBKDF::listAlgs();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPlugin::listSignatures() const
{
    return OpenSslAsymmetric::listSignatures();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPlugin::listAsymmetricCiphers() const
{
    return OpenSslAsymmetric::listAsymmetricCiphers();
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslPlugin::listDHs() const
{
    return OpenSslDH::listDHs();
}

//---------------------------------------------------------------
common::Error OpenSslPlugin::createAsymmetricPrivateKeyFromContent(
        common::SharedPtr<PrivateKey> &pkey,
        const char *buf,
        size_t size,
        ContainerFormat format,
        KeyProtector* protector,
        const char *engineName
    )
{
    return OpenSslAsymmetric::createPrivateKeyFromContent(pkey,buf,size,format,this,protector,engineName);
}

//---------------------------------------------------------------
common::Error OpenSslPlugin::publicKeyAlgorithm(
        CryptAlgorithmConstP &alg,
        const PublicKey *key,
        const char *engineName
    )
{
    return OpenSslAsymmetric::publicKeyAlgorithm(alg,key,this,engineName);
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END

#ifndef NO_DYNAMIC_HATN_PLUGINS
HATN_PLUGIN_EXPORT(hatn::crypt::openssl::OpenSslPlugin)
#endif
