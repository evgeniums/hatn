/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/ciphersuite.h
 *
 *      Cipher suite
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTCIPHERSUITE_H
#define HATNCRYPTCIPHERSUITE_H

#include <hatn/crypt/crypt.h>

#include <hatn/common/objectid.h>
#include <hatn/common/singleton.h>

#include <hatn/crypt/cryptalgorithm.h>
#include <hatn/crypt/cryptplugin.h>

#include <hatn/crypt/cryptdataunits.h>

HATN_CRYPT_NAMESPACE_BEGIN

/**
 * @brief Cipher suite
 *
 * Maximum length of suite's ID is 128 bytes
 */
class HATN_CRYPT_EXPORT CipherSuite
{
    public:

        using IdType=common::FixedByteArray128;

        //! Ctor
        CipherSuite() noexcept;

        /**
         * @brief Ctor with ID
         * @param Suit ID
         */
        CipherSuite(const char* id);

        /**
         * @brief Ctor with ID
         * @param Suit ID
         */
        CipherSuite(const lib::string_view& id);

        /**
         * @brief Ctor from external suit data unit
         * @param suite Suit data unit
         */
        CipherSuite(common::SharedPtr<cipher_suite::shared_traits::managed> suite) noexcept;

        ~CipherSuite()=default;
        CipherSuite(const CipherSuite& other)=delete;
        CipherSuite(CipherSuite&& other)=default;

        CipherSuite& operator=(const CipherSuite& other)=delete;
        CipherSuite& operator=(CipherSuite&& other)=default;

        /**
         * @brief Load suit from JSON
         * @param json Suit description
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error loadFromJSON(const ContainerT& json)
        {
            clearRawStorage();
            if (!m_suite->loadFromJSON(json))
            {
                return cryptError(CryptError::CIPHER_SUITE_JSON_FAILED);
            }
            return common::Error();
        }

        /**
         * @brief Create suite from JSON description
         * @param json JSON description
         * @return Created suite
         *
         * @throws std::runtime_error if JSON pasing fails
         */
        template <typename ContainerT>
        static std::shared_ptr<CipherSuite> fromJSON(const ContainerT& json)
        {
            auto suite=std::make_shared<CipherSuite>();
            if (!suite->loadFromJSON(json))
            {
                throw common::ErrorException(cryptError(CryptError::CIPHER_SUITE_JSON_FAILED));
            }
            return suite;
        }

        /**
         * @brief Serializa suite to JSON
         * @param container Target container
         * @param prettyFormat Add line endings and identations
         * @param maxDecimalPlaces Maximum number of decimal places for double output
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error toJSON(ContainerT& container,bool prettyFormat=false, int maxDecimalPlaces=0) const
        {
            return m_suite->toJSON(container,prettyFormat,maxDecimalPlaces);
        }

        /**
         * @brief Load suite from buffer
         * @param data Buffer pointer
         * @param size Data size
         * @return Operation status
         */
        common::Error load(
            const char* data,
            size_t size
        );

        /**
         * @brief Load suite from container
         * @param container Data container
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error load(const ContainerT& container)
        {
            return load(container.data(),container.size());
        }

        /**
         * @brief Store suite to container
         * @param container Target container
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error store(
            ContainerT& container
        ) const
        {
            auto wireData=m_suite->wireDataKeeper();
            if (wireData.isNull())
            {
                HATN_CHECK_RETURN(const_cast<CipherSuite*>(this)->prepareRawStorage())
                wireData=m_suite->wireDataKeeper();
            }
            const auto* buf=wireData->mainContainer();
            container.load(buf->data(),buf->size());
            return common::Error();
        }

        /**
         * @brief Load suite from file
         * @param filename Filename
         * @return Operation status
         */
        common::Error loadFromFile(const char* filename);

        /**
         * @brief Load suite from file
         * @param filename Filename
         * @return Operation status
         */
        common::Error loadFromFile(const std::string& filename)
        {
            return loadFromFile(filename.c_str());
        }

        //! Get suite's ID
        const char* id() const;

        /**
         * @brief Store suite's ID to container
         * @param container Target container
         * @return Operation status
         */
        template <typename ContainerT>
        void storeID(ContainerT& container)
        {
            container.load(m_suite->fieldValue(cipher_suite::id));
        }

        //! Get suite
        const common::SharedPtr<cipher_suite::shared_traits::managed>& suite() const noexcept
        {
            return m_suite;
        }
        //! Set suite
        void setSuite(common::SharedPtr<cipher_suite::shared_traits::managed> suite) noexcept
        {
            m_suite=std::move(suite);
        }

        /**
         * @brief Get cipher algorithm of the suite
         * @param alg Algorithm
         * @return Operation status
         */
        common::Error cipherAlgorithm(CryptAlgorithmConstP& alg) const;
        /**
         * @brief Get digest algorithm of the suite
         * @param alg Algorithm
         * @return Operation status
         */
        common::Error digestAlgorithm(CryptAlgorithmConstP& alg) const;
        /**
         * @brief Get AEAD algorithm of the suite
         * @param alg Algorithm
         * @return Operation status
         */
        common::Error aeadAlgorithm(CryptAlgorithmConstP& alg) const;
        /**
         * @brief Get MAC algorithm of the suite
         * @param alg Algorithm
         * @return Operation status
         */
        common::Error macAlgorithm(CryptAlgorithmConstP& alg) const;
        /**
         * @brief Get HKDF algorithm of the suite
         * @param alg Algorithm
         * @return Operation status
         *
         * HKDF algorithm here is a kind of digest algorithm that will be used in HKDF
         */
        common::Error hkdfAlgorithm(CryptAlgorithmConstP& alg) const;
        /**
         * @brief Get PBKDF algorithm of the suite
         * @param alg Algorithm
         * @return Operation status
         */
        common::Error pbkdfAlgorithm(CryptAlgorithmConstP& alg) const;
        /**
         * @brief Get DH algorithm of the suite
         * @param alg Algorithm
         * @return Operation status
         */
        common::Error dhAlgorithm(CryptAlgorithmConstP& alg) const;
        /**
         * @brief Get ECDH algorithm of the suite
         * @param alg Algorithm
         * @return Operation status
         */
        common::Error ecdhAlgorithm(CryptAlgorithmConstP& alg) const;
        /**
         * @brief Get digital signature algorithm of the suite
         * @param alg Algorithm
         * @return Operation status
         */
        common::Error signatureAlgorithm(CryptAlgorithmConstP& alg) const;

        //! Create symmetric cipher encryptor
        common::SharedPtr<SEncryptor> createSEncryptor(common::Error& ec) const;
        //! Create symmetric cipher decryptor
        common::SharedPtr<SDecryptor> createSDecryptor(common::Error& ec) const;
        //! Create AEAD encryptor
        common::SharedPtr<AEADEncryptor> createAeadEncryptor(common::Error& ec) const;
        //! Create AEAD decryptor
        common::SharedPtr<AEADDecryptor> createAeadDecryptor(common::Error& ec) const;
        //! Create digest processor
        common::SharedPtr<Digest> createDigest(common::Error& ec) const;
        //! Create MAC processor
        common::SharedPtr<MAC> createMAC(common::Error& ec) const;
        /**
         * @brief Create PBKDF processor
         * @param targetKeyAlg Algorithm the derived keys will be used with
         * @return PBKDF processor
         */
        common::SharedPtr<PBKDF> createPBKDF(common::Error& ec,const CryptAlgorithm* targetKeyAlg=nullptr) const;
        /**
         * @brief Create HKDF processor
         * @param targetKeyAlg Algorithm the derived keys will be used with
         * @return HKDF processor
         */
        common::SharedPtr<HKDF> createHKDF(common::Error& ec,const CryptAlgorithm* targetKeyAlg=nullptr) const;
        //! Create DH processor
        common::SharedPtr<DH> createDH(common::Error& ec) const;
        //! Create ECDH processor
        common::SharedPtr<ECDH> createECDH(common::Error& ec) const;
        //! Create signature sign processor
        common::SharedPtr<SignatureSign> createSignatureSign(common::Error& ec) const;
        //! Create signature verify processor
        common::SharedPtr<SignatureVerify> createSignatureVerify(common::Error& ec) const;

        //! Create passphrase key
        common::SharedPtr<PassphraseKey> createPassphraseKey(common::Error& ec,const CryptAlgorithm* targetKeyAlg=nullptr) const;

        /**
         * @brief Create X.509 certificate
         * @return X.509 certificate
         */
        common::SharedPtr<X509Certificate> createX509Certificate(common::Error& ec) const;
        /**
         * @brief Create X.509 certificate chain
         * @return X.509 certificate chain
         */
        common::SharedPtr<X509CertificateChain> createX509CertificateChain(common::Error& ec) const;
        /**
         * @brief Create X.509 certificate store
         * @return X.509 certificate store
         */
        common::SharedPtr<X509CertificateStore> createX509CertificateStore(common::Error& ec) const;

    private:

        //! Clear container with preserialized suite
        void clearRawStorage();
        //! Prepare container with preserialized suite
        common::Error prepareRawStorage();

        template <typename FieldT>
        common::Error findAlgorithm(
            const FieldT& field,
            CryptAlgorithmConstP& alg,
            CryptAlgorithmConstP& algCache,
            CryptAlgorithm::Type type
        ) const;

        common::SharedPtr<cipher_suite::shared_traits::managed> m_suite;

        mutable const CryptAlgorithm* m_cipherAlgorithm;
        mutable const CryptAlgorithm* m_digestAlgorithm;
        mutable const CryptAlgorithm* m_aeadAlgorithm;
        mutable const CryptAlgorithm* m_macAlgorithm;
        mutable const CryptAlgorithm* m_hkdfAlgorithm;
        mutable const CryptAlgorithm* m_pbkdfAlgorithm;
        mutable const CryptAlgorithm* m_dhAlgorithm;
        mutable const CryptAlgorithm* m_ecdhAlgorithm;
        mutable const CryptAlgorithm* m_signatureAlgorithm;
};

//! Set of sipher suites
class HATN_CRYPT_EXPORT CipherSuites : public common::Singleton
{
    public:

        HATN_SINGLETON_DECLARE()

        //! Ctor
        CipherSuites()=default;
        //! Dtor
        ~CipherSuites()=default;

        CipherSuites(const CipherSuites&)=delete;
        CipherSuites(CipherSuites&&) =delete;
        CipherSuites& operator=(const CipherSuites&)=delete;
        CipherSuites& operator=(CipherSuites&&) =delete;

        //! Signleton instance
        static CipherSuites& instance() noexcept;

        //! Reset the set
        void reset() noexcept;

        static void free() noexcept
        {
            instance().reset();
        }

        /**
         * @brief Set default suite
         * @param suite Default suite
         *
         * Default suite is used when no specific suite for algorithm is defined
         */
        void setDefaultSuite(std::shared_ptr<CipherSuite> suite) noexcept;
        //! Get default suite
        const CipherSuite* defaultSuite() const noexcept;

        /**
         * @brief Add suite to set
         * @param suite Suite
         */
        void addSuite(std::shared_ptr<CipherSuite> suite);

        /**
         * @brief Find suite
         * @param id Suite's ID
         * @return Found suite or nullptr
         */
        const CipherSuite* suite(const CipherSuite::IdType& id) const noexcept;
        const CipherSuite* suite(const char* id,size_t idLength) const noexcept
        {
            CipherSuite::IdType key(id,idLength,true);
            return suite(key);
        }

        /**
         * @brief Overloaded method to find suite
         * @param id Suite's ID
         * @return
         */
        const CipherSuite* suite(const char* id) const noexcept
        {
            return suite(id,strlen(id));
        }

        /**
         * @brief Set default crypt engine
         * @param engine Default crypt engine
         */
        void setDefaultEngine(std::shared_ptr<CryptEngine> engine);

        //! Get default crypt engine
        CryptEngine* defaultEngine() const noexcept;

        CryptPlugin* defaultPlugin() const noexcept
        {
            if (defaultEngine())
            {
                return defaultEngine()->plugin();
            }
            return nullptr;
        }

        RandomGenerator* defaultRandomGenerator() const noexcept
        {
            return m_randomGenerator.get();
        }

        /**
         * @brief Load crypt engines to the set
         * @param engines Crypt engines
         */
        void loadEngines(std::map<CryptAlgorithmTypeNameMapKey,std::shared_ptr<CryptEngine>> engines);

        std::map<CryptAlgorithmTypeNameMapKey,std::shared_ptr<CryptEngine>> engines() const noexcept;

        /**
         * @brief Find engine for crypt algorithm
         * @param type Algorithm type
         * @param name Algorithm name
         * @param nameLength Length of algorithm name
         * @param fallBackDefault Return default engine if specific engine is not found
         * @return Found engine or default engine if no engine was found and fallBackDefault is set
         */
        CryptEngine* engineForAlgorithm(
            CryptAlgorithm::Type type,
            const char* name=nullptr,
            size_t nameLength=0,
            bool fallBackDefault=true
        ) const noexcept;

    private:

        std::map<CipherSuite::IdType,std::shared_ptr<CipherSuite>> m_suites;
        std::shared_ptr<CipherSuite> m_defaultSuite;

        std::map<CryptAlgorithmTypeNameMapKey,std::shared_ptr<CryptEngine>> m_engines;
        std::shared_ptr<CryptEngine> m_defaultEngine;

        common::SharedPtr<RandomGenerator> m_randomGenerator;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTCIPHERSUITE_H
