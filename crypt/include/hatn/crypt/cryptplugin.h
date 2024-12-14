/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/cryptplugin.h
  *
  *   Base class for encryption plugins
  *
  */

/****************************************************************************/

#ifndef HATNCRYPTPLUGIN_H
#define HATNCRYPTPLUGIN_H

#include <memory>

#include <hatn/common/featureset.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/plugin.h>
#include <hatn/common/locker.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/securestreamcontext.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/passphrasekey.h>
#include <hatn/crypt/cipherworker.h>
#include <hatn/crypt/digest.h>
#include <hatn/crypt/hmac.h>
#include <hatn/crypt/pbkdf.h>
#include <hatn/crypt/hkdf.h>
#include <hatn/crypt/signature.h>
#include <hatn/crypt/randomgenerator.h>
#include <hatn/crypt/passwordgenerator.h>
#include <hatn/crypt/dh.h>
#include <hatn/crypt/ecdh.h>
#include <hatn/crypt/aeadworker.h>
#include <hatn/crypt/x509certificate.h>
#include <hatn/crypt/x509certificatechain.h>
#include <hatn/crypt/x509certificatestore.h>

HATN_CRYPT_NAMESPACE_BEGIN

namespace detail
{
struct CryptTraits
{
    using MaskType=uint32_t;
    enum class Feature : MaskType
    {
        RandomGenerator=0,
        PasswordGenerator,
        Digest,
        SymmetricEncryption,
        Signature,
        AEAD,
        MAC,
        HMAC,
        HKDF,
        PBKDF,
        X509,
        DH,
        ECDH,
        TLS,
        ConstTimeMemCmp,

        END
    };
};
}

using Crypt=common::FeatureSet<detail::CryptTraits>;

struct CryptPluginConfig
{
    virtual ~CryptPluginConfig()=default;
    CryptPluginConfig(const CryptPluginConfig&)=default;
    CryptPluginConfig(CryptPluginConfig&&) =default;
    CryptPluginConfig& operator=(const CryptPluginConfig&)=default;
    CryptPluginConfig& operator=(CryptPluginConfig&&) =default;
};

//! Base class for encryption plugins
class HATN_CRYPT_EXPORT CryptPlugin : public common::Plugin
{
    public:

        //! Crypt plugin type
        constexpr static const char* Type="com.hatn.cryptplugin";

        //! Ctor
        explicit CryptPlugin(
            const common::PluginInfo* pluginInfo
        ) noexcept;

        /**
         * @brief Initialize plugin
         */
        virtual common::Error init(const CryptPluginConfig* cfg=nullptr) =0;

        //! Clean up backend and deinitialize plugin
        inline common::Error cleanup()
        {
            auto ec=doCleanup(m_algs);
            m_algs.clear();
            m_engines.clear();
            return ec;
        }

        /**
         * @brief List names of all implemented digest algorithms
         * @return Names of digests
         */
        virtual std::vector<std::string> listDigests() const
        {
            return std::vector<std::string>();
        }

        /**
         * @brief List names of all implemented symmetric cipher algorithms
         * @return Names of symmetric ciphers
         */
        virtual std::vector<std::string> listCiphers() const
        {
            return std::vector<std::string>();
        }

        /**
         * @brief List names of all built-in elliptic curves
         * @return Names of eliptic curves
         */
        virtual std::vector<std::string> listEllipticCurves() const
        {
            return std::vector<std::string>();
        }

        /**
         * @brief List names of all implemented AEAD cipher algorithms
         * @return Names of AEAD ciphers
         *
         * Names can be compound like "BASE_NAME/parameter1/.../parameterN".
         * In this case listed name will give a hint:
         * <pre>
         * "BASE_NAME[/<parameter1>]" - optional parameter
         * "BASE_NAME/<parameter1>" - mandatory parameter
         * </pre>
         */
        virtual std::vector<std::string> listAEADs() const
        {
            return std::vector<std::string>();
        }

        /**
         * @brief List names of all implemented MAC algorithms
         * @return Names of MAC algorithms
         *
         * Names can be compound like "BASE_NAME/parameter1/.../parameterN".
         * In this case listed name will give a hint:
         * <pre>
         * "BASE_NAME[/<parameter1>]" - optional parameter
         * "BASE_NAME/<parameter1>" - mandatory parameter
         * </pre>
         */
        virtual std::vector<std::string> listMACs() const
        {
            return std::vector<std::string>();
        }

        /**
         * @brief List names of all implemented PBKDF algorithms
         * @return Names of PBKDF algorithms
         *
         * Names can be compound like "BASE_NAME/parameter1/.../parameterN".
         * In this case listed name will give a hint:
         * <pre>
         * "BASE_NAME[/<parameter1>]" - optional parameter
         * "BASE_NAME/<parameter1>" - mandatory parameter
         * </pre>
         */
        virtual std::vector<std::string> listPBKDFs() const
        {
            return std::vector<std::string>();
        }

        /**
         * @brief List algorithms that can be used gor digital signature
         * @return List of algorithms
         *
         * Names can be compound like "BASE_NAME/parameter1/.../parameterN".
         * In this case listed name will give a hint:
         * <pre>
         * "BASE_NAME[/<parameter1>]" - optional parameter
         * "BASE_NAME/<parameter1>" - mandatory parameter
         * </pre>
         */
        virtual std::vector<std::string> listSignatures() const
        {
            return std::vector<std::string>();
        }

        /**
         * @brief List names of all implemented DH algorithms.
         * @return Names of DH algorithms.
         *
         * Normally, an algorithm's name means name of corresponding safe prime group
         * for Diffie-Hellman algorithm.
         * Names can be compound like "BASE_NAME/parameter1/.../parameterN".
         * In this case listed name will give a hint:
         * <pre>
         * "BASE_NAME[/<parameter1>]" - optional parameter
         * "BASE_NAME/<parameter1>" - mandatory parameter
         * </pre>
         */
        virtual std::vector<std::string> listDHs() const
        {
            return std::vector<std::string>();
        }

        /**
         * @brief Find cryptographic engine
         * @param engineName Name of engine where to search for algorithm implementation
         * @param engine Result pointer
         * @return Operaion status
         */
        common::Error findEngine(
            const char* engineName,
            std::shared_ptr<CryptEngine>& engine
        );

        /**
         * @brief Find cryptographic algorithm implementation
         * @param alg Result pointer
         * @param type Type of algorithm
         * @param name Name of algorithm in the following format:
                       "native_name/parameter1/parameter2/.../parameterN".
                       E.g.: "RSA/3076", "EC/SECP224R1", "AES-256-CBC", etc.
         * @param engineName Name of engine where to search for algorithm implementation
         * @return Operaion status
         */
        common::Error findAlgorithm(
            CryptAlgorithmConstP& alg,
            CryptAlgorithm::Type type,
            const char* name,
            const char* engineName=nullptr
        );

        common::Error findAlgorithm(
            CryptAlgorithmConstP& alg,
            CryptAlgorithm::Type type,
            const std::string& name,
            const char* engineName=nullptr
        )
        {
            return findAlgorithm(alg,type,name.c_str(),engineName);
        }

        //! Create TLS context
        virtual common::SharedPtr<SecureStreamContext> createSecureStreamContext(
            SecureStreamContext::EndpointType endpointType=SecureStreamContext::EndpointType::Generic,
            SecureStreamContext::VerifyMode verifyMode=SecureStreamContext::VerifyMode::None
        ) const
        {
            std::ignore=endpointType;
            std::ignore=verifyMode;
            return common::SharedPtr<SecureStreamContext>();
        }

        //! Create symmetric encryptor
        virtual common::SharedPtr<SEncryptor> createSEncryptor(const SymmetricKey* key=nullptr) const
        {
            std::ignore=key;
            return common::SharedPtr<SEncryptor>();
        }
        //! Create symmetric decryptor
        virtual common::SharedPtr<SDecryptor> createSDecryptor(const SymmetricKey* key=nullptr) const
        {
            std::ignore=key;
            return common::SharedPtr<SDecryptor>();
        }

        //! Create AEAD encryptor
        virtual common::SharedPtr<AEADEncryptor> createAeadEncryptor(const SymmetricKey* key=nullptr) const
        {
            std::ignore=key;
            return common::SharedPtr<AEADEncryptor>();
        }

        //! Create AEAD decryptor
        virtual common::SharedPtr<AEADDecryptor> createAeadDecryptor(const SymmetricKey* key=nullptr) const
        {
            std::ignore=key;
            return common::SharedPtr<AEADDecryptor>();
        }

        //! Create digest processor
        virtual common::SharedPtr<Digest> createDigest(const CryptAlgorithm* alg=nullptr) const
        {
            std::ignore=alg;
            return common::SharedPtr<Digest>();
        }

        /**
         * @brief Create MAC processor
         * @return Created MAC
         */
        virtual common::SharedPtr<MAC> createMAC(const SymmetricKey* key=nullptr) const
        {
            std::ignore=key;
            return common::SharedPtr<MAC>();
        }

        /**
         * @brief Create HMAC processor
         * @return Created HMAC
         */
        virtual common::SharedPtr<HMAC> createHMAC(const CryptAlgorithm* alg=nullptr) const
        {
            std::ignore=alg;
            return common::SharedPtr<HMAC>();
        }

        /**
         * @brief Create PBKDF processor
         * @param kdfAlg KDF algorithm to use for derivation
         * @param cipherAlg Algorithm to use for actual encryption key if applicable
         * @return PBKDF processor
         */
        virtual common::SharedPtr<PBKDF> createPBKDF(const CryptAlgorithm* cipherAlg=nullptr,
                                                     const CryptAlgorithm* kdfAlg=nullptr
                                                ) const
        {
            std::ignore=cipherAlg;
            std::ignore=kdfAlg;
            return common::SharedPtr<PBKDF>();
        }

        /**
         * @brief Create HKDF processor
         * @param hashAlg Hash algorithm to use for derivation
         * @param cipherAlg Algorithm to use for actual encryption key if applicable
         * @return PBKDF processor
         */
        virtual common::SharedPtr<HKDF> createHKDF(const CryptAlgorithm* cipherAlg=nullptr,
                                                     const CryptAlgorithm* hashAlg=nullptr
                                                ) const
        {
            std::ignore=cipherAlg;
            std::ignore=hashAlg;
            return common::SharedPtr<HKDF>();
        }

        /**
         * @brief Create DH processor
         * @return DH processor
         */
        virtual common::SharedPtr<DH> createDH(const CryptAlgorithm* alg=nullptr) const
        {
            std::ignore=alg;
            return common::SharedPtr<DH>();
        }

        /**
         * @brief Create ECDH processor
         * @return ECDH processor
         */
        virtual common::SharedPtr<ECDH> createECDH(const CryptAlgorithm* alg) const
        {
            std::ignore=alg;
            return common::SharedPtr<ECDH>();
        }

        common::SharedPtr<SEncryptor> createSEncryptor(const CryptAlgorithm* alg) const
        {
            auto obj=createSEncryptor();
            if (obj)
            {
                obj->setAlg(alg);
            }
            return obj;
        }
        common::SharedPtr<SDecryptor> createSDecryptor(const CryptAlgorithm* alg) const
        {
            auto obj=createSDecryptor();
            if (obj)
            {
                obj->setAlg(alg);
            }
            return obj;
        }
        common::SharedPtr<AEADEncryptor> createAeadEncryptor(const CryptAlgorithm* alg) const
        {
            auto obj=createAeadEncryptor();
            if (obj)
            {
                obj->setAlg(alg);
            }
            return obj;
        }
        common::SharedPtr<AEADDecryptor> createAeadDecryptor(const CryptAlgorithm* alg) const
        {
            auto obj=createAeadDecryptor();
            if (obj)
            {
                obj->setAlg(alg);
            }
            return obj;
        }
        common::SharedPtr<MAC> createMAC(const CryptAlgorithm* alg) const
        {
            auto obj=createMAC();
            if (obj)
            {
                obj->setAlg(alg);
            }
            return obj;
        }

        /**
         * @brief Create handler to work with passhrase keys
         * @param kdfAlg Algorithm to use for derivation of actual encryption key if applicable
         * @param cipherAlg Algorithm to use for actual encryption key if applicable
         * @return New key handler without key's content
         */
        virtual common::SharedPtr<PassphraseKey> createPassphraseKey(const CryptAlgorithm* cipherAlg=nullptr,
                                                                     const CryptAlgorithm* kdfAlg=nullptr
                                                          ) const
        {
            std::ignore=cipherAlg;
            std::ignore=kdfAlg;
            return common::SharedPtr<PassphraseKey>();
        }

        /**
         * @brief Create public key for asymmetric algorithms (signature, DH, asymmetric cipher)
         * @param alg Asymmetric algorithm
         * @return New key
         */
        virtual common::SharedPtr<PublicKey> createPublicKey(const CryptAlgorithm* alg=nullptr) const
        {
            std::ignore=alg;
            return common::SharedPtr<PublicKey>();
        }

        /**
         * @brief Create asymmetric private key from content
         * @param pkey Target key
         * @param buf Buffer pointer
         * @param size Buffer size
         * @param format Data format
         * @param protector Key protector
         * @param engineName Name of backend engine
         * @return Operation status
         *
         */
        virtual common::Error createAsymmetricPrivateKeyFromContent(
            common::SharedPtr<PrivateKey>& pkey,
            const char* buf,
            size_t size,
            ContainerFormat format,
            KeyProtector* protector=nullptr,
            const char* engineName=nullptr
        )
        {
            std::ignore=pkey;
            std::ignore=buf;
            std::ignore=size;
            std::ignore=format;
            std::ignore=protector;
            std::ignore=engineName;
            return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
        }

        /**
         * @brief Create asymmetric private key from content
         * @param pkey Target key
         * @param buf Buffer with content
         * @param format Data format
         * @param protector Key protector
         * @param engineName Name of backend engine
         * @return Operation status
         *
         */
        template <typename ContainerT>
        common::Error createAsymmetricPrivateKeyFromContent(
            common::SharedPtr<PrivateKey>& pkey,
            const ContainerT& buf,
            ContainerFormat format,
            KeyProtector* protector=nullptr,
            const char* engineName=nullptr
        )
        {
            return createAsymmetricPrivateKeyFromContent(pkey,buf.data(),buf.size(),format,protector,engineName);
        }

        /**
         * @brief Get algorithm of public key
         * @param alg Target algorithm
         * @param key Public key
         * @param engineName Name of backend engine
         * @return Operation status
         */
        virtual common::Error publicKeyAlgorithm(
            CryptAlgorithmConstP &alg,
            const PublicKey* key,
            const char* engineName=nullptr
        )
        {
            std::ignore=alg;
            std::ignore=key;
            std::ignore=engineName;
            return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
        }

        /**
         * @brief Compare two memory regions in constant time avoiding timing attacks on HMAC
         * @param a
         * @param b
         * @param len
         * @return 0 if equals
         *
         * @note Default implementation was copied from openssl cryptlib.c, see annotations there
         */
        virtual int constTimeMemCmp(const void *a, const void *b, size_t len) const;

        /**
         * @brief Create random data generator
         * @return Generator
         */
        virtual common::SharedPtr<RandomGenerator> createRandomGenerator() const
        {
            return common::SharedPtr<RandomGenerator>();
        }

        /**
         * @brief Create random password generator
         * @return Generator
         */
        virtual common::SharedPtr<PasswordGenerator> createPasswordGenerator() const
        {
            return common::SharedPtr<PasswordGenerator>();
        }

        /**
         * @brief Create X.509 certificate
         * @return X.509 certificate
         */
        virtual common::SharedPtr<X509Certificate> createX509Certificate() const
        {
            return common::SharedPtr<X509Certificate>();
        }

        /**
         * @brief Create X.509 certificate chain
         * @return X.509 certificate chain
         */
        virtual common::SharedPtr<X509CertificateChain> createX509CertificateChain() const
        {
            return common::SharedPtr<X509CertificateChain>();
        }

        /**
         * @brief Create X.509 certificate store
         * @return X.509 certificate store
         */
        virtual common::SharedPtr<X509CertificateStore> createX509CertificateStore() const
        {
            return common::SharedPtr<X509CertificateStore>();
        }

        /**
         * @brief Get features implemented by plugin
         * @return Implemeted features
         */
        virtual Crypt::Features features() const noexcept
        {
            return 0;
        }

        /**
         * @brief Chack if plugin implements a feature
         * @param feature Feature to check
         * @return
         */
        bool isFeatureImplemented(Crypt::Feature feature) const noexcept
        {
            return Crypt::hasFeature(features(),feature);
        }

        /**
         * @brief Compare contents of two containers
         * @param a Container A
         * @param b Container B
         * @param size Size of data to compare, if zero the size of Container A
         * @param offsetA Offset in Container A
         * @param offsetB Offset in Container B
         * @return True of succeded
         *
         * Comparison is not time constant if the containers' sizes are not equal.
         * If the sizes are equal then contents of containers are compared in time constant manner.
         */
        template <typename ContainerA, typename ContainerB>
        bool compareContainers(
                const ContainerA& a,
                const ContainerB& b,
                size_t size=0,
                size_t offsetA=0,
                size_t offsetB=0
            )
        {
            if (a.size()==0 && b.size()==0)
            {
                return true;
            }
            if (a.size()==0 || b.size()==0)
            {
                return false;
            }
            if (size==0)
            {
                size=a.size();
                if (offsetA!=0)
                {
                    if (offsetA>size)
                    {
                        return false;
                    }
                    size-=offsetA;
                }
            }
            if (a.size()<(size+offsetA) || b.size()<(size+offsetB))
            {
                return false;
            }
            return constTimeMemCmp(a.data()+offsetA,b.data()+offsetB,size)==0;
        }

        common::Error randBytes(char* data,size_t size) const;

        template <typename ContainerT>
        common::Error randContainer(ContainerT& container, size_t maxSize, size_t minSize=0)
        {
            if (!m_randGen)
            {
                const_cast<CryptPlugin*>(this)->m_randGen=createRandomGenerator();
            }
            if (!m_randGen)
            {
                return cryptError(CryptError::GENERAL_FAIL);
            }
            return m_randGen->randContainer(container,maxSize,minSize);
        }

    protected:

        using AlgTable=std::map<CryptAlgorithmMapKey,std::shared_ptr<CryptAlgorithm>>;

        /**
         * @brief Find cryptographic algorithm implementation
         * @param alg Result pointer
         * @param type Type of algorithm
         * @param name Name of algorithm in the following format:
                       "native_name/parameter1/parameter2/.../parameterN".
                       E.g.: "RSA/3076", "EC/SECP224R1", "AES-256-CBC", etc.
         * @param engineName Name of engine where to search for algorithm implementation
         * @return Operaion status
         */
        virtual common::Error doFindAlgorithm(
            std::shared_ptr<CryptAlgorithm>& alg,
            CryptAlgorithm::Type type,
            const char* name,
            CryptEngine* engine
        ) =0;

        //! Clean up backend
        virtual common::Error doCleanup(std::map<CryptAlgorithm::Type,std::shared_ptr<AlgTable>>& algs) =0;

        virtual common::Error doFindEngine(const char* engineName, std::shared_ptr<CryptEngine>& engine)
        {
            std::ignore=engineName;
            std::ignore=engine;
            return cryptError(CryptError::INVALID_ENGINE);
        }

        std::shared_ptr<CryptEngine> defaultEngine() const noexcept
        {
            return m_defaultEngine;
        }

    private:

        std::map<CryptAlgorithm::Type,std::shared_ptr<AlgTable>> m_algs;
        std::map<const char*,std::shared_ptr<CryptEngine>> m_engines;
        std::shared_ptr<CryptEngine> m_defaultEngine;
        common::SharedPtr<RandomGenerator> m_randGen;

        common::MutexLock m_algMutex;
};

//---------------------------------------------------------------
template <typename ContainerInT, typename ContainerOutT>
common::Error Digest::digest(
        const CryptAlgorithm* algorithm,
        const ContainerInT& data,
        ContainerOutT& result,
        size_t offsetIn,
        size_t sizeIn,
        size_t offsetOut
    )
{
    auto ctx=algorithm->engine()->plugin()->createDigest();
    if (!ctx)
    {
        return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    HATN_CHECK_RETURN(ctx->init(algorithm))
    return ctx->processAndFinalize(data,result,offsetIn,sizeIn,offsetOut);
}

//---------------------------------------------------------------
template <typename ContainerInT, typename ContainerHashT>
common::Error Digest::check(
    const CryptAlgorithm* algorithm,
    const ContainerInT& data,
    const ContainerHashT& hash,
    size_t offsetIn,
    size_t sizeIn,
    size_t offsetHash,
    size_t sizeHash
)
{
    common::ByteArray calcHash;

    HATN_CHECK_RETURN(digest(algorithm,data,calcHash,offsetIn,sizeIn))
    if (!algorithm->engine()->plugin()->compareContainers(hash,calcHash,sizeHash,offsetHash))
    {
        return cryptError(CryptError::DIGEST_MISMATCH);
    }

    return common::Error();
}

//---------------------------------------------------------------
template <typename ContainerInT, typename ContainerOutT>
common::Error HMAC::hmac(
    const CryptAlgorithm* algorithm,
    const MACKey* key,
    const ContainerInT& data,
    ContainerOutT& result,
    size_t offsetIn,
    size_t sizeIn,
    size_t offsetOut
)
{
    auto ctx=algorithm->engine()->plugin()->createHMAC();
    if (!ctx)
    {
        return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }
    ctx->setKey(key);
    HATN_CHECK_RETURN(ctx->init(algorithm))
    return ctx->processAndFinalize(data,result,offsetIn,sizeIn,offsetOut);
}

//---------------------------------------------------------------
template <typename ContainerInT, typename ContainerHashT>
common::Error HMAC::checkHMAC(
    const CryptAlgorithm* algorithm,
    const MACKey* key,
    const ContainerInT& data,
    const ContainerHashT& hash,
    size_t offsetIn,
    size_t sizeIn,
    size_t offsetHash,
    size_t sizeHash
)
{
    common::ByteArray calcHash;

    HATN_CHECK_RETURN(hmac(algorithm,key,data,calcHash,offsetIn,sizeIn))
    if (!algorithm->engine()->plugin()->compareContainers(hash,calcHash,sizeHash,offsetHash))
    {
        return cryptError(CryptError::MAC_FAILED);
    }

    return common::Error();
}

//---------------------------------------------------------------
template <typename SaltContainerT>
common::Error PBKDF::derive(
    PassphraseKey* passphrase,
    const SaltContainerT& salt
)
{
    auto handler=passphrase->kdfAlg()->engine()->plugin()->createPBKDF(passphrase->alg(),passphrase->kdfAlg());
    if (handler.isNull())
    {
        return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
    }

    common::SharedPtr<SymmetricKey> derivedKeyTmp;
    common::SharedPtr<SymmetricKey>* derivedKey=&derivedKeyTmp;

    if (passphrase->isDerivedKeyReady())
    {
        derivedKey=&passphrase->derivedKeyHolder();
    }
    auto ec=handler->derive(passphrase,*derivedKey,salt);
    if (ec)
    {
        return ec;
    }

    passphrase->setDerivedKey(*derivedKey);
    return common::Error();
}

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTPLUGIN_H
