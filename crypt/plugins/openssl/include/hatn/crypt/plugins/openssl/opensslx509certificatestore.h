/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslx509certificatestore.h
 *
 * 	Wrapper/container of store of trusted X.509 certificates and certificate authorities with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLX509STORE_H
#define HATNOPENSSLX509STORE_H

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/x509.h>

#include <hatn/crypt/x509certificatestore.h>

HATN_OPENSSL_NAMESPACE_BEGIN

namespace detail
{
struct X509StoreTraits
{
    static void free(::X509_STORE* store)
    {
        ::X509_STORE_free(store);
    }
};
struct X509StoreCtxTraits
{
    static void free(::X509_STORE_CTX* ctx)
    {
        ::X509_STORE_CTX_free(ctx);
    }
};
}

//! Wrapper/container of store of trusted X.509 certificates and certificate authorities OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslX509CertificateStore :
            public common::NativeHandlerContainer<X509_STORE,detail::X509StoreTraits,X509CertificateStore,OpenSslX509CertificateStore>
{
    public:

        //! Ctor
        OpenSslX509CertificateStore();

        /**
         * @brief Add Certificate Authority to the store
         * @param crt Certificate Authority
         * @return Error status
         */
        virtual common::Error addCertificate(const X509Certificate& crt) override;

        /**
         * @brief Load certificate to the store from a file
         * @param filename File path
         * @return Error status
         */
        virtual common::Error addCertificate(const char* filename) override;

        /**
         * @brief Set folder with hashed PEM CA
         * @param folder Folder path
         * @return Operation status
         */
        virtual common::Error setCaFolder(const char* folder) override;

        /**
         * @brief Get list of trusted certificates
         * @param ec Error code
         * @return List of trusted certificates
         */
        virtual std::vector<common::SharedPtr<X509Certificate>> certificates(common::Error& ec) const override;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLX509STORE_H
