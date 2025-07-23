/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/encryptioncontainer.h
 *      Base container class for encryption keys and other data
 */
/****************************************************************************/

#ifndef HATNCRYPTENCRYPTIONCONTAINER_H
#define HATNCRYPTENCRYPTIONCONTAINER_H

#include <hatn/common/error.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/memorylockeddata.h>

#include <hatn/crypt/crypt.h>

HATN_CRYPT_NAMESPACE_BEGIN

enum class ContainerFormat : int
{
    UNKNOWN=0,
    PEM,
    DER,
    RAW_PLAIN,
    RAW_ENCRYPTED
};

inline std::string ContainerFormatToStr(ContainerFormat format)
{
    switch (format)
    {
        case (ContainerFormat::PEM):
        {
            return std::string("pem");
        }
            break;

        case (ContainerFormat::DER):
        {
            return std::string("der");
        }
            break;

        case (ContainerFormat::RAW_PLAIN):
        {
            return std::string("dat");
        }
            break;

        case (ContainerFormat::RAW_ENCRYPTED):
        {
            return std::string("hcc");
        }
            break;

        case (ContainerFormat::UNKNOWN):
            break;
    }
    return "unknown";
}

//! Base container class for encryption keys and other data
template <typename ContentT, ContainerFormat Format=ContainerFormat::PEM>
class KeyContainer
{
    public:

        //! Constructor
        KeyContainer():m_format(Format)
        {}

        //! Ctor from ByteArray
        KeyContainer(ContentT content, ContainerFormat format=Format) noexcept : m_format(format),m_content(std::move(content))
        {
        }

        //! Ctor from data container
        template <typename ContainerT>
        KeyContainer(const ContainerT& container, ContainerFormat format=Format):m_format(format)
        {
            loadContent(container);
        }

        //! Ctor from data
        KeyContainer(const char* buf, size_t size, ContainerFormat format=Format):m_format(format),m_content(buf,size)
        {
        }

        //! Load content
        template <typename ContainerT>
        inline void loadContent(const ContainerT& container, ContainerFormat format=Format)
        {
            setFormat(format);
            m_content.load(container.data(),container.size());
        }

        //! Load content
        template <typename ContentT1, ContainerFormat Format1>
        inline void loadContent(const KeyContainer<ContentT1,Format1>& other)
        {
            loadContent(other.content(),other.format());
        }

        //! Load content
        inline void loadContent(const char* buf, size_t size, ContainerFormat format=Format)
        {
            setFormat(format);
            m_content.load(buf,size);
        }

        //! Load content
        inline void loadContent(const char* buf)
        {
            m_content.load(buf);
        }

        //! Set content
        inline void setContent(ContentT content) noexcept
        {
            m_content=std::move(content);
        }

        //! Comparation operator
        inline bool operator ==(const KeyContainer& other) const noexcept
        {
            return m_format==other.m_format && m_content==other.m_content;
        }

        //! Get content
        inline const ContentT& content() const noexcept
        {
            return m_content;
        }

        //! Get content
        inline ContentT& content() noexcept
        {
            return m_content;
        }

        /**
         * @brief Set format of content
         * @param format
         */
        inline void setFormat(ContainerFormat format) noexcept
        {
            m_format=format;
        }

        /**
         * @brief Get format of content
         * @return Format of content
         */
        inline ContainerFormat format() const noexcept
        {
            return m_format;
        }

        //! Load content from file
        inline common::Error loadFromFile(
            const char* fileName //!< Filename
        )
        {
            return content().loadFromFile(fileName);
        }

        //! Load content from file
        inline common::Error loadFromFile(
            const std::string& fileName //!< Filename
        )
        {
            return content().loadFromFile(fileName);
        }
    private:

        ContainerFormat m_format;
        ContentT m_content;
};

template <ContainerFormat Format>
using SecureContainer=KeyContainer<common::MemoryLockedArray,Format>;
using SecurePlainContainer=SecureContainer<ContainerFormat::RAW_PLAIN>;

inline bool checkInContainerSize(
        const size_t containerSize,
        const size_t offset,
        size_t& processSize,
        size_t backOffset=0
    ) noexcept
{
    size_t subtractOffset=0;
    if (processSize==0)
    {
        processSize=containerSize;
        subtractOffset=offset;
    }
    else if (containerSize<(processSize+offset))
    {
        return false;
    }
    if (processSize<(subtractOffset+backOffset))
    {
        return false;
    }
    processSize=processSize-subtractOffset-backOffset;
    return true;
}

inline bool checkOutContainerSize(
        const size_t containerSize,
        const size_t offset
    )
{
    return offset<=containerSize;
}

template <typename ContainerT>
bool checkEmptyOutContainerResult(size_t sizeIn, ContainerT& dataOut, size_t offsetOut)
{
    if (sizeIn==0)
    {
        if (offsetOut==0)
        {
            dataOut.clear();
        }
        else
        {
            dataOut.resize(offsetOut);
        }
        return false;
    }
    return true;
}

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTENCRYPTIONCONTAINER_H
