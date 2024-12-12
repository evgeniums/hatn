/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslsecretkey.h
 *
 * Bases for secret key classes of OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLSECRETKEY_H
#define HATNOPENSSLSECRETKEY_H

#include <hatn/crypt/securekey.h>
#include <hatn/crypt/passphrasekey.h>
#include <hatn/crypt/encrypthmac.h>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>
#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslplugin.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! base template for secret key classes of OpenSSL backend
template <typename BaseT>
class OpenSslSecretKey : public BaseT
{
    public:

        using BaseT::BaseT;

        /**
         * @brief Check if the key can keep unprotected content
         * @return
         *
         * OpenSSL uses plain data of keys as inputs to cipher functions,
         * so the keys must keep their data in buffers in unprotected form.
         */
        virtual bool canKeepUnprotectedContent() const noexcept override
        {
            return true;
        }
};

class OpenSslSymmetricKey : public OpenSslSecretKey<SymmetricKey>
{
    public:

        using OpenSslSecretKey<SymmetricKey>::OpenSslSecretKey;
};

using OpenSslPassphraseKey=PassphraseKey;

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLSECRETKEY_H
