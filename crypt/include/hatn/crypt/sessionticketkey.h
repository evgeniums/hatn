/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/sessionticketkey.h
  *
  *   Base class for keys to encrypt session tickets according to RFC5077
  *
  */

/****************************************************************************/

#ifndef HATNSESSIONTICKETKEY_H
#define HATNSESSIONTICKETKEY_H

#include <hatn/common/error.h>
#include <hatn/common/memorylockeddata.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/securekey.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Base class for keys to encrypt session tickets according to RFC5077
class SessionTicketKey
{
    public:

        constexpr static const size_t CIPHER_NAME_MAXLEN=32;
        constexpr static const size_t HMAC_NAME_MAXLEN=32;

        //! Ctor
        SessionTicketKey():m_valid(false)
        {}

        virtual ~SessionTicketKey()=default;
        SessionTicketKey(const SessionTicketKey&)=default;
        SessionTicketKey(SessionTicketKey&&) =default;
        SessionTicketKey& operator=(const SessionTicketKey&)=default;
        SessionTicketKey& operator=(SessionTicketKey&&) =default;

        //! Reset key
        virtual void reset()=0;

        //! Load key from data buffer
        virtual common::Error load(const char* data, size_t size, KeyProtector* protector=nullptr)=0;

        //! Load key from data buffer
        inline common::Error load(const common::MemoryLockedArray& content, KeyProtector* protector=nullptr)
        {
            return load(content.data(),content.size(),protector);
        }
        //! Store key to data buffer
        virtual common::Error store(common::MemoryLockedArray& content, KeyProtector* protector=nullptr) const=0;

        /**
         * @brief Generate new key
         * @param name Name of the key
         * @param cipherName Name of encryption algorithm
         * @param hmacName Name of HMAC algorithm
         * @return Operation status
         */
        virtual common::Error generate(
                                    const common::FixedByteArray16& name,
                                    const char* cipherName,
                                    const char* hmacName)=0;

        //! Check if key is valid
        inline bool isValid() const noexcept
        {
            return m_valid;
        }

        const common::FixedByteArray16& name() const noexcept
        {
            return m_name;
        }

    protected:

        //! Set valid flag in the key
        inline void setValid(bool enable=true) noexcept
        {
            m_valid=enable;
        }

        inline common::FixedByteArray16& mutableName() noexcept
        {
            return m_name;
        }

    private:

        bool m_valid;
        common::FixedByteArray16 m_name;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNSESSIONTICKETKEY_H
