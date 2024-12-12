/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslprivatekey.h
 *
 * Private key wrapper with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLPRIVATEKEY_H
#define HATNOPENSSLPRIVATEKEY_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/securekey.h>

#include <hatn/crypt/plugins/openssl/opensslsecretkey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

namespace detail
{
struct PkeyTraits
{
    static void free(EVP_PKEY* key)
    {
        ::EVP_PKEY_free(key);
    }
};
struct PkeyCtxTraits
{
    static void free(EVP_PKEY_CTX* ctx)
    {
        ::EVP_PKEY_CTX_free(ctx);
    }
};
}

//! Private key wrapper with OpenSSL backend
template <typename BaseT>
class OpenSslPKey :
            public common::NativeHandlerContainer<EVP_PKEY,detail::PkeyTraits,OpenSslSecretKey<BaseT>,OpenSslPKey<BaseT>>
{
    public:

        using BaseTmplT=common::NativeHandlerContainer<EVP_PKEY,detail::PkeyTraits,OpenSslSecretKey<BaseT>,OpenSslPKey<BaseT>>;
        using BaseTmplT::BaseTmplT;

        /**
         * @brief Parse content to native key
         * @param key Parsed native key
         * @param data Pointer to buffer
         * @param size Size of data
         * @param format Format of data
         * @param passwdCallback Password callback for encrypted content
         * @param passwdCallbackUserdata Data for password callback
         * @return Operation status
         */
        static Error parseContent(
            typename BaseTmplT::Native& native,
            const char* data,
            size_t size,
            ContainerFormat format=ContainerFormat::PEM,
            pem_password_cb* passwdCallback = nullptr,
            void* passwdCallbackUserdata = nullptr
        ) noexcept;

        /**
         * @brief Parse key from content to underlying native key
         * @param passwdCallback Password callback for encrypted files
         * @param passwdCallbackUserdata Data for password callback
         * @return Operation status
         */
        Error parseContent(
            pem_password_cb* passwdCallback = nullptr,
            void* passwdCallbackUserdata = nullptr
        ) noexcept;

        /**
         * @brief Serialize key to MemoryLockedArray
         * @param native Native object
         * @param content Target buffer
         * @param enc Encryption algorithm
         * @param format Container format
         * @param passPhrase Passphrase to encrypt key
         * @param psLength Length of passhrase
         * @param passwdCallback Password callback for encrypted content
         * @param passwdCallbackUserdata Data for password callback
         *
         * @return Operation status
         */
        static Error serialize(
            const typename BaseTmplT::Native& native,
            common::MemoryLockedArray& content,
            const EVP_CIPHER *enc,
            ContainerFormat format=ContainerFormat::PEM,
            const char *passPhrase=nullptr,
            size_t psLength=0,
            pem_password_cb* passwdCallback = nullptr,
            void* passwdCallbackUserdata = nullptr
        );

        virtual bool isNativeValid() const noexcept override
        {
            return this->isValid();
        }

        virtual bool isBackendKey() const noexcept override
        {
            return true;
        }

        virtual int nativeType() const noexcept
        {
            return -1;
        }

    protected:

        virtual common::Error doGenerate() override;

        virtual common::Error doExportToBuf(common::MemoryLockedArray& buf,
                                         ContainerFormat format,
                                         bool unprotected
                                    ) const override;
        virtual common::Error doImportFromBuf(
                    const char* buf,
                    size_t size,
                    ContainerFormat format,
                    bool keepContent) override;
};

using OpenSslPrivateKey=OpenSslPKey<PrivateKey>;
#ifdef _WIN32
template class HATN_OPENSSL_EXPORT OpenSslPKey<PrivateKey>;
#endif

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLPRIVATEKEY_H
