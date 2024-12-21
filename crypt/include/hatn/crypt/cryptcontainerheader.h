/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/cryptcontinerheader.h
 *
 *      Header of encrypted container
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTCONTAINNERHEADER_H
#define HATNCRYPTCONTAINNERHEADER_H

#include <boost/endian/conversion.hpp>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/ciphersuite.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Header of crypt container
class CryptContainerHeader
{
    public:

        constexpr static const uint8_t VERSION=0x01u;
        constexpr static const char* PREFIX="HCC"; // stands for "Hatn Crypt Container"
        constexpr static const char* STREAM_PREFIX="HCS"; // stands for "Hatn Crypt Stream"

        constexpr static const size_t PREFIX_LENGTH=3;
        constexpr static const size_t VERSION_OFFSET=PREFIX_LENGTH; // 3
        constexpr static const size_t DESCRIPTOR_SIZE_OFFSET=VERSION_OFFSET+sizeof(uint8_t); // 4
        constexpr static const size_t PLAINTEXT_SIZE_OFFSET=DESCRIPTOR_SIZE_OFFSET+sizeof(uint16_t); // 6
        constexpr static const size_t CIPHERTEXT_SIZE_OFFSET=PLAINTEXT_SIZE_OFFSET+sizeof(uint64_t); // 14

        /**
         * @brief Constructor from external buffer.
         * @param externalBuf Pointer to external buffer
         */
        template <typename BufT>
        CryptContainerHeader(BufT* externalBuf) noexcept : m_ptr(const_cast<char*>(externalBuf))
        {}

        /**
         * @brief Constructor with parameters.
         * @param descriptorSize Size of descriptor.
         * @param version Version of container format.
         */
        CryptContainerHeader(size_t descriptorSize,
                             bool streaming=false,
                             uint8_t version=VERSION) noexcept : m_ptr(&m_data[0])
        {
            reset(streaming,version);
            setDescriptorSize(descriptorSize);
        }

        //! Default ctor
        CryptContainerHeader():m_ptr(&m_data[0])
        {
            reset();
        }

        ~CryptContainerHeader()=default;

        //! Copy ctor
        CryptContainerHeader(const CryptContainerHeader& other) noexcept :
            m_ptr(other.m_ptr)
        {
            std::copy(&other.m_data[0],&other.m_data[0]+other.size(),&m_data[0]);
        }
        //! Copy operator
        CryptContainerHeader& operator=(const CryptContainerHeader& other) noexcept
        {
            if (&other!=this)
            {
                m_ptr=other.m_ptr;
                std::copy(&other.m_data[0],&other.m_data[0]+other.size(),&m_data[0]);
            }
            return *this;
        }

        //! Move ctor
        CryptContainerHeader(CryptContainerHeader&& other) noexcept :
            m_ptr(other.m_ptr)
        {
            std::copy(&other.m_data[0],&other.m_data[0]+other.size(),&m_data[0]);
            other.reset();
        }
        //! Move operator
        CryptContainerHeader& operator=(CryptContainerHeader&& other) noexcept
        {
            if (&other!=this)
            {
                m_ptr=other.m_ptr;
                std::copy(&other.m_data[0],&other.m_data[0]+other.size(),&m_data[0]);
                other.reset();
            }
            return *this;
        }

        /**
         * @brief Set container's format version
         * @param version Version
         */
        void setVersion(uint8_t version) noexcept
        {
            *(data()+VERSION_OFFSET)=version;
        }
        //! Get container's format version
        uint8_t version() const noexcept
        {
            return *(data()+VERSION_OFFSET);
        }

        //! Check container prefix
        bool checkPrefix() const noexcept
        {
            return memcmp(data(),PREFIX,PREFIX_LENGTH)==0 || memcmp(data(),STREAM_PREFIX,PREFIX_LENGTH)==0;
        }

        //! Set size of descriptor
        void setDescriptorSize(size_t val) noexcept
        {
            uint16_t size=static_cast<uint16_t>(val);
            boost::endian::native_to_little_inplace(size);
            memcpy(data()+DESCRIPTOR_SIZE_OFFSET,&size,sizeof(size));
        }
        //! Get size of descriptor
        size_t descriptorSize() const noexcept
        {
            uint16_t size=0;
            memcpy(&size,data()+DESCRIPTOR_SIZE_OFFSET,sizeof(size));
            boost::endian::little_to_native_inplace(size);
            return static_cast<size_t>(size);
        }

        //! Set size of plaintext
        void setPlaintextSize(uint64_t size) noexcept
        {
            contentSizeForStorageInplace(size);
            memcpy(data()+PLAINTEXT_SIZE_OFFSET,&size,sizeof(size));
        }
        //! Get size of plaintext
        uint64_t plaintextSize() const noexcept
        {
            uint64_t size=0;
            memcpy(&size,data()+PLAINTEXT_SIZE_OFFSET,sizeof(size));
            boost::endian::little_to_native_inplace(size);
            return size;
        }

        //! Set size of plaintext
        void setCiphertextSize(uint64_t size) noexcept
        {
            contentSizeForStorageInplace(size);
            memcpy(data()+CIPHERTEXT_SIZE_OFFSET,&size,sizeof(size));
        }
        //! Get size of ciphertext
        uint64_t ciphertextSize() const noexcept
        {
            uint64_t size=0;
            memcpy(&size,data()+CIPHERTEXT_SIZE_OFFSET,sizeof(size));
            boost::endian::little_to_native_inplace(size);
            return size;
        }

        //! Get header size
        constexpr static size_t size() noexcept
        {
            return sizeof(m_data);
        }
        //! Get pointer to header buffer
        char* data() const noexcept
        {
            return m_ptr;
        }

        /**
         * @brief Set to external buffer
         * @param externalBuf Pointer to external buffer
         */
        template <typename BufT>
        void setExternalBuf(BufT* externalBuf)
        {
            if (externalBuf!=nullptr)
            {
                m_ptr=const_cast<char*>(externalBuf);
            }
            else
            {
                m_ptr=&m_data[0];
                reset();
            }
        }

        //! Reset header
        void reset(bool streaming=false, uint8_t version=VERSION)
        {
            memset(m_ptr,0,size());
            fill(streaming,version);
        }

        /**
         * @brief Prefill header
         * @param version Version of container format to set in header
         */
        void fill(bool streaming=false, uint8_t version=VERSION)
        {
            setVersion(version);
            setStreamingMode(streaming);
        }

        //! Offset in header for the size of plaintext
        constexpr static size_t plaintextSizeOffset() noexcept
        {
            return PLAINTEXT_SIZE_OFFSET;
        }

        //! Offset in header for the size of ciphertext
        constexpr static size_t ciphertextSizeOffset() noexcept
        {
            return CIPHERTEXT_SIZE_OFFSET;
        }

        //! Offset in header for the size of descriptor
        constexpr static size_t descriptorSizeOffset() noexcept
        {
            return DESCRIPTOR_SIZE_OFFSET;
        }

        /**
         * @brief Convert size for storage format
         * @param size Size
         */
        static void contentSizeForStorageInplace(uint64_t& size) noexcept
        {
            boost::endian::native_to_little_inplace(size);
        }

        void setStreamingMode(bool enable) noexcept
        {
            if (enable)
            {
                memcpy(m_ptr,STREAM_PREFIX,PREFIX_LENGTH);
            }
            else
            {
                memcpy(m_ptr,PREFIX,PREFIX_LENGTH);
            }
        }

        bool isStreamingMode() const noexcept
        {
            return memcmp(data(),STREAM_PREFIX,PREFIX_LENGTH)==0;
        }

    private:

        char m_data[22];
        char* m_ptr;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTCONTAINNERHEADER_H
