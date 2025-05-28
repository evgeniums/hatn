/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslx509certificate.cpp
 * 	X.509 certificate parser
 */
/****************************************************************************/

#include <hatn/common/utils.h>
#include <hatn/common/logger.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslx509.h>
#include <hatn/crypt/plugins/openssl/opensslx509chain.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslX509Chain ********************/

//---------------------------------------------------------------
Error OpenSslX509Chain::parse(
        OpenSslX509Chain* chain,
        const char *data,
        size_t size
    ) noexcept
{
    chain->reset();

    if (size==0)
    {
        return Error();
    }

    BIO* bio=NULL;
    HATN_CHECK_RETURN(createMemBio(bio,data,size));

    X509 *ca=NULL;
    while ((ca = ::PEM_read_bio_X509(bio,NULL,NULL,NULL))!=NULL)
    {
        if (::ERR_peek_error() != 0)
        {
            ::BIO_free(bio);
            chain->reset();
            return makeLastSslError(CryptError::GENERAL_FAIL);
        }
        ::sk_X509_push(chain->nativeHandler().handler,ca);
    }

    ::BIO_free(bio);
    return Error();
}

//---------------------------------------------------------------
Error OpenSslX509Chain::serialize(const OpenSslX509Chain* chain, common::ByteArray &content)
{
    BIO *bio = ::BIO_new(::BIO_s_mem());
    if (bio==NULL)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }

    auto count=::sk_X509_num(chain->nativeHandler().handler);
    for (decltype(count) i=0;i<count;i++)
    {
        if (PEM_write_bio_X509(bio,sk_X509_value(chain->nativeHandler().handler,i))!=1)
        {
            BIO_free(bio);
            return makeLastSslError(CryptError::GENERAL_FAIL);
        }
    }

    readMemBio(bio,content);
    return Error();
}

//---------------------------------------------------------------
Error OpenSslX509Chain::exportToPemBuf(common::ByteArray &buf)
{
    createNativeHandlerIfNull();
    return serialize(this,buf);
}

//---------------------------------------------------------------
Error OpenSslX509Chain::importFromPemBuf(const char *buf, size_t size, bool keepContent)
{
    HATN_CHECK_RETURN(parse(this,buf,size))
    if (keepContent)
    {
        loadContent(buf,size);
    }
    return Error();
}

//---------------------------------------------------------------
void OpenSslX509Chain::reset()
{
    resetNative();
    setNativeHandler(sk_X509_new_null());
}

//---------------------------------------------------------------
Error OpenSslX509Chain::addCertificate(const X509Certificate &crt)
{
    createNativeHandlerIfNull();

    auto cert=dynamic_cast<const OpenSslX509*>(&crt);
    if (cert==nullptr)
    {
        return cryptError(CryptError::INVALID_OBJECT_TYPE);
    }

    auto nativeCrt=cert->nativeHandler().handler;

    //! @todo critical: Check errors
    ::X509_up_ref(nativeCrt);
    ::sk_X509_push(nativeHandler().handler,nativeCrt);

    return Error();
}

//---------------------------------------------------------------
std::vector<common::SharedPtr<X509Certificate>> OpenSslX509Chain::certificates(Error &ec) const
{
    const_cast<OpenSslX509Chain*>(this)->createNativeHandlerIfNull();

    ec.reset();
    auto count=::sk_X509_num(nativeHandler().handler);
    std::vector<common::SharedPtr<X509Certificate>> certs;
    certs.reserve(count);
    for (decltype(count) i=0;i<count;i++)
    {
        auto nativeCrt=sk_X509_value(nativeHandler().handler,i);
        ::X509_up_ref(nativeCrt);

        auto&& crt=common::makeShared<OpenSslX509>();
        crt->setNativeHandler(nativeCrt);
        // codechecker_false_positive [performance-move-const-arg] Analyzer can't see push_back with move semantic
        certs.push_back(std::move(crt));
    }
    return certs;
}

//---------------------------------------------------------------
void OpenSslX509Chain::createNativeHandlerIfNull()
{
    if (!isValid())
    {
        setNativeHandler(sk_X509_new_null());
    }
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
