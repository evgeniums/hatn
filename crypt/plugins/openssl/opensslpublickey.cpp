/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslprivatekey.cpp
 *
 * 	Private key wrapper with OpenSSL backend
 *
 */
/****************************************************************************/

#include <hatn/common/logger.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>

#include <hatn/crypt/plugins/openssl/opensslpublickey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslPublicKey ********************/

//---------------------------------------------------------------
Error OpenSslPublicKey::parseContent(
        Native &native,
        const char *data,
        size_t size,
        ContainerFormat format
    ) noexcept
{
    native.reset();

    if (size==0)
    {
        return cryptError(CryptError::INVALID_KEY_LENGTH);
    }

    BIO *bio;
    auto ec=createMemBio(bio,data,size);
    if (ec)
    {
        return ec;
    }

    if (format==ContainerFormat::DER)
    {
        native.handler=::d2i_PUBKEY_bio(bio,NULL);
    }
    else if (format==ContainerFormat::PEM)
    {
        native.handler=::PEM_read_bio_PUBKEY(bio,NULL,NULL,NULL);
    }
    else
    {
        return cryptError(CryptError::INVALID_CONTENT_FORMAT);
    }

    if (native.isNull())
    {
        BIO_free(bio);
        return makeLastSslError();
    }
    BIO_free(bio);
    return Error();
}

//---------------------------------------------------------------
Error OpenSslPublicKey::serialize(
        const Native &native,
        common::ByteArray &content,
        ContainerFormat format
    )
{
    content.clear();

    if (native.isNull())
    {
        return Error();
    }

    ::ERR_clear_error();
    BIO *bio = ::BIO_new(::BIO_s_mem());
    if (bio==nullptr)
    {
        return makeLastSslError();
    }

    int ret=0;
    if (format==ContainerFormat::DER)
    {
        ret=::i2d_PUBKEY_bio(bio,native.handler);
    }
    else if (format==ContainerFormat::PEM)
    {

        ret=::PEM_write_bio_PUBKEY(bio,native.handler);
    }
    else
    {
        return cryptError(CryptError::INVALID_CONTENT_FORMAT);
    }
    if (ret!=1)
    {
        BIO_free(bio);
        return makeLastSslError();
    }

    readMemBio(bio,content);
    return Error();
}

//---------------------------------------------------------------
Error OpenSslPublicKey::importFromBuf(
        const char *buf, size_t size,
        ContainerFormat format,
        bool keepContent
    )
{
    resetNative();

    if (format==ContainerFormat::RAW_PLAIN)
    {
        // raw plain format can not be loaded to native handler, content is kept as is in container
        loadContent(buf,size);
        setFormat(format);
        return Error();
    }
    else if (format!=ContainerFormat::PEM && format!=ContainerFormat::DER)
    {
        return cryptError(CryptError::INVALID_CONTENT_FORMAT);
    }

    HATN_CHECK_RETURN(parseContent(nativeHandler(),buf,size,format))
    if (keepContent)
    {
        loadContent(buf,size);
        setFormat(format);
    }
    return common::Error();
}

//---------------------------------------------------------------
Error OpenSslPublicKey::exportToBuf(common::ByteArray &buf, ContainerFormat format) const
{
    if (format==ContainerFormat::RAW_PLAIN)
    {
        // raw plain format is taken from content, the native handler is not used

        buf.load(content().data(),content().size());
        return Error();
    }
    else if (format!=ContainerFormat::PEM && format!=ContainerFormat::DER)
    {
        return cryptError(CryptError::INVALID_CONTENT_FORMAT);
    }

    return serialize(nativeHandler(),
                     buf,
                     format
                     );
}

//---------------------------------------------------------------
Error OpenSslPublicKey::derive(const PrivateKey &pkey)
{
    ::ERR_clear_error();

    resetNative();
    content().clear();

    // using slow dynamic_cast here because casting performance in this case is not such important
    const OpenSslPrivateKey* pkeyNative=dynamic_cast<const OpenSslPrivateKey*>(&pkey);
    if (pkeyNative==nullptr)
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }
    if (!pkeyNative->isValid())
    {
        HATN_CHECK_RETURN(const_cast<OpenSslPrivateKey*>(pkeyNative)->parseNativeFromContent())
    }

    common::NativeHandler<EVP_PKEY,detail::PublicKeyTraits> pubkeyHandler;
    int keyID=::EVP_PKEY_base_id(pkeyNative->nativeHandler().handler);
    if (keyID==EVP_PKEY_ED25519 || keyID==EVP_PKEY_ED448)
    {
        if (EVP_PKEY_up_ref(pkeyNative->nativeHandler().handler)!=1)
        {
            return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
        }
        pubkeyHandler.handler=pkeyNative->nativeHandler().handler;
    }
    else
    {
        pubkeyHandler.handler=::EVP_PKEY_new();
        switch (keyID)
        {
            case(EVP_PKEY_EC):
            {
                auto* ctx=::EVP_PKEY_get1_EC_KEY(pkeyNative->nativeHandler().handler);
                if (ctx==nullptr)
                {
                    return makeLastSslError(CryptError::INVALID_KEY_TYPE);
                }
                if (::EVP_PKEY_assign_EC_KEY(pubkeyHandler.handler,ctx)!=1)
                {
                    ::EC_KEY_free(ctx);
                    return makeLastSslError(CryptError::INVALID_KEY_TYPE);
                }
            }
                break;

            case(EVP_PKEY_RSA):
            {
                auto* ctx=::EVP_PKEY_get1_RSA(pkeyNative->nativeHandler().handler);
                if (ctx==nullptr)
                {
                    return makeLastSslError(CryptError::INVALID_KEY_TYPE);
                }
                if (::EVP_PKEY_assign_RSA(pubkeyHandler.handler,ctx)!=1)
                {
                    ::RSA_free(ctx);
                    return makeLastSslError(CryptError::INVALID_KEY_TYPE);
                }
            }
                break;

            case(EVP_PKEY_DSA):
            {
                auto* ctx=::EVP_PKEY_get1_DSA(pkeyNative->nativeHandler().handler);
                if (ctx==nullptr)
                {
                    return makeLastSslError(CryptError::INVALID_KEY_TYPE);
                }
                if (::EVP_PKEY_assign_DSA(pubkeyHandler.handler,ctx)!=1)
                {
                    ::DSA_free(ctx);
                    return makeLastSslError(CryptError::INVALID_KEY_TYPE);
                }
            }
                break;

            default:
            {
                return cryptError(CryptError::INVALID_KEY_TYPE);
            }
        }
    }

    nativeHandler().handler=std::exchange(pubkeyHandler.handler,nullptr);
    return Error();
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
