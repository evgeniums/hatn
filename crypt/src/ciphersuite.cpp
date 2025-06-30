/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/ciphersuite.cpp
  *
  *   Cipher suite
  *
  */

/****************************************************************************/

#include <boost/algorithm/string.hpp>

#include <hatn/common/makeshared.h>

#include <hatn/dataunit/visitors.h>

#include <hatn/crypt/encryptmac.h>
#include <hatn/crypt/ciphersuite.h>

namespace hatn {

using namespace common;

namespace crypt {

/*********************** CipherSuite **************************/

//---------------------------------------------------------------
CipherSuite::CipherSuite() noexcept
    : m_suite(common::makeShared<cipher_suite::shared_traits::managed>()),
      m_cipherAlgorithm(nullptr),
      m_digestAlgorithm(nullptr),
      m_aeadAlgorithm(nullptr),
      m_macAlgorithm(nullptr),
      m_hkdfAlgorithm(nullptr),
      m_pbkdfAlgorithm(nullptr),
      m_dhAlgorithm(nullptr),
      m_ecdhAlgorithm(nullptr),
      m_signatureAlgorithm(nullptr),
      m_suites(nullptr)
{}

//---------------------------------------------------------------
CipherSuite::CipherSuite(const char *id)
    : CipherSuite()
{
    m_suite->setFieldValue(cipher_suite::id,id);
}

//---------------------------------------------------------------
CipherSuite::CipherSuite(const lib::string_view& id)
    : CipherSuite()
{
    m_suite->setFieldValue(cipher_suite::id,id);
}

//---------------------------------------------------------------
CipherSuite::CipherSuite(common::SharedPtr<cipher_suite::shared_traits::managed> suite) noexcept
  : m_suite(std::move(suite)),
    m_cipherAlgorithm(nullptr),
    m_digestAlgorithm(nullptr),
    m_aeadAlgorithm(nullptr),
    m_macAlgorithm(nullptr),
    m_hkdfAlgorithm(nullptr),
    m_pbkdfAlgorithm(nullptr),
    m_dhAlgorithm(nullptr),
    m_ecdhAlgorithm(nullptr),
    m_signatureAlgorithm(nullptr)
{}

//---------------------------------------------------------------
common::Error CipherSuite::load(const char *data, size_t size)
{
    clearRawStorage();
    if (!du::io::deserializeInline(*m_suite,data,size))
    {
        return cryptError(CryptError::CIPHER_SUITE_LOAD_FAILED);
    }
    return common::Error();
}

//---------------------------------------------------------------
const char* CipherSuite::id() const
{
    const auto& field=m_suite->field(cipher_suite::id);
    return field.c_str();
}

//---------------------------------------------------------------
common::Error CipherSuite::prepareRawStorage()
{
    auto buf=common::makeShared<dataunit::WireDataSingle>();
    if (dataunit::io::serialize(*m_suite,buf->impl())<0)
    {
        return cryptError(CryptError::CIPHER_SUITE_STORE_FAILED);
    }
    m_suite->keepWireData(std::move(buf));
    return OK;
}

//---------------------------------------------------------------
void CipherSuite::clearRawStorage()
{
    m_suite->resetWireDataKeeper();
}

//---------------------------------------------------------------
template <typename FieldT>
common::Error CipherSuite::findAlgorithm(
        const FieldT& field,
        CryptAlgorithmConstP &alg,
        CryptAlgorithmConstP &algCache,
        CryptAlgorithm::Type type
    ) const
{
    alg=algCache;
    if (alg==nullptr)
    {
        const auto& algField=m_suite->field(field);
        if (!algField.isSet())
        {
            return cryptError(CryptError::SUITE_ALGORITHM_NOT_FOUND);
        }
        // try to find engine specific for this algorithm type and name
        auto engine=suites()->engineForAlgorithm(type,algField.buf()->data(),algField.buf()->size(),false);
        if (engine==nullptr || engine->plugin()==nullptr)
        {
            // try to find engine specific for any algorithm of this type
            engine=suites()->engineForAlgorithm(type);
            if (engine==nullptr || engine->plugin()==nullptr)
            {
                return cryptError(CryptError::INVALID_ALGORITHM);
            }
        }
        if (algField.buf()->isRawBuffer())
        {
            HATN_CHECK_RETURN(engine->plugin()->findAlgorithm(alg,type,std::string(algField.dataPtr(),algField.dataSize()),engine->name()))
        }
        else
        {
            HATN_CHECK_RETURN(engine->plugin()->findAlgorithm(alg,type,algField.c_str(),engine->name()))
        }
        algCache=alg;
    }
    return common::Error();
}

//---------------------------------------------------------------
common::Error CipherSuite::cipherAlgorithm(CryptAlgorithmConstP &alg) const
{
    return findAlgorithm(cipher_suite::cipher,alg,m_cipherAlgorithm,CryptAlgorithm::Type::SENCRYPTION);
}

//---------------------------------------------------------------
common::Error CipherSuite::digestAlgorithm(CryptAlgorithmConstP &alg) const
{
    return findAlgorithm(cipher_suite::digest,alg,m_digestAlgorithm,CryptAlgorithm::Type::DIGEST);
}

//---------------------------------------------------------------
common::Error CipherSuite::aeadAlgorithm(CryptAlgorithmConstP &alg) const
{
    return findAlgorithm(cipher_suite::aead,alg,m_aeadAlgorithm,CryptAlgorithm::Type::AEAD);
}

//---------------------------------------------------------------
common::Error CipherSuite::macAlgorithm(CryptAlgorithmConstP &alg) const
{
    return findAlgorithm(cipher_suite::mac,alg,m_macAlgorithm,CryptAlgorithm::Type::MAC);
}

//---------------------------------------------------------------
common::Error CipherSuite::pbkdfAlgorithm(CryptAlgorithmConstP &alg) const
{
    return findAlgorithm(cipher_suite::pbkdf,alg,m_pbkdfAlgorithm,CryptAlgorithm::Type::PBKDF);
}

//---------------------------------------------------------------
common::Error CipherSuite::hkdfAlgorithm(CryptAlgorithmConstP &alg) const
{
    return findAlgorithm(cipher_suite::hkdf,alg,m_hkdfAlgorithm,CryptAlgorithm::Type::DIGEST);
}

//---------------------------------------------------------------
common::Error CipherSuite::dhAlgorithm(CryptAlgorithmConstP &alg) const
{
    return findAlgorithm(cipher_suite::dh,alg,m_dhAlgorithm,CryptAlgorithm::Type::DH);
}

//---------------------------------------------------------------
common::Error CipherSuite::ecdhAlgorithm(CryptAlgorithmConstP &alg) const
{
    return findAlgorithm(cipher_suite::ecdh,alg,m_ecdhAlgorithm,CryptAlgorithm::Type::ECDH);
}

//---------------------------------------------------------------
common::Error CipherSuite::signatureAlgorithm(CryptAlgorithmConstP &alg) const
{
    return findAlgorithm(cipher_suite::signature,alg,m_signatureAlgorithm,CryptAlgorithm::Type::SIGNATURE);
}

//---------------------------------------------------------------
common::SharedPtr<SEncryptor> CipherSuite::createSEncryptor(Error &ec) const
{
    CryptAlgorithmConstP alg;
    ec=cipherAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<SEncryptor>();
    }
    auto ret=alg->engine()->plugin()->createSEncryptor(alg);
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<SDecryptor> CipherSuite::createSDecryptor(Error &ec) const
{
    CryptAlgorithmConstP alg;
    ec=cipherAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<SDecryptor>();
    }
    auto ret=alg->engine()->plugin()->createSDecryptor(alg);
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<AEADEncryptor> CipherSuite::createAeadEncryptor(Error &ec) const
{
    CryptAlgorithmConstP alg;
    ec=aeadAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<AEADEncryptor>();
    }
    SharedPtr<AEADEncryptor> enc;
    if (alg->isBackendAlgorithm())
    {
        enc=alg->engine()->plugin()->createAeadEncryptor(alg);
    }
    else if (boost::algorithm::istarts_with(std::string(alg->name()),"encryptmac:"))
    {
        enc=makeShared<EncryptMACEnc>();
    }

    if (enc.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return enc;
}

//---------------------------------------------------------------
common::SharedPtr<AEADDecryptor> CipherSuite::createAeadDecryptor(Error &ec) const
{
    CryptAlgorithmConstP alg;
    ec=aeadAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<AEADDecryptor>();
    }
    SharedPtr<AEADDecryptor> dec;
    if (alg->isBackendAlgorithm())
    {
        dec=alg->engine()->plugin()->createAeadDecryptor(alg);
    }
    else if (boost::algorithm::istarts_with(std::string(alg->name()),"encryptmac:"))
    {
        dec=makeShared<EncryptMACDec>();
    }

    if (dec.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return dec;
}

//---------------------------------------------------------------
common::SharedPtr<Digest> CipherSuite::createDigest(Error &ec) const
{
    CryptAlgorithmConstP alg;
    ec=digestAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<Digest>();
    }
    auto ret=alg->engine()->plugin()->createDigest(alg);
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<MAC> CipherSuite::createMAC(Error &ec) const
{
    CryptAlgorithmConstP alg;
    ec=macAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<MAC>();
    }
    auto ret=alg->engine()->plugin()->createMAC(alg);
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<PBKDF> CipherSuite::createPBKDF(Error &ec,const CryptAlgorithm *targetKeyAlg) const
{
    CryptAlgorithmConstP alg;
    ec=pbkdfAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<PBKDF>();
    }
    auto ret=alg->engine()->plugin()->createPBKDF(targetKeyAlg,alg);
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<HKDF> CipherSuite::createHKDF(Error &ec,const CryptAlgorithm *targetKeyAlg) const
{
    CryptAlgorithmConstP alg;
    ec=hkdfAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<HKDF>();
    }
    auto ret=alg->engine()->plugin()->createHKDF(targetKeyAlg,alg);
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<DH> CipherSuite::createDH(Error &ec) const
{
    CryptAlgorithmConstP alg;
    ec=dhAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<DH>();
    }
    auto ret=alg->engine()->plugin()->createDH(alg);
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<ECDH> CipherSuite::createECDH(Error &ec) const
{
    CryptAlgorithmConstP alg;
    ec=ecdhAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<ECDH>();
    }
    auto ret=alg->engine()->plugin()->createECDH(alg);
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<SignatureSign> CipherSuite::createSignatureSign(common::Error& ec) const
{
    CryptAlgorithmConstP alg;
    ec=signatureAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<SignatureSign>();
    }
    auto ret=alg->createSignatureSign();
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<SignatureVerify> CipherSuite::createSignatureVerify(common::Error& ec) const
{
    CryptAlgorithmConstP alg;
    ec=signatureAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<SignatureVerify>();
    }
    auto ret=alg->createSignatureVerify();
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<PassphraseKey> CipherSuite::createPassphraseKey(Error &ec, const CryptAlgorithm *targetKeyAlg) const
{
    CryptAlgorithmConstP pbkdfAlg;
    ec=pbkdfAlgorithm(pbkdfAlg);
    if (ec)
    {
        return common::SharedPtr<PassphraseKey>();
    }
    auto ret=pbkdfAlg->engine()->plugin()->createPassphraseKey(targetKeyAlg,pbkdfAlg);
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::Error CipherSuite::loadFromFile(const char *filename)
{
    ByteArray barr;
    HATN_CHECK_RETURN(barr.loadFromFile(filename));
    return loadFromJSON(barr);
}

//---------------------------------------------------------------
common::SharedPtr<X509Certificate> CipherSuite::createX509Certificate(common::Error& ec) const
{
    CryptAlgorithmConstP alg;
    ec=signatureAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<X509Certificate>();
    }
    auto ret=alg->engine()->plugin()->createX509Certificate();
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<X509CertificateChain> CipherSuite::createX509CertificateChain(common::Error& ec) const
{
    CryptAlgorithmConstP alg;
    ec=signatureAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<X509CertificateChain>();
    }
    auto ret=alg->engine()->plugin()->createX509CertificateChain();
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
common::SharedPtr<X509CertificateStore> CipherSuite::createX509CertificateStore(common::Error& ec) const
{
    CryptAlgorithmConstP alg;
    ec=signatureAlgorithm(alg);
    if (ec)
    {
        return common::SharedPtr<X509CertificateStore>();
    }
    auto ret=alg->engine()->plugin()->createX509CertificateStore();
    if (ret.isNull())
    {
        ec=cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    return ret;
}

//---------------------------------------------------------------
const CipherSuites* CipherSuite::suites() const noexcept
{
    if (m_suites==nullptr)
    {
        return &CipherSuitesGlobal::instance();
    }
    return m_suites;
}

/*********************** CipherSuites **************************/

HATN_SINGLETON_INIT(CipherSuitesGlobal)

//---------------------------------------------------------------
CryptEngine* CipherSuites::engineForAlgorithm(
        CryptAlgorithm::Type type,
        const char *name,
        size_t nameLength,
        bool fallBackDefault
    ) const noexcept
{
    if (m_engines.empty())
    {
        return fallBackDefault?m_defaultEngine.get():nullptr;
    }

    if (name!=nullptr && nameLength==0)
    {
        nameLength=strlen(name);
    }
    CryptAlgorithmTypeNameMapKey key{type,CryptAlgorithm::Name(name,nameLength,true)};

    auto it=m_engines.find(key);
    if (it==m_engines.end())
    {
        key={type,CryptAlgorithm::Name()};
        it=m_engines.find(key);
        if (it==m_engines.end())
        {
            return fallBackDefault?m_defaultEngine.get():nullptr;
        }
    }
    return it->second.get();
}

//---------------------------------------------------------------
void CipherSuites::reset() noexcept
{
    m_defaultSuite.reset();
    m_suites.clear();
    m_defaultEngine.reset();
    m_engines.clear();
    m_randomGenerator.reset();
}

//---------------------------------------------------------------
void CipherSuites::setDefaultEngine(std::shared_ptr<CryptEngine> engine)
{
    m_defaultEngine=std::move(engine);
    auto plugin=defaultPlugin();
    if (plugin)
    {
        m_randomGenerator=plugin->createRandomGenerator();
    }
}

//---------------------------------------------------------------
CryptEngine* CipherSuites::defaultEngine() const noexcept
{
    return m_defaultEngine.get();
}

//---------------------------------------------------------------
void CipherSuites::loadEngines(std::map<CryptAlgorithmTypeNameMapKey, std::shared_ptr<CryptEngine> > engines)
{
    m_engines=std::move(engines);
}

//---------------------------------------------------------------
std::map<CryptAlgorithmTypeNameMapKey,std::shared_ptr<CryptEngine>> CipherSuites::engines() const noexcept
{
    return m_engines;
}

//---------------------------------------------------------------
const CipherSuite* CipherSuites::suite(const CipherSuite::IdType &key) const noexcept
{    
    auto it=m_suites.find(key);
    if (it!=m_suites.end())
    {
        return it->second.get();
    }
    if (key.empty())
    {
        return m_defaultSuite.get();
    }
    return nullptr;
}

//---------------------------------------------------------------
std::shared_ptr<CipherSuite> CipherSuites::suiteShared(lib::string_view id) const noexcept
{
    auto it=m_suites.find(id);
    if (it!=m_suites.end())
    {
        return it->second;
    }
    if (id.empty())
    {
        return m_defaultSuite;
    }
    return std::shared_ptr<CipherSuite>{};
}

//---------------------------------------------------------------
void CipherSuites::addSuite(std::shared_ptr<CipherSuite> suite)
{
    suite->setSuites(this);
    CipherSuite::IdType key(suite->id());
#if __cplusplus < 201703L
    m_suites[std::move(key)]=std::move(suite);
#else
    m_suites.insert_or_assign(std::move(key),std::move(suite));
#endif
}

//---------------------------------------------------------------
void CipherSuites::setDefaultSuite(std::shared_ptr<CipherSuite> suite) noexcept
{
    m_defaultSuite=std::move(suite);
}

//---------------------------------------------------------------
Error CipherSuites::setDefaultSuite(lib::string_view suiteId)
{
    auto s=suiteShared(suiteId);
    if (!s)
    {
        return cryptError((CryptError::INVALID_CIPHER_SUITE));
    }

    m_defaultSuite=std::move(s);
    return OK;
}

//---------------------------------------------------------------
const CipherSuite* CipherSuites::defaultSuite() const noexcept
{
    return m_defaultSuite.get();
}

//---------------------------------------------------------------
CipherSuitesGlobal& CipherSuitesGlobal::instance() noexcept
{
    static CipherSuitesGlobal inst;
    return inst;
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
