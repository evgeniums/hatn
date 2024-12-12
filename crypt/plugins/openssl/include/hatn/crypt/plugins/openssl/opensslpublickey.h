/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file crypt/plugins/openssl/opensslpublickey.h
 *
 * Public keys wrapper with OpenSSL backend
 *
 */

/****************************************************************************/

#ifndef HATNOPENSSLPUBLICKEY_H
#define HATNOPENSSLPUBLICKEY_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/securekey.h>

#include <hatn/crypt/plugins/openssl/opensslsecretkey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

namespace detail
{
struct PublicKeyTraits
{
    static void free(EVP_PKEY* key)
    {
        ::EVP_PKEY_free(key);
    }
};
}

//! Public key wrapper with OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslPublicKey :
                            public common::NativeHandlerContainer<EVP_PKEY,detail::PublicKeyTraits,PublicKey,OpenSslPublicKey>
{
    public:

        using common::NativeHandlerContainer<EVP_PKEY,detail::PublicKeyTraits,PublicKey,OpenSslPublicKey>::NativeHandlerContainer;

        virtual bool isBackendKey() const noexcept override
        {
            return true;
        }

        /**
         * @brief Parse content to native key
         * @param key Parsed native key
         * @param data Pointer to buffer
         * @param size Size of data
         * @param format Format of data
         * @return Operation status
         */
        static Error parseContent(
            Native& native,
            const char* data,
            size_t size,
            ContainerFormat format=ContainerFormat::PEM
        ) noexcept;

        /**
         * @brief Serialize key to ByteArray
         * @param native Native object
         * @param content Target buffer
         * @param format Container format
         *
         * @return Operation status
         */
        static Error serialize(
            const Native& native,
            common::ByteArray& content,
            ContainerFormat format=ContainerFormat::PEM
        );

        virtual bool isNativeValid() const noexcept override
        {
            return isValid();
        }

        /**
         * @brief Derive public key from private key
         * @param pkey Private key
         * @return Operation status
         */
        virtual common::Error derive(const PrivateKey& pkey) override;

        /**
         * @brief Import key from buffer
         * @param buf Buffer to import from
         * @param size Size of the buffer
         * @param format Data format
         * @return Operation status
         */
        virtual common::Error importFromBuf(const char* buf, size_t size,ContainerFormat format=ContainerFormat::PEM,bool keepContent=true) override;

        /**
         * @brief Export secure key to buffer in protected format
         * @param buf Buffer to put exported result to
         * @param format Data format
         * @return Operation status
         */
        virtual common::Error exportToBuf(common::ByteArray& buf,ContainerFormat format=ContainerFormat::PEM) const override;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLPUBLICKEY_H
