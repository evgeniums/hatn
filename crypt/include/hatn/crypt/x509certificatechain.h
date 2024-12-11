/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/x509certificatechain.h
 *      Wrapper/container of X.509 certificate chains
 */
/****************************************************************************/

#ifndef HATNCRYPTX509CERTIFICATECHAIN_H
#define HATNCRYPTX509CERTIFICATECHAIN_H

#include <hatn/common/utils.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/fixedbytearray.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/keycontainer.h>

HATN_CRYPT_NAMESPACE_BEGIN

class X509Certificate;

//! Wrapper/container of X.509 certificate chain
class HATN_CRYPT_EXPORT X509CertificateChain : public KeyContainer<common::ByteArray>
{
    public:

        using KeyContainer<common::ByteArray>::KeyContainer;

        virtual ~X509CertificateChain()=default;
        X509CertificateChain(const X509CertificateChain&)=delete;
        X509CertificateChain(X509CertificateChain&&) =delete;
        X509CertificateChain& operator=(const X509CertificateChain&)=delete;
        X509CertificateChain& operator=(X509CertificateChain&&) =delete;

        /**
         * @brief Add certificate to chain
         * @param crt Certificate Authority
         * @return Error status
         */
        virtual common::Error addCertificate(const X509Certificate& crt) =0;

        /**
         * @brief Get list of certificates
         * @param ec Error code
         * @return List of certificates
         */
        virtual std::vector<common::SharedPtr<X509Certificate>> certificates(common::Error& ec) const =0;

        /**
         * @brief Get list of certificates
         * @return List of certificates
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

        /**
         * @brief Load chain from list of certificates
         * @param certs List of certificates
         * @return Operation status
         */
        common::Error loadCerificates(const std::vector<common::SharedPtr<X509Certificate>>& certs);

        /**
         * @brief Load chain from buffer in PEM format
         * @param buf Pointer to buffer with PEM data
         * @param size Size of buffer
         * @param keepContent Keep content in this object
         * @return Error status
         */
        virtual common::Error importFromPemBuf(
            const char* buf,
            size_t size,
            bool keepContent=false
        ) =0;

        /**
         * @brief Load chain from buffer in PEM format
         * @param buf Pointer to buffer with PEM data
         * @param size Size of buffer
         * @param keepContent Keep content in this object
         * @return Error status
         */
        template <typename ContainerT>
        common::Error importFromPemBuf(
            const ContainerT& buf,
            bool keepContent=false
        )
        {
            return importFromPemBuf(buf.data(),buf.size(),keepContent);
        }

        /**
         * @brief Load chain from file
         * @param filename Name of file with PEM content
         * @param keepContent Keep content in this object
         * @return Error status
         */
        inline common::Error loadFromFile(
            const char* filename,
            bool keepContent=false
        )
        {
            common::ByteArray buf;
            HATN_CHECK_RETURN(buf.loadFromFile(filename))
            return importFromPemBuf(buf,keepContent);
        }

        /**
         * @brief Load chain from file
         * @param filename Name of file with PEM content
         * @param keepContent Keep content in this object
         * @return Error status
         */
        inline common::Error loadFromFile(
            const std::string& filename,
            bool keepContent=false
        )
        {
            return loadFromFile(filename.c_str(),keepContent);
        }

        /**
         * @brief Export to buffer in PEM format
         * @param buf Target buffer
         * @return Error status
         */
        virtual common::Error exportToPemBuf(
            common::ByteArray& buf
        ) =0;

        /**
         * @brief Save chain to file
         * @param filename Name of file with PEM content
         * @return Error status
         */
        inline common::Error saveToFile(
            const char* filename
        )
        {
            common::ByteArray buf;
            HATN_CHECK_RETURN(exportToPemBuf(buf))
            return buf.saveToFile(filename);
        }

        /**
         * @brief Save chain to file
         * @param filename Name of file with PEM content
         * @return Error status
         */
        inline common::Error saveToFile(
            const std::string& filename
        )
        {
            return saveToFile(filename.c_str());
        }

        //! Reset chain
        virtual void reset() =0;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTX509CERTIFICATECHAIN_H
