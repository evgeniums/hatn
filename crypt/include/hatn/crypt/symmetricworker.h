/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/symmetricworker.h
 *
 *      Base classes for implementation of symmetric encryption
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTSYMMETRICWORKER_H
#define HATNCRYPTSYMMETRICWORKER_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/cipherworker.h>
#include <hatn/crypt/securekey.h>

HATN_CRYPT_NAMESPACE_BEGIN

/********************** SymmetricWorker **********************************/

//! Base class for symmetric encryptors and decryptors
template <bool Encrypt>
class SymmetricWorker : public CipherWorker
{
    public:

        /**
         * @brief Ctor
         * @param key Symmetric key to be used for encryption/decryption
         *
         * Worker will use encryption algotithm as set in encryption key.
         *
         * @throws std::runtime_error if key doesn't have role ENCRYPT_SYMMETRIC
         */
        SymmetricWorker(const SymmetricKey* key);

        SymmetricWorker():SymmetricWorker(nullptr)
        {}

        /**
         * @brief Set encryption key
         * @param key Key
         * @param updateInherited Update key in inherited class
         *
         * @throws ErrorException if the key can not be set
         */
        void setKey(const SymmetricKey* key, bool updateInherited=true);

        //! Get encryption key
        inline const SymmetricKey* key() const noexcept
        {
            return m_key;
        }

        //! Set cryptographic algorithm
        inline void setAlg(const CryptAlgorithm* alg)
        {
            if (m_key!=nullptr && alg!=nullptr)
            {
                Assert(m_key->alg()==alg,"Invalid crypt algorithm");
            }
            m_alg=alg;
        }

        //! Get cryptographic algorithm
        inline const CryptAlgorithm* alg() const noexcept
        {
            return m_alg;
        }

        /**
         * @brief Init encryptor/decryptor
         * @param iv Initialization vector. If IV less than required then it will be padded by zeros.
         * @return Operation status
         */
        template <typename ContainerIvT>
        common::Error init(const ContainerIvT& iv);

        common::Error init(const common::SpanBuffer& iv);

        size_t maxPadding() const noexcept;

        /**
        * @brief Do whole processing of the data: generate/extract IV, init and procesAndFinalize
        * @param dataIn Buffer with input data
        * @param dataOut Buffer for output result
        * @param offsetIn Offset in input buffer for processing
        * @param offsetOut Offset in output buffer starting from which to put result
        * @return Operation status
         */
        template <typename ContainerInT,typename ContainerOutT>
        common::Error runPack(
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
        );

        //! Overloaded runPack
        template <typename ContainerOutT>
        common::Error runPack(
            const common::SpanBuffers& dataIn,
            ContainerOutT& dataOut,
            size_t offsetOut=0
        );

        //! Get block size for this encryption algorithm
        virtual size_t blockSize() const noexcept
        {
            return alg()->blockSize();
        }
        //! Get IV size for this encryption algorithm
        virtual size_t ivSize() const noexcept
        {
            return alg()->ivSize();
        }

        //! Generate IV
        template <typename ContainerT>
        common::Error generateIV(ContainerT& iv, size_t offset=0) const;

        template <typename ContainerOutT, typename CipherTextT>
        common::Error run(
            const common::SpanBuffer& iv,
            const CipherTextT& ciphertext,
            ContainerOutT& plaintext
        )
        {
            plaintext.clear();
            HATN_CHECK_RETURN(init(iv))
            return processAndFinalize(ciphertext,plaintext);
        }

        virtual const CryptAlgorithm* getAlg() const override
        {
            return alg();
        }

        virtual size_t getMaxPadding() const noexcept override
        {
            return maxPadding();
        }

        void setImplicitKeyMode(bool enable) noexcept
        {
            m_implicitKeyMode=enable;
        }

        bool isImplicitkeyMode() const noexcept
        {
            return m_implicitKeyMode;
        }

        Error checkAlg() const noexcept
        {
            if (m_alg==nullptr)
            {
                return cryptError(CryptError::INVALID_ALGORITHM);
            }
            return OK;
        }

    protected:

        virtual Error canProcessAndFinalize() const noexcept override
        {
            if (!m_implicitKeyMode)
            {
                if (key()->alg()->isType(CryptAlgorithm::Type::AEAD))
                {
                    return cryptError(CryptError::INVALID_OPERATION);
                }
            }
            return OK;
        }

        virtual common::Error doGenerateIV(char* ivData, size_t* size=nullptr) const =0;

        /**
         * @brief Init encryptor/decryptor
         * @param initVector Initialization data, usually it is IV
         * @return Operation status
         */
        virtual common::Error doInit(const char* initVector, size_t size=0) =0;

        /**
         * @brief Update encryption key in derived class
         *
         * @throws ErrorException if the key can not be updated
         */
        virtual void doUpdateKey()
        {}

    private:

        const SymmetricKey* m_key;
        const CryptAlgorithm* m_alg;
        bool m_implicitKeyMode;
};

class HATN_CRYPT_EXPORT SEncryptor : public SymmetricWorker<true>
{
    public:

        using SymmetricWorker<true>::SymmetricWorker;

        /**
         * @brief Init cipher in stream mode, IV will be auto generated and prepended.
         * @param ciphertext Buffer for result.
         * @param offset Offset in result buffer.
         * @return Operation status.
         */
        template <typename ContainerT>
        common::Error initStream(
            ContainerT& result,
            size_t offset=0
        );

        /**
         * @brief Encrypt data in stream mode.
         * @param plaintext Input data.
         * @param ciphertext Result container.
         * @param offset Offset in result container.
         * @return Operation status.
         */
        template <typename BufferT, typename ContainerT>
        common::Error encryptStream(
            const BufferT& plaintext,
            ContainerT& ciphertext,
            size_t offset=0
        );

        size_t streamPrefixSize() const noexcept
        {
            return alg()->ivSize();
        }
};

class HATN_CRYPT_EXPORT SDecryptor : public SymmetricWorker<false>
{
    public:

        using SymmetricWorker<false>::SymmetricWorker;

        /**
             * @brief Init cipher in stream mode, IV is expected in the beginning of ciphertext.
             * @param ciphertext Buffer containing IV.
             * @param offset Offset in buffer.
             * @return Offset of data in ciphertext to use in decryptStream().
        */
        template <typename ContainerT>
        Result<size_t> initStream(
            const ContainerT& ciphertext,
            size_t offset=0
        );

        /**
             * @brief Decrypt data in stream mode.
             * @param ciphertext Input data.
             * @param plaintext Result container.
             * @param offset Offset in result container.
             * @return Operation status.
        */
        template <typename BufferT, typename ContainerT>
        common::Error decryptStream(
            const BufferT& ciphertext,
            ContainerT& plaintext,
            size_t offset=0
        );

        size_t streamPrefixSize() const noexcept
        {
            return alg()->ivSize();
        }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTSYMMETRICWORKER_H
