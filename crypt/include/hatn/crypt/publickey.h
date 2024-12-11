/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/publickey.h
 *
 *      Wrapper/container of public keys
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTPUBLICKEY_H
#define HATNCRYPTPUBLICKEY_H

#include <hatn/common/bytearray.h>
#include <hatn/common/pointerwithinit.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/keycontainer.h>
#include <hatn/crypt/cryptalgorithm.h>

HATN_CRYPT_NAMESPACE_BEGIN

class PrivateKey;

//! Wrapper/container of public key
class PublicKey : public KeyContainer<common::ByteArray>
{
    public:

        using KeyContainer<common::ByteArray>::KeyContainer;

        virtual ~PublicKey()=default;
        PublicKey(const PublicKey&)=delete;
        PublicKey(PublicKey&&) =delete;
        PublicKey& operator=(const PublicKey&)=delete;
        PublicKey& operator=(PublicKey&&) =delete;

        //! Check if native key handler is valid
        virtual bool isNativeValid() const noexcept
        {
            return true;
        }

        //! Set cryptographic algorithm
        void setAlg(const CryptAlgorithm* alg) noexcept
        {
            m_alg=alg;
        }
        //! Get cryptographic algorithm
        const CryptAlgorithm* alg() const noexcept
        {
            return m_alg.ptr;
        }

        //! Check if algorithm is set
        bool isAlgDefined() const noexcept
        {
            return !m_alg.isNull();
        }

        inline common::Error serializeNativeToContainer()
        {
            return exportToBuf(content(),format());
        }
        inline common::Error parseNativeFromContainer()
        {
            return importFromBuf(content().data(),content().size(),format());
        }

        template <typename ContainerT>
        common::Error importFromBuf(const ContainerT& container,ContainerFormat format=ContainerFormat::PEM,bool keepContent=true)
        {
            return importFromBuf(container.data(),container.size(),format,keepContent);
        }

        /**
         * @brief Import from buffer
         * @param buf Buffer to import from
         * @param size Size of the buffer
         * @param format Data format
         * @return Operation status
         */
        virtual common::Error importFromBuf(const char* buf, size_t size,ContainerFormat format=ContainerFormat::UNKNOWN,bool keepContent=true)
        {
            loadContent(buf,size);
            if (format!=ContainerFormat::UNKNOWN)
            {
                setFormat(format);
            }
            std::ignore=keepContent;
            return common::Error();
        }

        /**
         * @brief Import from c-string
         * @param buf Buffer to import from
         * @return Operation status
         */
        virtual common::Error importFromBuf(const char* buf,ContainerFormat format=ContainerFormat::RAW_PLAIN)
        {
            return importFromBuf(buf,strlen(buf),format);
        }

        /**
         * @brief Export key to buffer
         * @param buf Buffer to put exported result to
         * @param format Data format
         * @return Operation status
         */
        virtual common::Error exportToBuf(common::ByteArray& buf,ContainerFormat fmt=ContainerFormat::PEM) const
        {
            if (fmt!=format())
            {
                return makeCryptError(CryptErrorCode::INVALID_CONTENT_FORMAT);
            }
            buf.load(content());
            return common::Error();
        }

        /**
         * @brief Export public key to file
         * @param filename File name
         * @param format Format of exported data
         *
         * @return Operation status
         */
        common::Error exportToFile(const char* filename,
                             ContainerFormat format=ContainerFormat::PEM
                        ) const
        {
            common::ByteArray tmpBuf;
            HATN_CHECK_RETURN(exportToBuf(tmpBuf,format));
            return tmpBuf.saveToFile(filename);
        }

        /**
         * @brief Export public key to file
         * @param filename File name
         * @param format Format of exported data
         *
         * @return Operation status
         */
        template <typename FilenameT>
        common::Error exportToFile(const FilenameT& filename,
                                         ContainerFormat format=ContainerFormat::PEM
                                    ) const
        {
            return exportToFile(filename.c_str(),format);
        }

        /**
         * @brief Import key from file
         * @param filename File name
         * @param bufFormat Buffer data format
         * @return Operation status
         */
        common::Error importFromFile(
            const char* filename,
            ContainerFormat bufFormat=ContainerFormat::UNKNOWN
        )
        {
            common::ByteArray tmpBuf;
            HATN_CHECK_RETURN(tmpBuf.loadFromFile(filename))
            return importFromBuf(tmpBuf,bufFormat);
        }

        /**
         * @brief Import key from file
         * @param filename File name
         * @param bufFormat Buffer data format
         * @return Operation status
         */
        template <typename FilenameT>
        common::Error importFromFile(
                const FilenameT& filename,
                ContainerFormat bufFormat=ContainerFormat::UNKNOWN
            )
        {
            return importFromFile(filename.c_str(),bufFormat);
        }

        /**
         * @brief Derive public key from private key
         * @param pkey Private key
         * @return Operation status
         */
        virtual common::Error derive(const PrivateKey& pkey)
        {
            std::ignore=pkey;
            return makeCryptError(CryptErrorCode::INVALID_OPERATION);
        }

        virtual bool isBackendKey() const noexcept
        {
            return false;
        }

        //! Check is the key is null
        inline bool isNull() const noexcept
        {
            return isBackendKey()? !isNativeValid() : content().isEmpty();
        }

        //! Unpack key's content
        common::Error unpackContent()
        {
            if (!isNativeValid())
            {
                common::ByteArray buf=content();
                HATN_CHECK_RETURN(importFromBuf(buf,format()))
            }
            return common::Error();
        }

    private:

        common::ConstPointerWithInit<CryptAlgorithm> m_alg;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTPUBLICKEY_H
