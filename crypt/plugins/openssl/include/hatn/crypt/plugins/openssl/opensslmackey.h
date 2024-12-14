/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file crypt/plugins/openssl/opensslmackey.h
 *
 *  MAC key with OpenSSL backend
 *
 */

/****************************************************************************/

#ifndef HATNOPENSSLMACKEY_H
#define HATNOPENSSLMACKEY_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/hmac.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

#ifdef _WIN32
    template class HATN_OPENSSL_EXPORT OpenSslPKey<MACKey>;
#endif

#if OPENSSL_API_LEVEL >= 30100

class OpenSslMACKey : public OpenSslPKey<MACKey>
{
    public:

        using baseT=OpenSslPKey<MACKey>;

        using baseT::baseT;

        virtual bool isBackendKey() const noexcept override
        {
            return false;
        }
};

#else

using OpenSslHMACKey=MACKey;
using OpenSslMACKey=OpenSslPKey<MACKey>;

class MACKey_HMAC : public OpenSslMACKey
{
public:

    using OpenSslMACKey::OpenSslMACKey;

    virtual int nativeType() const noexcept override
    {
        return EVP_PKEY_HMAC;
    }
};
class MACKey_Poly1305 : public OpenSslMACKey
{
public:

    using OpenSslMACKey::OpenSslMACKey;

    virtual int nativeType() const noexcept override
    {
        return EVP_PKEY_POLY1305;
    }
};
class MACKey_SIPHASH : public OpenSslMACKey
{
public:

    using OpenSslMACKey::OpenSslMACKey;

    virtual int nativeType() const noexcept override
    {
        return EVP_PKEY_SIPHASH;
    }
};
class MACKey_CMAC : public OpenSslMACKey
{
public:

    using OpenSslMACKey::OpenSslMACKey;

    virtual int nativeType() const noexcept override
    {
        return EVP_PKEY_CMAC;
    }
};

#endif


HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLMACKEY_H
