/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file crypt/plugins/openssl/opensslhkdf.h
 *
 * 	Hash Key Derivation Functions (HKDF) using OpenSSL backend
 *
 */

/****************************************************************************/

#ifndef HATNOPENSSLHKDF_H
#define HATNOPENSSLHKDF_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/kdf.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/hkdf.h>

#include <hatn/crypt/plugins/openssl/opensslplugin.h>
#include <hatn/crypt/plugins/openssl/opensslerror.h>

HATN_OPENSSL_NAMESPACE_BEGIN

namespace detail
{
struct HKDFTraits
{
    static void free(::EVP_PKEY_CTX * ctx)
    {
        ::EVP_PKEY_CTX_free(ctx);
    }
};
}

//! Hash Key Derivation Functions (HKDF)
class HATN_OPENSSL_EXPORT OpenSslHKDF : public common::NativeHandlerContainer<EVP_PKEY_CTX,detail::HKDFTraits,HKDF,OpenSslHKDF>
{
    public:

    using common::NativeHandlerContainer<EVP_PKEY_CTX,detail::HKDFTraits,HKDF,OpenSslHKDF>::NativeHandlerContainer;

    protected:

        virtual void doReset() noexcept override;

        virtual common::Error doInit(
            const SecureKey* masterKey,
            const char* saltData,
            size_t saltSize
        ) override;

        virtual common::Error doDerive(
            common::SharedPtr<SymmetricKey>& derivedKey,
            const char* infoData,
            size_t infoSize
        ) override;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLHKDF_H
