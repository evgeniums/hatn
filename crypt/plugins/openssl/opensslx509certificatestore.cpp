/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslx509certificatestore.cpp
 * 	Wrapper/container of store of trusted X.509 certificates and certificate authorities with OpenSSL backend
 */
/****************************************************************************/

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslx509certificatestore.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslX509CertificateStore ********************/

//---------------------------------------------------------------
OpenSslX509CertificateStore::OpenSslX509CertificateStore()
{
    setNativeHandler(::X509_STORE_new());
}

//---------------------------------------------------------------
Error OpenSslX509CertificateStore::addCertificate(const X509Certificate &crt)
{
    auto cert=dynamic_cast<const OpenSslX509*>(&crt);
    if (cert==nullptr)
    {
        return cryptError(CryptError::INVALID_OBJECT_TYPE);
    }
    if (::X509_STORE_add_cert(nativeHandler().handler,cert->nativeHandler().handler)!=1)
    {
        return makeLastSslError(CryptError::X509_STORE_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslX509CertificateStore::addCertificate(const char *filename)
{
    if (::X509_STORE_load_locations(nativeHandler().handler,filename,NULL)!=1)
    {
        return makeLastSslError(CryptError::X509_STORE_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslX509CertificateStore::setCaFolder(const char *folder)
{
    if (::X509_STORE_load_locations(nativeHandler().handler,NULL,folder)!=1)
    {
        return makeLastSslError(CryptError::X509_STORE_FAILED);
    }
    return Error();
}

//---------------------------------------------------------------
std::vector<common::SharedPtr<X509Certificate>> OpenSslX509CertificateStore::certificates(Error &ec) const
{
    ec.reset();
    auto crtStack=::X509_STORE_get0_objects(const_cast<X509_STORE*>(nativeHandler().handler));
    auto count=::sk_X509_OBJECT_num(crtStack);
    std::vector<common::SharedPtr<X509Certificate>> certs;
    certs.reserve(count);
    for (decltype(count) i=0;i<count;i++)
    {
        auto nativeObj=sk_X509_OBJECT_value(crtStack,i);
        auto nativeCrt=::X509_OBJECT_get0_X509(nativeObj);
        ::X509_up_ref(nativeCrt);

        auto crt=common::makeShared<OpenSslX509>();
        crt->setNativeHandler(nativeCrt);
        // codechecker_false_positive [performance-move-const-arg] Analyzer can't see push_back with move semantic
        certs.push_back(std::move(crt));
    }
    return certs;
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
