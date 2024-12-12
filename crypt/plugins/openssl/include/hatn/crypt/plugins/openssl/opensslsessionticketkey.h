/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslsessionticketkey.h
  *
  *   Key to encrypt session tickets according to RFC5077 with OpenSSL backend
  *
  */

/****************************************************************************/

#ifndef HATNOPENSSLSESSIONTICKETKEY_H
#define HATNOPENSSLSESSIONTICKETKEY_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <hatn/crypt/sessionticketkey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Key to encrypt session tickets according to RFC5077
class HATN_OPENSSL_EXPORT OpenSslSessionTicketKey : public SessionTicketKey
{
    public:

        constexpr static const size_t HMAC_KEY_LEN=16;

        //! Ctor
        OpenSslSessionTicketKey():m_cipher(nullptr),m_hmac(nullptr)
        {}

        //! Reset key
        virtual void reset() override
        {
            doReset();
        }

        //! Load key from data buffer
        virtual common::Error load(const char* data, size_t size, KeyProtector* protector=nullptr) override;
        //! Store key to data buffer
        virtual common::Error store(common::MemoryLockedArray& content, KeyProtector* protector=nullptr) const override;

        /**
         * @brief Generate new key
         * @param name Name of the key
         * @param cipherName Name of encryption algorithm
         * @param hmacName Name of HMAC algorithm
         * @return
         */
        virtual common::Error generate(
                                    const common::FixedByteArray16& name,
                                    const char* cipherName,
                                    const char* hmacName) override;

        const common::MemoryLockedArray& cipherKey() const noexcept
        {
            return m_cipherKey;
        }
        const common::MemoryLockedArray& macKey() const noexcept
        {
            return m_macKey;
        }
        const EVP_CIPHER* cipher() const noexcept
        {
            return m_cipher;
        }
        const EVP_MD* hmac() const noexcept
        {
            return m_hmac;
        }

        bool operator == (const OpenSslSessionTicketKey& other) const noexcept
        {
            return name()==other.name();
        }

    private:

        void doReset()
        {
            mutableName().clear();
            m_cipherKey.clear();
            m_macKey.clear();
            m_cipher=nullptr;
            m_hmac=nullptr;
            setValid(false);
        }

        common::MemoryLockedArray m_cipherKey;
        common::MemoryLockedArray m_macKey;
        const EVP_CIPHER* m_cipher;
        const EVP_MD* m_hmac;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLSESSIONTICKETKEY_H
