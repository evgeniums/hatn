/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/encryptedtoken.h
  */

/****************************************************************************/

#ifndef HATNENCRYPTEDTOKEN_H
#define HATNENCRYPTEDTOKEN_H

#include <hatn/crypt/ciphersuite.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

class HATN_SERVERAPP_EXPORT EncryptedToken
{
    public:

        EncryptedToken() : m_suite(nullptr)
        {}

        Error init(
            const crypt::CipherSuite* suite,
            lib::string_view secret
        );

        Error encrypt(
            const common::ByteArray* serializedToken,
            common::ByteArray& encryptedToken,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const;

        Error decrypt(
            const common::ByteArray& encryptedToken,
            common::ByteArray& serializedToken,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const;

        template <typename TokenT>
        Error serializeToken(
            const TokenT& token,
            common::ByteArray& encryptedToken,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const;

        template <typename TokenT>
        Result<common::SharedPtr<TokenT>> parseToken(
            const common::ByteArray& encryptedToken,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const;

    protected:

        const crypt::CipherSuite* m_suite;
        common::SharedPtr<crypt::SymmetricKey> m_tokenEncryptionKey;
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNENCRYPTEDTOKEN_H
