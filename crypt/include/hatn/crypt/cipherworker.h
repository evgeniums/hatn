/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/cipherworker.h
 *
 *      Base classes for implementation of symmetric encryption
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTCIPHERWORKER_H
#define HATNCRYPTCIPHERWORKER_H

#include <type_traits>

#include <hatn/common/bytearray.h>
#include <hatn/common/spanbuffer.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/securekey.h>

HATN_CRYPT_NAMESPACE_BEGIN

/********************** CipherWorker **********************************/

//! Base template class for symmetric encryptors and decryptors
template <bool Encrypt>
class CipherWorker
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
        CipherWorker(const SymmetricKey* key);

        CipherWorker():CipherWorker(nullptr)
        {}

        //! Dtor
        virtual ~CipherWorker()=default;

        CipherWorker(const CipherWorker&)=delete;
        CipherWorker(CipherWorker&&) =default;
        CipherWorker& operator=(const CipherWorker&)=delete;
        CipherWorker& operator=(CipherWorker&&) =default;

        /**
         * @brief Set encryption key
         * @param key Key
         * @param updateInherited Update key in inherited class
         *
         * @throws ErrorException if the key can not be set
         */
        void setKey(const SymmetricKey* key, bool updateInherited=true);

        /**
         * @brief Init encryptor/decryptor
         * @param iv Initialization vector. If IV less than required then it will be padded by zeros.
         * @return Operation status
         */
        template <typename ContainerIvT>
        common::Error init(const ContainerIvT& iv);

        common::Error init(const common::SpanBuffer& iv);

        //! Reset worker so that it can be used again with new data
        void reset() noexcept;

        size_t maxPadding() const noexcept;

        /**
         * @brief Process block of data
         * @param dataIn Buffer with input data
         * @param dataOut Buffer for output result
         * @param sizeOut Resulting size
         * @param sizeIn Input size to process
         * @param offsetIn Offset in input buffer for processing
         * @param offsetOut Offset in output buffer starting from which to put result
         * @param lastBlock Finalize processing
         * @param noResize Do not resize output container, take it as is
         * @return Operation status
         */
        template <typename ContainerInT, typename ContainerOutT>
        common::Error process(
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t& sizeOut,
            size_t sizeIn=0,
            size_t offsetIn=0,
            size_t offsetOut=0,
            bool lastBlock=false,
            bool noResize=false
        );

        /**
         * @brief Process last block of data and finalize processing
         * @param dataOut Buffer for output result
         * @param sizeOut Resulting size
         * @param offsetOut Offset in output buffer starting from which to put result
         * @return Operation status
         */
        template <typename ContainerOutT>
        common::Error finalize(
            ContainerOutT& dataOut,
            size_t& sizeOut,
            size_t offsetOut=0
        );

        /**
        * @brief Process the whole data buffer and finalize result
        * @param dataIn Buffer with input data
        * @param dataOut Buffer for output result
        * @param offsetIn Offset in input buffer for processing
        * @param offsetOut Offset in output buffer starting from which to put result
        * @param aeadOffset Offset in input buffer up to which do not encrypt data but only authentificate in AEAD mode
        * @return Operation status
        **/
        template <typename ContainerInT, typename ContainerOutT>
        common::Error processAndFinalize(
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
        );

        template <typename ContainerOutT>
        common::Error processAndFinalize(
            const common::SpanBuffer& dataIn,
            ContainerOutT& dataOut,
            size_t offsetOut=0
        );
        template <typename ContainerOutT>
        common::Error processAndFinalize(
            const common::SpanBuffers& dataIn,
            ContainerOutT& dataOut,
            size_t offsetOut=0
        );

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

        //! Get encryption key
        inline const SymmetricKey* key() const noexcept
        {
            return m_key;
        }

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

    protected:

        /**
         * @brief Actually process data in derived class
         * @param bufIn Input buffer
         * @param sizeIn Size of input data
         * @param bufOut Output buffer
         * @param sizeOut Resulting size
         * @param lastBlock Finalize processing
         * @return Operation status
         */
        virtual common::Error doProcess(
            const char* bufIn,
            size_t sizeIn,
            char* bufOut,
            size_t& sizeOut,
            bool lastBlock
        ) = 0;

        virtual common::Error doGenerateIV(char* ivData) const =0;

        /**
         * @brief Init encryptor/decryptor
         * @param initVector Initialization data, usually it is IV
         * @return Operation status
         */
        virtual common::Error doInit(const char* initVector) =0;

        //! Reset worker so that it can be used again with new data
        virtual void doReset() noexcept =0;

        /**
         * @brief Update encryption key in derived class
         *
         * @throws ErrorException if the key can not be updated
         */
        virtual void doUpdateKey()
        {}

    private:

        const SymmetricKey* m_key;
        bool m_initialized;
        const CryptAlgorithm* m_alg;
};

class HATN_CRYPT_EXPORT SEncryptor : public CipherWorker<true>
{
    public:

        using CipherWorker<true>::CipherWorker;

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

class HATN_CRYPT_EXPORT SDecryptor : public CipherWorker<false>
{
    public:

        using CipherWorker<false>::CipherWorker;

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

#endif // HATNCRYPTCIPHERWORKER_H
