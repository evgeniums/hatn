/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/securekey.h
 *
 *  Base classes for secure keys
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTSECUREKEY_H
#define HATNCRYPTSECUREKEY_H

#include <hatn/common/error.h>
#include <hatn/common/memorylockeddata.h>
#include <hatn/common/pointerwithinit.h>
#include <hatn/common/valueordefault.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/cryptalgorithm.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/keycontainer.h>

HATN_CRYPT_NAMESPACE_BEGIN

class SymmetricKey;
class EncryptHMAC;
class KeyProtector;

//! Base class for secure keys
class HATN_CRYPT_EXPORT SecureKey : public KeyContainer<common::MemoryLockedArray,ContainerFormat::RAW_ENCRYPTED>
{
    public:

        enum class Role : int
        {
            GENERAL=0,
            ENCRYPT_SYMMETRIC=0x1,
            ENCRYPT_ASYMMETRIC=0x2,
            MAC=0x8,
            SIGN=0x10,
            DH_SECRET=0x20,
            DH_PRIV_KEY=0x40,
            PASSPHRASE=0x80
        };
        constexpr static uint32_t roleInt(Role role) noexcept
        {
            return static_cast<uint32_t>(role);
        }
        inline bool hasRole(Role r) const noexcept
        {
            return roleInt(r) & role();
        }

        using KeyContainer<common::MemoryLockedArray,ContainerFormat::RAW_ENCRYPTED>::KeyContainer;
        using PasswordCb=std::function<bool (char* /*password data*/, size_t /*max length*/, size_t& /*password length*/)>;

        virtual ~SecureKey()=default;
        SecureKey(const SecureKey&)=delete;
        SecureKey(SecureKey&&) =delete;
        SecureKey& operator=(const SecureKey&)=delete;
        SecureKey& operator=(SecureKey&&) =delete;

        //! Set protector
        inline void setProtector(KeyProtector* key) noexcept
        {
            m_protector.ptr=key;
        }
        //! Get protector
        inline KeyProtector* protector() const noexcept
        {
            return m_protector.ptr;
        }

        //! Set protected flag
        inline void setContentProtected(bool enable) noexcept
        {
            m_protected.val=enable;
        }
        //! Get protected flag
        inline bool isContentProtected() const noexcept
        {
            return m_protected.val;
        }

        //! Pack key's content so that it contains imported (secure) form of key data
        common::Error packContent(ContainerFormat format=ContainerFormat::PEM,bool unprotected=false)
        {
            if (isContentProtected()==unprotected || (isNativeValid()&&content().isEmpty()))
            {
                common::MemoryLockedArray buf;
                HATN_CHECK_RETURN(exportToBuf(buf,format,unprotected))
                content()=std::move(buf);
            }
            return common::Error();
        }

        //! Unpack key's content
        common::Error unpackContent()
        {
            if (isContentProtected() || !isNativeValid())
            {
                common::MemoryLockedArray buf=content();
                return importFromBuf(buf,format(),true);
            }
            return common::Error();
        }

        /**
         * @brief Export secure key to buffer
         * @param buf Buffer to put exported result to
         * @param format Format of exported data
         * @param unprotected Unprotected/protected format of exported data. Note that some backends will not permit exporintg unprotected keys.
         *
         * @return Operation status
         */
        inline common::Error exportToBuf(common::MemoryLockedArray& buf,
                                         ContainerFormat format=ContainerFormat::PEM,
                                         bool unprotected=false
                                    ) const
        {
            return doExportToBuf(buf,format,unprotected || format==ContainerFormat::RAW_PLAIN || format==ContainerFormat::DER);
        }

        /**
         * @brief Export secure key to file
         * @param filename File name
         * @param format Format of exported data
         * @param unprotected Unprotected/protected format of exported data. Note that some backends will not permit exporintg unprotected keys.
         *
         * @return Operation status
         */
        common::Error exportToFile(const char* filename,
                                         ContainerFormat format=ContainerFormat::PEM,
                                         bool unprotected=false
                                    ) const;

        /**
         * @brief Export secure key to file
         * @param filename File name
         * @param format Format of exported data
         * @param unprotected Unprotected/protected format of exported data. Note that some backends will not permit exporintg unprotected keys.
         *
         * @return Operation status
         */
        template <typename FilenameT>
        common::Error exportToFile(const FilenameT& filename,
                                         ContainerFormat format=ContainerFormat::PEM,
                                         bool unprotected=false
                                    ) const
        {
            return exportToFile(filename.c_str(),format, unprotected);
        }

        /**
         * @brief Import secure key
         * @param buf Buffer to import from
         * @param size Size of the buffer
         * @param bufFormat Buffer data format
         * @param keepContent Keep content in key's container
         * @return Operation status
         */
        inline common::Error importFromBuf(
                const char* buf,
                size_t size,
                ContainerFormat bufFormat=ContainerFormat::UNKNOWN,
                bool keepContent=false
            )
        {
            if (bufFormat==ContainerFormat::UNKNOWN)
            {
                bufFormat=format();
            }
            return doImportFromBuf(buf,size,bufFormat,keepContent);
        }

        /**
         * @brief Import secure key
         * @param buf Buffer to import from
         * @param bufFormat Buffer data format
         * @param keepContent Keep content in key's container
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error importFromBuf(
                const ContainerT& buf,
                ContainerFormat bufFormat=ContainerFormat::UNKNOWN,
                bool keepContent=false
            )
        {
            return importFromBuf(buf.data(),buf.size(),bufFormat,keepContent);
        }

        /**
         * @brief Import secure key from plain string
         * @param buf C-string to import from
         * @return Operation status
         */
        inline common::Error importFromBuf(
                const char* buf
            )
        {
            return importFromBuf(buf,strlen(buf),ContainerFormat::RAW_PLAIN);
        }

        /**
         * @brief Import key from file
         * @param filename File name
         * @param bufFormat Buffer data format
         * @param keepContent Keep content in key's container
         * @return Operation status
         */
        common::Error importFromFile(
            const char* filename,
            ContainerFormat bufFormat=ContainerFormat::UNKNOWN,
            bool keepContent=false
        );

        /**
         * @brief Import key from file
         * @param filename File name
         * @param bufFormat Buffer data format
         * @param keepContent Keep content in key's container
         * @return Operation status
         */
        template <typename FilenameT>
        common::Error importFromFile(
                const FilenameT& filename,
                ContainerFormat bufFormat=ContainerFormat::UNKNOWN,
                bool keepContent=false
            )
        {
            return importFromFile(filename.c_str(),bufFormat,keepContent);
        }

        /**
         * @brief Import raw data from key deriiation functions
         * @param buf Buffer to import from
         * @param bufSize Size of the buffer
         * @return Operation status
         */
        virtual common::Error importFromKDF(
                const char* buf,
                size_t size
            )
        {
            return importFromBuf(buf,size,ContainerFormat::RAW_PLAIN);
        }

        inline common::Error serializeNativeToContent()
        {
            return exportToBuf(content());
        }
        inline common::Error parseNativeFromContent()
        {
            return importFromBuf(content());
        }

        //! Check if the key can keep unprotected content
        virtual bool canKeepUnprotectedContent() const noexcept
        {
            return false;
        }

        //! Check is the key is null
        inline bool isNull() const noexcept
        {
            return isBackendKey()? !isNativeValid() : content().isEmpty();
        }

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

        //! Get roles of this key
        virtual uint32_t role() const noexcept
        {
            return roleInt(Role::GENERAL);
        }

        /**
         * @brief Generate a key
         * @return Operation status
         *
         * Note that alg() must be set before key generation.
         */
        common::Error generate() noexcept
        {
            if (!isAlgDefined())
            {
                return cryptError(CryptError::INVALID_ALGORITHM);
            }
            return doGenerate();
        }

        virtual bool isBackendKey() const noexcept
        {
            return false;
        }

    protected:

        /**
         * @brief Export secure key to buffer
         * @param key Key to export
         * @param buf Buffer to put exported result to
         * @param format Format of exported data
         * @param unprotected Unprotected/protected format of exported data. Note that some backends will not permit exporintg unprotected keys.
         *
         * @return Operation status
         */
        virtual common::Error doExportToBuf(common::MemoryLockedArray& buf,
                                         ContainerFormat format,
                                         bool unprotected
                                    ) const;

        /**
         * @brief Import secure key from protected format
         * @param buf Buffer to import from
         * @param bufSize Size of the buffer
         * @param format Buffer data format
         * @param keepContent Keep content in key's container
         * @return Operation status
         */
        virtual common::Error doImportFromBuf(const char* buf, size_t size, ContainerFormat format, bool keepContent);

        virtual common::Error doGenerate()
        {
            return cryptError(CryptError::INVALID_OPERATION);
        }

    private:

        common::PointerWithInit<KeyProtector> m_protector;
        common::ValueOrDefault<bool,false> m_protected;

        common::ConstPointerWithInit<CryptAlgorithm> m_alg;
};

//! Base class for private key used in asymmetric algorithms
class PrivateKey : public SecureKey
{
    public:

        //! Constructor
        PrivateKey()
        {
            setFormat(ContainerFormat::PEM);
        }

        //! Ctor from ByteArray
        explicit PrivateKey(common::MemoryLockedArray data) noexcept : SecureKey(std::move(data))
        {
            setFormat(ContainerFormat::PEM);
        }

        //! Ctor from data container
        template <typename ContainerT>
        explicit PrivateKey(const ContainerT& container) : SecureKey(container)
        {
            setFormat(ContainerFormat::PEM);
        }

        //! Ctor from data
        PrivateKey(const char* buf, size_t size):SecureKey(buf,size)
        {
            setFormat(ContainerFormat::PEM);
        }

        virtual uint32_t role() const noexcept override
        {
            return roleInt(Role::ENCRYPT_ASYMMETRIC) | roleInt(Role::SIGN) | roleInt(Role::DH_PRIV_KEY);
        }
};

template <bool Encrypt> class CipherWorker;

//! Base class for symmetric key used in symmetric algorithms
class HATN_CRYPT_EXPORT SymmetricKey : public SecureKey
{
    public:

        using SecureKey::SecureKey;

        virtual uint32_t role() const noexcept override
        {
            return roleInt(Role::ENCRYPT_SYMMETRIC);
        }

    protected:

        virtual common::Error doGenerate() override;
};
using SymmetricKeyConstPtr=const SymmetricKey*;

//! Base class for MAC keys used in MAC algorithms
class MACKey : public SymmetricKey
{
    public:

        using SymmetricKey::SymmetricKey;

        virtual uint32_t role() const noexcept override
        {
            return SymmetricKey::role() | roleInt(Role::MAC);
        }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTSECUREKEY_H
