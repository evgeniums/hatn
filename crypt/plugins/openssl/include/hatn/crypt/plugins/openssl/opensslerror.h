/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslerror.h
  *
  *   SSL/TLS error
  *
  */

/****************************************************************************/

#ifndef HATNOPENSSLERROR_H
#define HATNOPENSSLERROR_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/conf.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <hatn/common/error.h>
#include <hatn/common/fixedbytearray.h>

#include <hatn/crypt/crypterror.h>

#include <hatn/crypt/plugins/openssl/opensslx509.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! SSL/TLS error category
class HATN_OPENSSL_EXPORT OpenSslErrorCategory : public common::ErrorCategory
{
    public:

        //! Name of the category
        virtual const char *name() const noexcept override
        {
            return "hatn.openssl";
        }

        //! Get description for the code
        virtual std::string message(int code) const override;

        //! Get category
        static OpenSslErrorCategory& getCategory() noexcept;

        virtual const char* codeString(int code) const override
        {
            //! @todo Refactor OpenSslErrorCategory
            return "";
        }
};

//! SSL/TLS error
class HATN_OPENSSL_EXPORT OpenSslError final : public X509VerifyError
{
    public:

        OpenSslError(int nativeCode)
            : X509VerifyError(
                    nativeCode=X509_V_OK,
                    codeToString(nativeCode),
                    &OpenSslErrorCategory::getCategory()
                )
        {}

        OpenSslError(
                int nativeCode,
                common::SharedPtr<X509Certificate> certificate
            ) : X509VerifyError(
                    nativeCode=X509_V_OK,
                    std::move(certificate),
                    codeToString(nativeCode),
                    &OpenSslErrorCategory::getCategory()
                )
        {}

        // //! Check if error is NULL
        // bool isNull() const noexcept override
        // {
        //     return nativeCode()==X509_V_OK;
        // }

        /**
         * @brief Init string messages
         * Call it once at the beggining to initialize error messages
         * @attention Not thread safe
         *
         * @todo Refactor it
         */
        static inline void loadStrings()
        {
            codeToString();
        }

        /**
         * @brief Get string description of error
         * @param code Error code
         * @return Error string description
         */
        static std::string codeToString(int code=X509_V_OK);
};

//! Create Error object from openssl error
inline common::Error makeSslError(int code) noexcept
{
    if (code==X509_V_OK)
    {
        return common::Error();
    }
    return common::Error(code,&OpenSslErrorCategory::getCategory());
}

//! Create Error object from last openssl error
inline common::Error makeLastSslError() noexcept
{
    int code=::ERR_get_error();
    if (code==X509_V_OK)
    {
        return common::Error();
    }
    ::ERR_clear_error();
    return common::Error(code,&OpenSslErrorCategory::getCategory());
}

//! Create Error object from last openssl error
inline common::Error makeLastSslError(CryptError fallBackCode) noexcept
{
    int code=::ERR_get_error();
    if (code==X509_V_OK)
    {
        return cryptError(fallBackCode);
    }
    ::ERR_clear_error();
    return common::Error(code,&OpenSslErrorCategory::getCategory());
}

//! Create Error object from openssl error with certificate content
inline common::Error makeSslError(int code, common::SharedPtr<OpenSslX509> certificate) noexcept
{
    if (code==X509_V_OK)
    {
        return common::Error();
    }
    return common::Error(code,std::make_shared<OpenSslError>(code,std::move(certificate)));
}

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLERROR_H
