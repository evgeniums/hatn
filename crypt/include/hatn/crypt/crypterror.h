/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/crypterror.h
  *
  *   Definitions for errors of crypt module
  *
  */

/****************************************************************************/

#ifndef HATNCRYPTERROR_H
#define HATNCRYPTERROR_H

#include <hatn/common/error.h>
#include <hatn/common/fixedbytearray.h>

#include <hatn/crypt/crypt.h>

HATN_CRYPT_NAMESPACE_BEGIN

enum class CryptErrorCode : int
{
    OK=static_cast<int>(common::CommonError::OK),
    GENERAL_FAIL,
    ENCRYPTION_FAILED,
    DECRYPTION_FAILED,
    DIGEST_FAILED,
    MAC_FAILED,
    DIGEST_MISMATCH,
    INVALID_CIPHER,
    INVALID_DH_STATE,
    INVALID_KEY_PROTECTION,
    INVALID_CONTENT_FORMAT,
    INVALID_DIGEST,
    INVALID_KEY_LENGTH,
    NOT_SUPPORTED_BY_PLUGIN,
    INVALID_DIGEST_STATE,
    INVALID_MAC_STATE,
    KDF_FAILED,
    INVALID_KEY_TYPE,
    SIGN_FAILED,
    VERIFY_FAILED,
    INVALID_ALGORITHM,
    INVALID_OPERATION,
    INVALID_ENGINE,
    KEY_INITIALIZATION_FAILED,
    KEYS_MISMATCH,
    NOT_PERMITTED,
    INVALID_CIPHER_STATE,
    INVALID_MAC_KEY,
    INVALID_MAC_SIZE,
    DH_FAILED,
    EXPORT_KEY_FAILED,
    INVALID_SIGNATURE_STATE,
    INVALID_SIGNATURE_SIZE,
    NOT_SUPPORTED_BY_CIPHER_SUITE,
    INVALID_KDF_TYPE,
    UNSUPPORTED_FORMAT_VERSION,
    PARSE_CONTAINER_FAILED,
    SERIALIZE_CONTAINER_FAILED,
    INVALID_CIPHER_SUITE,
    INVALID_KEY,
    CIPHER_SUITE_JSON_FAILED,
    CIPHER_SUITE_LOAD_FAILED,
    CIPHER_SUITE_STORE_FAILED,
    DH_PARAMS_NOT_FOUND,
    FILE_STAMP_FAILED,
    INVALID_OBJECT_TYPE,
    X509_STORE_FAILED,
    X509_VERIFICATION_FAILED,
    INVALID_X509_CERTIFICATE,
    X509_PKEY_MISMATCH,
    TLS_ERROR,
    TLS_CLOSED
};

//! Crypt error category
//! @todo Refactor crypt errors
class HATN_CRYPT_EXPORT CryptErrorCategory : public common::ErrorCategory
{
    public:

        //! Name of the category
        virtual const char *name() const noexcept override
        {
            return "hatn.crypt";
        }

        //! Get description for the code
        virtual std::string message(int code) const override;

        //! Get string representation of the code.
        virtual const char* codeString(int /*code*/) const override
        {
            return "";
        }

        //! Get category
        static CryptErrorCategory& getCategory() noexcept;
};

//! Create crypt Error object from error code
inline common::Error makeCryptError(int code) noexcept
{
    return common::Error(code,&CryptErrorCategory::getCategory());
}

//! Create Error object from error code
inline common::Error makeCryptError(CryptErrorCode code) noexcept
{
    return makeCryptError(static_cast<int>(code));
}

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTERROR_H
