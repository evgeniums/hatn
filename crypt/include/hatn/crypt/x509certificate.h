/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/x509certificate.h
 *      Wrapper/container of X.509 certificate
 */
/****************************************************************************/

#ifndef HATNCRYPTX509CERTIFICATE_H
#define HATNCRYPTX509CERTIFICATE_H

#include <chrono>

#include <hatn/common/error.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/fixedbytearray.h>
#include <hatn/common/nativeerror.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/keycontainer.h>
#include <hatn/crypt/publickey.h>

HATN_CRYPT_NAMESPACE_BEGIN

class PrivateKey;
class X509CertificateStore;
class X509CertificateChain;

/**
 * @brief Wrapper/container of X.509 certificate
 *
 * \todo Check names, check private key, check CA field, extract digest
 */
class HATN_CRYPT_EXPORT X509Certificate : public KeyContainer<common::ByteArray>,
                                              public common::EnableSharedFromThis<X509Certificate>
{
    public:

        using TimePoint=std::chrono::time_point<std::chrono::system_clock>;
        using NameType=common::FixedByteArray256;

        enum class BasicNameField : int
        {
            CommonName,
            Country,
            Organization,
            Locality,
            OrganizationUnit,
            StateOrProvince,
            EmailAddress
        };

        enum class AltNameType : int
        {
            DNS,
            Email,
            URI,
            IP
        };

        using KeyContainer<common::ByteArray>::KeyContainer;

        virtual ~X509Certificate()=default;
        X509Certificate(const X509Certificate&)=delete;
        X509Certificate(X509Certificate&&) =delete;
        X509Certificate& operator=(const X509Certificate&)=delete;
        X509Certificate& operator=(X509Certificate&&) =delete;

        /**
         * @brief Get content of a basic name field of the subject
         * @param field Field ID
         * @param error Put error here or throw
         * @return Field's value
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual std::string subjectNameField(BasicNameField field, common::Error* error=nullptr) const =0;

        /**
         * @brief Get content of a basic name field of the issuer
         * @param error Put error here or throw
         * @return Field's value
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual std::string issuerNameField(BasicNameField field, common::Error* error=nullptr) const =0;

        /**
         * @brief Get subject's alternative names
         * @param type Type of alternative name
         * @param error Put error here or throw
         * @return List of alternative names
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual std::vector<std::string> subjectAltNames(AltNameType type, common::Error* error=nullptr) const =0;

        /**
         * @brief Get issuer's alternative names
         * @param type Type of alternative name
         * @param error Put error here or throw
         * @return List of alternative names
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual std::vector<std::string> issuerAltNames(AltNameType type, common::Error* error=nullptr) const =0;

        /**
         * @brief Get serial number
         * @param error Put error here or throw
         * @return Serial number
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual common::FixedByteArray20 serial(common::Error* error=nullptr) const =0;

        std::string formatSerial(common::Error* error=nullptr) const;
        static std::string formatTime(const TimePoint& tp);

        /**
         * @brief Get time the certificate will be valid starting from
         * @param error Put error here or throw
         * @return TimePoint
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual TimePoint validNotBefore(common::Error* error=nullptr) const =0;

        /**
         * @brief Get time the certificate will be valid up to
         * @param error Put error here or throw
         * @return TimePoint
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual TimePoint validNotAfter(common::Error* error=nullptr) const =0;

        /**
         * @brief Check if certificate is valid for specified date
         * @param tp Date to validate
         * @return Operation status
         */
        inline common::Error isDateValid(const TimePoint& tp=std::chrono::system_clock::now()) const noexcept
        {
            try
            {
                if (tp>=validNotBefore() && tp<=validNotAfter())
                {
                    return common::Error();
                }
            }
            catch (const common::ErrorException& e)
            {
                return e.error();
            }

            return cryptError(CryptError::GENERAL_FAIL);
        }

        /**
         * @brief Get version
         * @param error Put error here or throw
         * @return Version number
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual size_t version(common::Error* error=nullptr) const =0;

        /**
         * @brief Get public key
         * @param error Put error here or throw
         * @return Public key
         *
         * @throws ErrorException if error==nullptr and error occured
         */
        virtual common::SharedPtr<PublicKey> publicKey(common::Error* error=nullptr) const =0;

        /**
         * @brief Import certificate from buffer
         * @param buf Buffer to import from
         * @param size Size of the buffer
         * @param format Data format
         * @return Operation status
         */
        virtual common::Error importFromBuf(const char* buf, size_t size,ContainerFormat format=ContainerFormat::PEM,bool keepContent=true) =0;

        /**
         * @brief Import certificate from buffer
         * @param buf Buffer to import from
         * @param format Data format
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error importFromBuf(
                const ContainerT& buf,
                ContainerFormat format=ContainerFormat::PEM,
                bool keepContent=true
            )
        {
            return importFromBuf(buf.data(),buf.size(),format,keepContent);
        }

        /**
         * @brief Export certificate to buffer in protected format
         * @param buf Buffer to put exported result to
         * @param format Data format
         * @return Operation status
         */
        virtual common::Error exportToBuf(common::ByteArray& buf,ContainerFormat format=ContainerFormat::PEM) const =0;

        /**
         * @brief Load chain from file
         * @param filename Name of file with content
         * @param format Data format
         * @param keepContent Keep content in this object
         * @return Error status
         */
        common::Error loadFromFile(
            const char* filename,
            ContainerFormat format=ContainerFormat::PEM,
            bool keepContent=false
        )
        {
            common::ByteArray buf;
            HATN_CHECK_RETURN(buf.loadFromFile(filename))
            return importFromBuf(buf,format,keepContent);
        }

        /**
         * @brief Load chain from file
         * @param filename Name of file with content
         * @param format Data format
         * @param keepContent Keep content in this object
         * @return Error status
         */
        common::Error loadFromFile(
            const std::string& filename,
                ContainerFormat format=ContainerFormat::PEM,
                bool keepContent=false
        )
        {
            return loadFromFile(filename.c_str(),format,keepContent);
        }

        /**
         * @brief Save chain to file
         * @param filename Name of file with content
         * @param format Data format
         * @return Error status
         */
        common::Error saveToFile(
            const char* filename,
            ContainerFormat format=ContainerFormat::PEM
        )
        {
            common::ByteArray buf;
            HATN_CHECK_RETURN(exportToBuf(buf,format))
            return buf.saveToFile(filename);
        }

        /**
         * @brief Save chain to file
         * @param filename Name of file with content
         * @param format Data format
         * @return Error status
         */
        common::Error saveToFile(
            const std::string& filename,
            ContainerFormat format=ContainerFormat::PEM
        )
        {
            return saveToFile(filename.c_str(),format);
        }

        /**
         * @brief Check if this certificate is consistent with private key
         * @param key Private key
         * @return Operation status
         */
        virtual common::Error checkPrivateKey(const PrivateKey& key) const noexcept
        {
            std::ignore=key;
            return cryptError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
        }

        /**
         * @brief Check if this certificate is issued by other certificate
         * @param ca Certficate authority
         * @return Operation status
         */
        virtual common::Error checkIssuedBy(const X509Certificate& ca) const noexcept =0;

        /**
         * @brief Check if this self signed certificate
         * @return Operation status
         */
        inline bool isSelfSigned() const noexcept
        {
            return !checkIssuedBy(*this);
        }

        /**
         * @brief Verify if this certificate is directly signed by other certificate
         * @param cert Other Certificate
         * @return Operation status
         *
         * Certificate chain is not verified, so the other certificate can be intermediate
         */
        virtual common::Error verify(const X509Certificate& other) const noexcept =0;

        /**
         * @brief Check if this certificate can be verified by the trusted store
         * @param store Store of trusted CA
         * @param chain Chain of CA
         * @return Operation status
         */
        virtual common::Error verify(const X509CertificateStore& store,
                                     const X509CertificateChain* chain=nullptr) const noexcept =0;

        //! Compare two certificates
        inline bool operator==(const X509Certificate& other) const
        {
            auto checkConvertFormat=[](const X509Certificate& crt)
            {
                if (crt.format()!=ContainerFormat::DER || crt.content().isEmpty())
                {
                    auto& crtM=const_cast<X509Certificate&>(crt);
                    if (crt.isNativeNull() && !crt.content().isEmpty())
                    {
                        auto ec=crtM.parseNativeFromContainer();
                        if (ec)
                        {
                            return false;
                        }
                    }
                    if (!crt.isNativeNull())
                    {
                        crtM.setFormat(ContainerFormat::DER);
                        auto ec=crtM.serializeNativeToContainer();
                        if (ec)
                        {
                            return false;
                        }
                    }
                }
                return true;
            };
            if (!checkConvertFormat(*this)||!checkConvertFormat(other))
            {
                return false;
            }
            return  format()==other.format()
                    &&
                    content()==other.content();
        }

        virtual bool isNativeNull() const
        {
            return true;
        }

        //! Serialize certificate to internal container
        inline common::Error serializeNativeToContainer()
        {
            return exportToBuf(content(),format());
        }

        //! Parse certificate from internal container
        inline common::Error parseNativeFromContainer()
        {
            return importFromBuf(content().data(),content().size(),format());
        }

        //! Export to string
        std::string toString(bool pretty=false, bool withKey=true) const;
};

//! X509 verification error
class HATN_CRYPT_EXPORT X509VerifyError : public common::NativeError
{
    public:

        //! Constructor.
        X509VerifyError(
            int nativeCode,
            std::string nativeMessage=std::string{},
            const common::ErrorCategory* category=nullptr
            ) : common::NativeError(std::move(nativeMessage),nativeCode,category)
        {}

        X509VerifyError(
            int nativeCode, //!< Error code
            common::SharedPtr<X509Certificate> certificate, //!< Certificate that caused the error
            std::string nativeMessage=std::string{},
            const common::ErrorCategory* category=nullptr
        )   : common::NativeError(std::move(nativeMessage),nativeCode,category),
              m_certificate(std::move(certificate))
        {}

        //! Set name of mismatched host
        inline void setMismatchedName(
            const char* requestedName,
            const char* verifiedName
        )
        {
            m_mismatchedNameRequested.load(requestedName);
            m_mismatchedNameVerified.load(verifiedName);
        }

        //! Get name of requested mismatched host
        inline const X509Certificate::NameType& requestedMismatchedName() const noexcept
        {
            return m_mismatchedNameRequested;
        }
        //! Get name of verified mismatched host
        inline const X509Certificate::NameType& verifiedMismatchedName() const noexcept
        {
            return m_mismatchedNameVerified;
        }

        common::Error serializeAppend(common::ByteArray& buf) const;

        //! Get certificate associated with the error
        inline const common::SharedPtr<X509Certificate>& certificate() const noexcept
        {
            return m_certificate;
        }

        static common::Error serialize(int32_t code, const X509VerifyError* nativeError, common::ByteArray& buf);
        static common::Error serialize(const common::Error& ec, common::ByteArray& buf);

    protected:

        //! Compare self content with content of other error
        bool compareContent(const common::NativeError& other) const noexcept override;

    private:

        common::SharedPtr<X509Certificate> m_certificate;
        X509Certificate::NameType m_mismatchedNameRequested;
        X509Certificate::NameType m_mismatchedNameVerified;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTX509CERTIFICATE_H
