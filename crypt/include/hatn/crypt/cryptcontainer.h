/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/cryptcontainer.h
 *
 *      Processor (encryptor/decryptor) of cryptographic (encrypted) containers
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTCRYPCONTAINER_H
#define HATNCRYPTCRYPCONTAINER_H

#include <boost/endian/conversion.hpp>

#include <hatn/common/spanbuffer.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/cryptalgorithm.h>
#include <hatn/crypt/aead.h>
#include <hatn/crypt/cipherworker.h>
#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/passphrasekey.h>
#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/cryptcontainerheader.h>

#include <hatn/crypt/cryptdataunits.h>

HATN_CRYPT_NAMESPACE_BEGIN

/**
 * @brief Processor (encryptor/decryptor) of cryptographic (encrypted) containers
 */
class HATN_CRYPT_EXPORT CryptContainer
{
    public:

        /**
         * @brief Ctor
         * @param factory Allocator factory
         */
        explicit CryptContainer(
                common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) noexcept : CryptContainer(nullptr,nullptr,factory)
        {}

        /**
         * @brief Ctor
         * @param suite Cipher suite
         * @param factory Allocator factory
         */
        explicit CryptContainer(
                const CipherSuite* suite,
                common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) noexcept : CryptContainer(nullptr,suite,factory)
        {}

        /**
         * @brief CryptContainer
         * @param masterKey Master key used to encrypt/decrypt container
         * @param suite Cipher suite
         * @param factory Allocator factory
         */
        explicit CryptContainer(
            const SymmetricKey* masterKey,
            const CipherSuite* suite=nullptr,
            common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) noexcept;

        ~CryptContainer()=default;
        CryptContainer(const CryptContainer&)=delete;
        CryptContainer(CryptContainer&&) =default;
        CryptContainer& operator=(const CryptContainer&)=delete;
        CryptContainer& operator=(CryptContainer&&) =default;

        /**
         * @brief Set master key
         * @param key Master key
         */
        inline void setMasterKey(const SymmetricKey* key) noexcept;
        //! Get master key
        inline const SymmetricKey* masterKey() const noexcept;

        /**
         * @brief Set type of key derivation function
         * @param type KDF type
         */
        inline void setKdfType(container_descriptor::KdfType type) noexcept;
        //! Get type of key derivation function
        inline container_descriptor::KdfType kdfType() const noexcept;

        /**
         * @brief Set salt to be used in KDF for derivation of keys
         * @param salt Salt
         */
        inline void setSalt(const common::ConstDataBuf& salt);
        //! Get salt to be used in KDF for derivation of keys
        inline common::ConstDataBuf salt() const noexcept;

        /**
         * @brief Set maximum size of plaintext data per normal chunk in container
         * @param size Maximum size of plaintext data per chunk
         *
         * If size is zero (default) then the chunk's size has no limits
         */
        inline void setChunkMaxSize(uint32_t size) noexcept;
        //! Get maximum size of plaintext data per normal chunk in container
        inline uint32_t chunkMaxSize() const noexcept;

        /**
         * @brief Set maximum size of plaintext data for the first chunk of container
         * @param size Maximum size of plaintext data in the first chunk
         *
         * If size is zero then max plaintext size of the first size will be the same as plaintext size of a noral chunk
         */
        inline void setFirstChunkMaxSize(uint32_t size) noexcept;
        //! Get maximum size of plaintext data for the first chunk of container
        inline uint32_t firstChunkMaxSize() const noexcept;

        /**
         * @brief Get maximum packed size of a chunk
         * @param seqnum Sequential number of the chunk
         * @param containerSize Size of container
         * @return Packed size of the chunk
         *
         * @throws ErrorException if can not create decryptor
         */
        inline uint32_t maxPackedChunkSize(uint32_t seqnum, uint32_t containerSize) const;

        inline uint32_t packedExtraSize() const;

        /**
         * @brief Get maximum packed size of a chunk
         * @param seqnum Sequential number of the chunk
         * @return Packed size of the chunk
         *
         * @throws ErrorException if can not create decryptor
         */
        inline uint32_t maxPackedChunkSize(uint32_t seqnum) const;

        /**
         * @brief Get maximum plain size of a chunk
         * @param seqnum Sequential number of the chunk
         * @return Plain size of the chunk
         */
        inline uint32_t maxPlainChunkSize(uint32_t seqnum) const;

        /**
         * @brief Set cipher suite to be used for packing the container
         * @param suite Cipher suite
         *
         * This suite can be also used for unpacking when there is no suite or suite_id defined in container's descriptor
         */
        inline void setCipherSuite(const CipherSuite* suite) noexcept;
        //! Get cipher suite to be used for packing the container
        inline const CipherSuite* cipherSuite() const noexcept;

        /**
         * @brief Enable attaching full descriptor of cipher suite to container's descriptor
         * @param enable Flag enable/disable
         *
         * When attaching is enabled then the whole suite's descriptor will be serialized with container's descriptor.
         * Normally, only suite ID is attached to container's descriptor.
         */
        inline void setAttachCipherSuiteEnabled(bool enable) noexcept;
        //! Check if cipher suite attaching is enabled
        inline bool isAttachCipherSuiteEnabled() const noexcept;

        void setAutoSalt(bool enable) noexcept
        {
            m_autoSalt=enable;
        }

        bool autoSalt() const noexcept
        {
            return m_autoSalt;
        }

        void setStreamingMode(bool enable) noexcept
        {
            m_streamingMode=enable;
        }

        bool isStreamingMode() const noexcept
        {
            return m_streamingMode;
        }

        /**
         * @brief Pack header to container
         * @param result Target container
         * @param descriptorSize Size of descriptor
         * @param offsetOut Offset in the target container
         * @return Operation status
         *
         */
        template <typename ContainerT>
        common::Error packHeader(
            ContainerT& result,
            uint16_t descriptorSize,
            uint64_t contentSize=0,
            size_t offsetOut=0
        );

        inline common::Error packHeader(
            char* headerPtr,
            uint16_t descriptorSize,
            uint64_t contentSize=0
        ) noexcept;

        inline void setCiphertextSize(
            char* headerPtr,
            uint64_t size
        ) noexcept;
        inline uint64_t cipherTextSize(const char* headerPtr) const noexcept;

        constexpr static size_t headerSize() noexcept
        {
            return CryptContainerHeader::size();
        }

        /**
         * @brief Pack header and descriptor to container
         * @param result Target container
         * @param offsetOut Offset in the target container
         * @return Operation status
         *
         */
        template <typename ContainerT>
        common::Error packHeaderAndDescriptor(
            ContainerT& result,
            uint64_t contentSize=0,
            size_t offsetOut=0
        );

        /**
         * @brief Unpack header from container
         * @param container Source container
         * @param plaintextSize Size of plaintext
         * @param descriptorSize Size of descriptor
         * @param ciphertextSize Size of ciphertext
         * @return Operation status
         *
         */
        template <typename ContainerT>
        common::Error unpackHeader(
            const ContainerT& container,
            uint64_t& plaintextSize,
            uint16_t& descriptorSize,
            uint64_t& ciphertextSize
        );

        /**
         * @brief Unpack header and descriptor from container
         * @param container Source container
         * @param plaintextSize Size of plaintext
         * @param ciphertextSize Size of ciphertext
         * @param consumedSize Size consumed by the header in source container
         * @return Operation status
         *
         * After unpacking container's descriptor the cipher suite will be looked for by ID or constructed directly if the suite's descriptor
         * was attached to container's descriptor.
         */
        template <typename ContainerT>
        common::Error unpackHeaderAndDescriptor(
            const ContainerT& container,
            uint64_t& plaintextSize,
            uint64_t& ciphertextSize,
            size_t& consumedSize
        );

        /**
         * @brief Pack descriptor
         * @param result Target container
         * @param offsetOut Offset in the target container
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error packDescriptor(
            ContainerT& result,
            size_t offsetOut=0
        );

        /**
         * @brief Unpack header from container
         * @param container Source container
         * @param unpackInline Unpack header inline without copying the unerlying contents of string and byte fields
         * @return Operation status
         *
         * After unpacking container's descriptor the cipher suite will be looked for by ID or constructed directly if the suite's descriptor
         * was attached to container's descriptor.
         */
        template <typename ContainerT>                
        common::Error unpackDescriptor(
            const ContainerT& container
        );

        /**
         * @brief Pack (encrypt) container
         * @param plaintext Input plaintext
         * @param result Target container
         * @param salt Salt for key derivation
         * @return Operation status
         */
        template <typename ContainerInT, typename ContainerOutT>
        common::Error pack(
            const ContainerInT& plaintext,
            ContainerOutT& result,
            const common::ConstDataBuf& salt=common::ConstDataBuf()
        );

        /**
         * @brief Unpack (decrypt) container
         * @param input Input ciphertext
         * @param plaintext Target container
         * @return Operation status
         */
        template <typename ContainerInT, typename ContainerOutT>
        common::Error unpack(
            const ContainerInT& input,
            ContainerOutT& plaintext
        );

        /**
         * @brief Pack a data chunk
         * @param plaintext Data chunk with plaintext. can be either SpanBuffer or SpanBuffers
         * @param result Target container
         * @param info Extra information to use for key derivation in HKDF mode
         * @param offsetOut Offset in target container
         * @return Operation status
         */
        template <typename BufferT, typename ContainerOutT>
        common::Error packChunk(
            const BufferT& plaintext,
            ContainerOutT& result,
            const common::ConstDataBuf& info=common::ConstDataBuf(),
            size_t offsetOut=0
        );

        /**
         * @brief Pack first data chunk
         * @param plaintext Data chunk with plaintext. can be either SpanBuffer or SpanBuffers
         * @param result Target container
         * @param info Extra information to use for key derivation in HKDF mode
         * @param offsetOut Offset in target container
         * @return Operation status
         */
        template <typename BufferT, typename ContainerOutT>
        common::Error packFirstChunk(
            const BufferT& plaintext,
            ContainerOutT& result,
            const common::ConstDataBuf& info=common::ConstDataBuf(),
            size_t offsetOut=0
        );

        /**
         * @brief Pack a data chunk
         * @param plaintext Data chunk with plaintext, can be either SpanBuffer or SpanBuffers
         * @param result Target container
         * @param seqnum Sequential number of the chunk, the number is used in HKDF as info for key derivation
         * @param offsetOut Offset in target container
         * @return Operation status
         */
        template <typename BufferT, typename ContainerOutT>
        common::Error packChunk(
            const BufferT& plaintext,
            ContainerOutT& result,
            uint32_t seqnum,
            size_t offsetOut=0
        );

        /**
         * @brief Unpack a data chunk
         * @param ciphertext Encrypted data, can be either SpanBuffer or SpanBuffers
         * @param result Target container
         * @param seqnum Sequential number of the chunk, the number is used in HKDF as info for key derivation
         * @return Operation status
         */
        template <typename BufferT, typename ContainerOutT>
        common::Error unpackChunk(
            const BufferT& ciphertext,
            ContainerOutT& result,
            const common::ConstDataBuf& info=common::ConstDataBuf()
        );

        /**
         * @brief Unpack a data chunk
         * @param ciphertext Encrypted data, can be either SpanBuffer or SpanBuffers
         * @param result Target container
         * @param info Extra information to use for key derivation in HKDF mode
         * @return Operation status
         */
        template <typename BufferT, typename ContainerOutT>
        common::Error unpackChunk(
            const BufferT& ciphertext,
            ContainerOutT& result,
            uint32_t seqnum
        );

        //! Check current state of container's processor
        inline common::Error checkState() const noexcept;

        //! Get allocator factory
        inline common::pmr::AllocatorFactory* factory() const noexcept;

        //! Derive encryption key for the chunk
        common::Error deriveKey(
            SymmetricKeyConstPtr& key,
            common::SharedPtr<SymmetricKey>& derivedKey,
            const common::ConstDataBuf& info,
            const CryptAlgorithm* alg=nullptr
        );

        /**
         * @brief Reset container's processor
         * @param withDescriptor Reset with descriptor including cipher suite, salt and kdf parameters
         */
        void reset(bool withDescriptor=false);

        /**
         * @brief Reset container's processor and destroy processing objects
         * @param withDescriptor Reset with descriptor including cipher suite, salt and kdf parameters
         */
        void hardReset(bool withDescriptor=true);

        Result<size_t> streamPrefixSize() const;

        template <typename ContainerOutT>
        common::Error initStreamEncryptor(
            ContainerOutT& ciphertext,
            uint32_t seqnum,
            size_t offset=0
        );

        template <typename ContainerOutT>
        common::Error initStreamEncryptor(
            ContainerOutT& ciphertext,
            const common::ConstDataBuf& info,
            size_t offset=0
        );

        template <typename BufferT, typename ContainerOutT>
        common::Error encryptStream(
            const BufferT& plaintext,
            ContainerOutT& ciphertext,
            size_t offseOut=0
        );

        template <typename BufferT>
        Result<size_t> initStreamDecryptor(
            const BufferT& ciphertext,
            uint32_t seqnum,
            size_t offsetOut=0
        );

        template <typename BufferT>
        Result<size_t> initStreamDecryptor(
            const BufferT& ciphertext,
            const common::ConstDataBuf& info,
            size_t offsetOut=0
        );

        template <typename BufferT, typename ContainerOutT>
        common::Error decryptStream(
            const BufferT& ciphertext,
            ContainerOutT& plaintext,
            size_t offsetOut=0
        );

    private:

        inline common::Error checkOrCreateDecryptor();

        const SymmetricKey* m_masterKey;
        const SymmetricKey* m_encryptionKey;
        common::SharedPtr<SymmetricKey> m_encryptionKeyHolder;
        const CryptAlgorithm* m_aeadAlg;

        common::SharedPtr<PBKDF> m_pbkdf;
        common::SharedPtr<HKDF> m_hkdf;

        common::SharedPtr<AEADEncryptor> m_enc;
        common::SharedPtr<AEADDecryptor> m_dec;

        const CryptAlgorithm* m_streamAlg;
        common::SharedPtr<SEncryptor> m_streamEnc;
        common::SharedPtr<SDecryptor> m_streamDec;
        common::SharedPtr<SymmetricKey> m_streamKeyHolder;

        const CipherSuite* m_cipherSuite;
        container_descriptor::shared_traits::type m_descriptor;

        bool m_attachSuite;
        CipherSuite m_extractedSuite;

        common::pmr::AllocatorFactory* m_factory;

        mutable lib::optional<uint32_t> m_maxPackedChunkSize;
        mutable lib::optional<uint32_t> m_maxPackedFirstChunkSize;

        bool m_autoSalt;
        bool m_streamingMode;
};

HATN_CRYPT_NAMESPACE_END

#include <hatn/crypt/cryptcontainer.ipp>

#endif // HATNCRYPTCRYPCONTAINER_H
