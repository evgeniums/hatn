/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/x509certificatestore.h
 *
 *      Wrapper/container of store of trusted X.509 certificates and certificate authorities
 */
/****************************************************************************/

#ifndef HATNCRYPTX509TRUSTEDSTORE_H
#define HATNCRYPTX509TRUSTEDSTORE_H

#include <hatn/common/error.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/x509certificate.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Wrapper/container of store of trusted X.509 certificates and certificate authorities
class X509CertificateStore
{
    public:

        X509CertificateStore()=default;
        virtual ~X509CertificateStore()=default;
        X509CertificateStore(const X509CertificateStore&)=delete;
        X509CertificateStore(X509CertificateStore&&) =delete;
        X509CertificateStore& operator=(const X509CertificateStore&)=delete;
        X509CertificateStore& operator=(X509CertificateStore&&) =delete;

        /**
         * @brief Add certificate to the store
         * @param crt Certificate Authority
         * @return Error status
         */
        virtual common::Error addCertificate(const X509Certificate& crt) =0;

        /**
         * @brief Load certificate to the store from a file
         * @param filename File path
         * @return Error status
         */
        virtual common::Error addCertificate(const char* filename) =0;

        /**
         * @brief Load certificate to the store from a file
         * @param filename File path
         * @return Error status
         */
        virtual common::Error addCertificate(const std::string& filename)
        {
            return addCertificate(filename.c_str());
        }

        /**
         * @brief Set folder with hashed PEM CA
         * @param folder Folder path
         * @return Operation status
         */
        virtual common::Error setCaFolder(const char* folder) =0;

        /**
         * @brief Set folder with hashed PEM CA
         * @param folder Folder path
         * @return Operation status
         */
        common::Error setCaFolder(const std::string& folder)
        {
            return setCaFolder(folder.c_str());
        }

        /**
         * @brief Get list of trusted certificates
         * @param ec Error code
         * @return List of trusted certificates
         */
        virtual std::vector<common::SharedPtr<X509Certificate>> certificates(common::Error& ec) const =0;

        /**
         * @brief Get list of trusted certificates
         * @return List of trusted certificates
         *
         * @throws common::ErrorException
         */
        inline std::vector<common::SharedPtr<X509Certificate>> certificates() const
        {
            common::Error ec;
            auto certs=certificates(ec);
            if (ec)
            {
                throw common::ErrorException(ec);
            }
            return certs;
        }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTX509TRUSTEDSTORE_H
