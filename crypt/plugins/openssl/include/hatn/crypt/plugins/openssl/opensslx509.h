/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslx509certificate.h
 * 	X.509 certificate wrapper with OpenSSL backend
 */
/****************************************************************************/

#ifndef HATNOPENSSLX509_H
#define HATNOPENSSLX509_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/x509.h>
#include <openssl/pem.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/x509certificate.h>
#include <hatn/crypt/publickey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

namespace detail
{
struct X509Traits
{
    static void free(X509* crt)
    {
        ::X509_free(crt);
    }
};
}

//! X.509 certificate with OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslX509 :
    public common::NativeHandlerContainer<::X509,detail::X509Traits,X509Certificate,OpenSslX509>
{
    public:

        using common::NativeHandlerContainer<::X509,detail::X509Traits,X509Certificate,OpenSslX509>::NativeHandlerContainer;

        /**
         * @brief Parse data to native object
         * @param crt Parsed native object
         * @param data Pointer to buffer
         * @param size Size of data
         * @param format Format of data
         * @param passwdCallback Password callback for encrypted files
         * @param passwdCallbackUserdata Data for password callback
         * @return Operation status
         */
        static Error parse(
            Native& crt,
            const char* data,
            size_t size,
            ContainerFormat format=ContainerFormat::PEM,
            pem_password_cb* passwdCallback = nullptr,
            void* passwdCallbackUserdata = nullptr
        ) noexcept;

        /**
         * @brief Parse certificate from content to m_crt
         * @param passwdCallback Password callback for encrypted files
         * @param passwdCallbackUserdata Data for password callback
         * @return Operation status
         */
        inline Error parse(
            pem_password_cb* passwdCallback = nullptr,
            void* passwdCallbackUserdata = nullptr
        ) noexcept
        {
            return parse(nativeHandler(),content().constData(),content().size(),format(),passwdCallback,passwdCallbackUserdata);
        }

        /**
         * @brief Serialize certificate to ByteArray
         * @param crt Native certificate object
         * @param content Target ByteArray content
         * @param format Container format
         * @return Operation status
         */
        static Error serialize(
            const Native& native,
            common::ByteArray& content,
            ContainerFormat format=ContainerFormat::PEM
        );

        /**
         * @brief Serialize certificate to internal content
         * @param format Container format
         * @return Operation status
         */
        inline Error serialize(
            ContainerFormat format=ContainerFormat::PEM
        )
        {
            return serialize(nativeHandler(),content(),format);
        }

        /**
         * @brief Import certificate from buffer
         * @param buf Buffer to import from
         * @param size Size of the buffer
         * @param format Data format
         * @return Operation status
         */
        virtual common::Error importFromBuf(const char* buf, size_t size,ContainerFormat format=ContainerFormat::PEM,bool keepContent=true) override
        {
            HATN_CHECK_RETURN(parse(nativeHandler(),buf,size,format))

            if (keepContent)
            {
                loadContent(buf,size);
            }

            return Error();
        }

        /**
         * @brief Export certificate to buffer in protected format
         * @param buf Buffer to put exported result to
         * @param format Data format
         * @return Operation status
         */
        virtual common::Error exportToBuf(common::ByteArray& buf,ContainerFormat format=ContainerFormat::PEM) const override
        {
            return serialize(nativeHandler(),buf,format);
        }

        /**
         * @brief Check if this certificate is consistent with private key
         * @param key Private key
         * @return Operation status
         */
        virtual common::Error checkPrivateKey(const PrivateKey& key) const noexcept override;

        /**
         * @brief Check if this certificate is issued by other certificate
         * @param ca Certficate authority
         * @return Operation status
         */
        virtual common::Error checkIssuedBy(const X509Certificate& ca) const noexcept override;

        /**
         * @brief Get time the certificate will be valid starting from
         * @param error Put error here or throw
         * @return TimePoint
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual TimePoint validNotBefore(common::Error* error=nullptr) const override;

        /**
         * @brief Get time the certificate will be valid up to
         * @param error Put error here or throw
         * @return TimePoint
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual TimePoint validNotAfter(common::Error* error=nullptr) const override;

        /**
         * @brief Get version
         * @param error Put error here or throw
         * @return Version number
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual size_t version(common::Error* error=nullptr) const override;

        /**
         * @brief Get serial number
         * @param error Put error here or throw
         * @return Serial number
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual common::FixedByteArray20 serial(common::Error* error=nullptr) const override;

        /**
         * @brief Get public key
         * @param error Put error here or throw
         * @return Public key
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual common::SharedPtr<PublicKey> publicKey(common::Error* error=nullptr) const override;

        /**
         * @brief Get content of a basic name field of the subject
         * @param field Field ID
         * @param error Put error here or throw
         * @return Field's value
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual std::string subjectNameField(BasicNameField field, common::Error* error=nullptr) const override;

        /**
         * @brief Get content of a basic name field of the issuer
         * @param error Put error here or throw
         * @return Field's value
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual std::string issuerNameField(BasicNameField field, common::Error* error=nullptr) const override;

        /**
         * @brief Get subject's alternative names
         * @param type Type of alternative name
         * @param error Put error here or throw
         * @return List of alternative names
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual std::vector<std::string> subjectAltNames(AltNameType type, common::Error* error=nullptr) const override;

        /**
         * @brief Get issuer's alternative names
         * @param type Type of alternative name
         * @param error Put error here or throw
         * @return List of alternative names
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual std::vector<std::string> issuerAltNames(AltNameType type, common::Error* error=nullptr) const override;

        /**
         * @brief Verify if this certificate is directly signed by other certificate
         * @param cert Other Certificate
         * @return Operation status
         *
         * Certificate chain is not verified, so the other certificate can be intermediate
         */
        virtual common::Error verify(const X509Certificate& otherCert) const noexcept override;

        /**
         * @brief Check if this certificate can be verified by the trusted store
         * @param store Store of trusted CA
         * @param chain Chain of CA
         * @return Operation status
         */
        virtual common::Error verify(const X509CertificateStore& store,
                                     const X509CertificateChain* chain=nullptr) const noexcept override;

        virtual bool isNativeNull() const override;

        Error checkValidOrParse() const;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLX509_H
