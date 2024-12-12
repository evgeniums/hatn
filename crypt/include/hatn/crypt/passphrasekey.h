/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/passphrasekey.h
 *
 *  Base class for keys that are containers of passhrases
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTPASSHRASEKEY_H
#define HATNCRYPTPASSHRASEKEY_H

#include <hatn/crypt/securekey.h>
#include <hatn/crypt/pbkdf.h>
#include <hatn/crypt/passwordgenerator.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Base class for keys that are containers of passhrases
class PassphraseKey : public SymmetricKey
{
    public:

        //! Constructor
        PassphraseKey()
        {
            setFormat(ContainerFormat::RAW_PLAIN);
        }

        //! Ctor from ByteArray
        explicit PassphraseKey(common::MemoryLockedArray data) noexcept : SymmetricKey(std::move(data))
        {
            setFormat(ContainerFormat::RAW_PLAIN);
        }

        //! Ctor from data container
        template <typename ContainerT>
        explicit PassphraseKey(const ContainerT& container) : SymmetricKey(container)
        {
            setFormat(ContainerFormat::RAW_PLAIN);
        }

        //! Ctor from data
        PassphraseKey(const char* buf, size_t size) : SymmetricKey(buf,size)
        {
            setFormat(ContainerFormat::RAW_PLAIN);
        }

        virtual uint32_t role() const noexcept override
        {
            return roleInt(Role::PASSPHRASE);
        }

        common::Error deriveKey()
        {
            if (!m_derivedKey)
            {
                return PBKDF::derive(this,m_salt);
            }
            return common::Error();
        }

        const SymmetricKey* derivedKey() const
        {
            if (!m_derivedKey)
            {
                auto ec=const_cast<PassphraseKey*>(this)->deriveKey();
                if (ec)
                {
                    throw common::ErrorException(ec);
                }
            }
            return m_derivedKey.get();
        }

        inline bool isDerivedKeyReady() const noexcept
        {
            return m_derivedKey;
        }

        void resetDerivedKey() noexcept
        {
            m_derivedKey.reset();
        }

        //! Set KDF algorithm
        inline void setKdfAlg(const CryptAlgorithm* alg) noexcept
        {
            m_kdfAlg=alg;
        }
        //! Get KDF algorithm
        inline const CryptAlgorithm* kdfAlg() const noexcept
        {
            return m_kdfAlg.ptr;
        }

        inline void setDerivedKey(common::SharedPtr<SymmetricKey> key) noexcept
        {
            m_derivedKey=std::move(key);
        }

        inline common::SharedPtr<SymmetricKey>& derivedKeyHolder() noexcept
        {
            return m_derivedKey;
        }

        //! Check if the key can keep unprotected content
        virtual bool canKeepUnprotectedContent() const noexcept override
        {
            return true;
        }

        void setSalt(const char* data, size_t size)
        {
            m_salt.load(data,size);
        }
        void setSalt(const char* data)
        {
            m_salt.load(data);
        }
        template <typename ContainerT>
        void setSalt(const ContainerT& container)
        {
            setSalt(container.data(),container.size());
        }

        const char* rawData() const noexcept
        {
            return content().data();
        }
        size_t rawSize() const noexcept
        {
            return content().size();
        }

        common::Error generatePassword(PasswordGenerator* gen)
        {
            setFormat(ContainerFormat::RAW_PLAIN);
            return gen->generate(content());
        }

    protected:

        virtual common::Error doGenerate() override
        {
            return cryptError(CryptError::INVALID_OPERATION);
        }

    private:

        common::ConstPointerWithInit<CryptAlgorithm> m_kdfAlg;
        common::SharedPtr<SymmetricKey> m_derivedKey;
        common::ByteArray m_salt;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTPASSHRASEKEY_H
