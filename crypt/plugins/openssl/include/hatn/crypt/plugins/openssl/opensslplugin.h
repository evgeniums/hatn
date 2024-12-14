/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file crypt/plugins/openssl/opensslplugin.h
  *
  *   Encryption plugin that uses OpenSSL as backend
  *
  */

/****************************************************************************/

#ifndef HATNOPENSSLPLUGIN_H
#define HATNOPENSSLPLUGIN_H

#include <hatn/crypt/cryptplugin.h>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Encryption plugin that uses OpenSSL as backend
class HATN_OPENSSL_EXPORT OpenSslPlugin : public CryptPlugin
{
    public:

        constexpr static const char* Name="hatnopenssl";
        constexpr static const char* Description="OpenSSL Plugin";
        constexpr static const char* Vendor="hatn.com";
        constexpr static const char* Revision="1.0.0";

        using CryptPlugin::CryptPlugin;

        /**
         * @brief Initialize plugin
         */
        virtual common::Error init(const CryptPluginConfig* cfg=nullptr) override;

        //! Create TLS context
        virtual common::SharedPtr<SecureStreamContext> createSecureStreamContext(
            SecureStreamContext::EndpointType endpointType=SecureStreamContext::EndpointType::Generic,
            SecureStreamContext::VerifyMode verifyMode=SecureStreamContext::VerifyMode::None
        ) const override;

        /**
         * @brief Compare two memory regions in constant time avoiding timing attacks on HMAC
         * @param a
         * @param b
         * @param len
         * @return 0 if equals
         */
        virtual int constTimeMemCmp(const void *a, const void *b, size_t len) const override;

        /**
         * @brief Create handler to work with passhrase keys
         * @param kdfAlg Algorithm to use for derivation of actual encryption key if applicable
         * @param cipherAlg Algorithm to use for actual encryption key if applicable
         * @return New key handler without key's content
         */
        virtual common::SharedPtr<PassphraseKey> createPassphraseKey(const CryptAlgorithm* cipherAlg=nullptr,
                                                                     const CryptAlgorithm* kdfAlg=nullptr
                                                          ) const override;

        /**
         * @brief Create public key for asymmetric algorithms (signature, DH, asymmetric cipher)
         * @return New key
         */
        virtual common::SharedPtr<PublicKey> createPublicKey(const CryptAlgorithm* algorithm=nullptr) const override;

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
        ) override;

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
        ) override;

        //! Create symmetric encryptor
        virtual common::SharedPtr<SEncryptor> createSEncryptor(const SymmetricKey* key=nullptr) const override;
        //! Create symmetric decryptor
        virtual common::SharedPtr<SDecryptor> createSDecryptor(const SymmetricKey* key=nullptr) const override;

        //! Create AEAD encryptor
        virtual common::SharedPtr<AEADEncryptor> createAeadEncryptor(const SymmetricKey* key=nullptr) const override;
        //! Create AEAD decryptor
        virtual common::SharedPtr<AEADDecryptor> createAeadDecryptor(const SymmetricKey* key=nullptr) const override;

        //! Create digest processor
        virtual common::SharedPtr<Digest> createDigest(const CryptAlgorithm* alg=nullptr) const override;

        /**
         * @brief Create MAC processor
         * @return Created MAC
         */
        virtual common::SharedPtr<MAC> createMAC(const SymmetricKey* key=nullptr) const override;

        /**
         * @brief Create HMAC processor
         * @return Created HMAC
         */
        virtual common::SharedPtr<HMAC> createHMAC(const CryptAlgorithm* alg=nullptr) const override;

        /**
         * @brief Create PBKDF processor
         * @param kdfAlg KDF algorithm to use for derivation
         * @param cipherAlg Algorithm to use for actual encryption key if applicable
         * @return PBKDF processor
         */
        virtual common::SharedPtr<PBKDF> createPBKDF(const CryptAlgorithm* cipherAlg=nullptr,
                                                     const CryptAlgorithm* kdfAlg=nullptr
                                                ) const override;

        /**
         * @brief Create HKDF processor
         * @param hashAlg Hash algorithm to use for derivation
         * @param cipherAlg Algorithm to use for actual encryption key if applicable
         * @return PBKDF processor
         */
        virtual common::SharedPtr<HKDF> createHKDF(const CryptAlgorithm* cipherAlg=nullptr,
                                                     const CryptAlgorithm* hashAlg=nullptr
                                                ) const override;

        /**
         * @brief Create DH processor
         * @return DH processor
         */
        virtual common::SharedPtr<DH> createDH(const CryptAlgorithm* alg=nullptr) const override;

        /**
         * @brief Create ECDH processor
         * @return ECDH processor
         */
        virtual common::SharedPtr<ECDH> createECDH(const CryptAlgorithm* alg) const override;

        /**
         * @brief Create random data generator
         * @return Generator
         */
        virtual common::SharedPtr<RandomGenerator> createRandomGenerator() const override;

        /**
         * @brief Create random password generator
         * @return Generator
         */
        virtual common::SharedPtr<PasswordGenerator> createPasswordGenerator() const override;

        /**
         * @brief Create X.509 certificate
         * @return X.509 certificate
         */
        virtual common::SharedPtr<X509Certificate> createX509Certificate() const override;

        /**
         * @brief Create X.509 certificate chain
         * @return X.509 certificate chain
         */
        virtual common::SharedPtr<X509CertificateChain> createX509CertificateChain() const override;

        /**
         * @brief Create X.509 certificate store
         * @return X.509 certificate store
         */
        virtual common::SharedPtr<X509CertificateStore> createX509CertificateStore() const override;

        /**
         * @brief Get features implemented by plugin
         * @return Implemeted features
         */
        virtual Crypt::Features features() const noexcept override
        {
            return Crypt::allFeatures();
        }

        /**
         * @brief List names of all implemented digest algorithms
         * @return Names of digests
         */
        virtual std::vector<std::string> listDigests() const override;

        /**
         * @brief List names of all implemented symmetric cipher algorithms
         * @return Names of symmetric ciphers
         */
        virtual std::vector<std::string> listCiphers() const override;

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
        virtual std::vector<std::string> listAEADs() const override;

        /**
         * @brief List names of all built-in elliptic curves
         * @return Names of eliptic curves
         */
        virtual std::vector<std::string> listEllipticCurves() const override;

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
         * </pre>
         */
        virtual std::vector<std::string> listMACs() const override;

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
         * </pre>
         */
        virtual std::vector<std::string> listPBKDFs() const override;

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
        virtual std::vector<std::string> listSignatures() const override;

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
        virtual std::vector<std::string> listDHs() const override;

        inline static int sslCtxIdx() noexcept
        {
            return 1;
        }

    protected:

        /**
         * @brief Find cryptographic algorithm implementation
         * @param alg Result pointer
         * @param type Type of algorithm
         * @param name Name of algorithm in the following format:
                       "native_name/parameter1/parameter2/.../parameterN".
                       E.g.: "RSA/3076", "EC/SECP224R1", "AES-256-CBC", etc.
         * @param engine Engine where to search for algorithm implementation
         * @return Operaion status
         */
        virtual common::Error doFindAlgorithm(
            std::shared_ptr<CryptAlgorithm>& alg,
            CryptAlgorithm::Type type,
            const char* name,
            CryptEngine* engine
        ) override;

        //! Clean up backend
        virtual common::Error doCleanup(std::map<CryptAlgorithm::Type,std::shared_ptr<AlgTable>>& algs) override;

        virtual common::Error doFindEngine(const char* engineName, std::shared_ptr<CryptEngine>& engine) override;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLPLUGIN_H
