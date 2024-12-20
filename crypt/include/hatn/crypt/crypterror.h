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

#define HATN_CRYPT_ERRORS(Do) \
    Do(CryptError,OK,_TR("OK")) \
    Do(CryptError,GENERAL_FAIL,_TR("Request or operation failed","crypt")) \
    Do(CryptError,ENCRYPTION_FAILED,_TR("Failed to encrypt data","crypt")) \
    Do(CryptError,DECRYPTION_FAILED,_TR("Failed to decrypt data","crypt")) \
    Do(CryptError,DIGEST_FAILED,_TR("Failed to calculate digest","crypt")) \
    Do(CryptError,DIGEST_MISMATCH,_TR("Invalid digest","crypt")) \
    Do(CryptError,MAC_FAILED,_TR("Failed to calculate MAC","crypt")) \
    Do(CryptError,INVALID_CONTENT_FORMAT,_TR("Invalid data format","crypt")) \
    Do(CryptError,INVALID_MAC_STATE,_TR("MAC is either not initialized yet or MAC key is not set","crypt")) \
    Do(CryptError,INVALID_CIPHER,_TR("Cipher algorithm is either invalid or not implemented by backend","crypt")) \
    Do(CryptError,INVALID_DH_STATE,_TR("DH is not ready","crypt")) \
    Do(CryptError,INVALID_KEY_PROTECTION,_TR("Key is protected but protector is either not set or invalid","crypt")) \
    Do(CryptError,INVALID_DIGEST,_TR("Digest algorithm is either invalid or not implemented by backend","crypt")) \
    Do(CryptError,INVALID_KEY_LENGTH,_TR("Invalid length of cryptographic key","crypt")) \
    Do(CryptError,NOT_SUPPORTED_BY_PLUGIN,_TR("Requested operation is not supported by cryptographic plugin","crypt")) \
    Do(CryptError,INVALID_DIGEST_STATE,_TR("Digest is not initialized yet","crypt")) \
    Do(CryptError,KDF_FAILED,_TR("Failed to derive key using requested KDF","crypt")) \
    Do(CryptError,SALT_REQUIRED,_TR("Salt required for requested cryptographic operation","crypt")) \
    Do(CryptError,SALT_GEN_FAILED,_TR("Failed to generate salt","crypt")) \
    Do(CryptError,INVALID_KEY_TYPE,_TR("Invalid type of cryptographic key","crypt")) \
    Do(CryptError,SIGN_FAILED,_TR("Failed to create digital signature","crypt")) \
    Do(CryptError,VERIFY_FAILED,_TR("Failed to create cryptographic signature","crypt")) \
    Do(CryptError,INVALID_ALGORITHM,_TR("Invalid cryptographic algorithm","crypt")) \
    Do(CryptError,SUITE_ALGORITHM_NOT_FOUND,_TR("Algorithm not found in cipher suite","crypt")) \
    Do(CryptError,INVALID_OPERATION,_TR("Invalid operation","crypt")) \
    Do(CryptError,INVALID_ENGINE,_TR("Invalid cryptographic engine","crypt")) \
    Do(CryptError,KEY_INITIALIZATION_FAILED,_TR("Failed to generate or initialize a key","crypt")) \
    Do(CryptError,KEYS_MISMATCH,_TR("Keys in a pair mismatch","crypt")) \
    Do(CryptError,NOT_PERMITTED,_TR("Operation not permitted","crypt")) \
    Do(CryptError,INVALID_CIPHER_STATE,_TR("Invalid state of cipher, initialize it first","crypt")) \
    Do(CryptError,INVALID_MAC_KEY,_TR("Invalid MAC key","crypt")) \
    Do(CryptError,INVALID_MAC_SIZE,_TR("MAC size does not match size of digest's hash","crypt")) \
    Do(CryptError,DH_FAILED,_TR("Failed to compute DH secret","crypt")) \
    Do(CryptError,EXPORT_KEY_FAILED,_TR("Failed to export cryptographic key","crypt")) \
    Do(CryptError,INVALID_SIGNATURE_STATE,_TR("Invalid state of digital signature processor","crypt")) \
    Do(CryptError,INVALID_SIGNATURE_SIZE,_TR("Invalid size of digital signature","crypt")) \
    Do(CryptError,NOT_SUPPORTED_BY_CIPHER_SUITE,_TR("Not supported by cipher suite","crypt")) \
    Do(CryptError,INVALID_KDF_TYPE,_TR("Invalid KDF type","crypt")) \
    Do(CryptError,UNSUPPORTED_FORMAT_VERSION,_TR("Unsupported format version","crypt")) \
    Do(CryptError,PARSE_CONTAINER_FAILED,_TR("Failed to parse data in container","crypt")) \
    Do(CryptError,SERIALIZE_CONTAINER_FAILED,_TR("Failed to serialize data to container","crypt")) \
    Do(CryptError,INVALID_CIPHER_SUITE,_TR("Invalid cipher suite","crypt")) \
    Do(CryptError,INVALID_KEY,_TR("Invalid key","crypt")) \
    Do(CryptError,CIPHER_SUITE_JSON_FAILED,_TR("Failed to load cipher suite from JSON","crypt")) \
    Do(CryptError,CIPHER_SUITE_LOAD_FAILED,_TR("Failed to load cipher suite","crypt")) \
    Do(CryptError,CIPHER_SUITE_STORE_FAILED,_TR("Failed to store cipher suite","crypt")) \
    Do(CryptError,DH_PARAMS_NOT_FOUND,_TR("DH parameters not found","crypt")) \
    Do(CryptError,FILE_STAMP_FAILED,_TR("Failed to process file stamp","crypt")) \
    Do(CryptError,INVALID_OBJECT_TYPE,_TR("Invalid object type","crypt")) \
    Do(CryptError,X509_STORE_FAILED,_TR("Failed to invoke operation on X509 certificates store","crypt")) \
    Do(CryptError,X509_VERIFICATION_FAILED,_TR("Failed to verify X509 certificate","crypt")) \
    Do(CryptError,INVALID_X509_CERTIFICATE,_TR("Invalid X509 certificate","crypt")) \
    Do(CryptError,X509_PKEY_MISMATCH,_TR("X509 certificate and private key mismatch","crypt")) \
    Do(CryptError,TLS_ERROR,_TR("TLS error","crypt")) \
    Do(CryptError,TLS_CLOSED,_TR("TLS connection closed","crypt")) \
    Do(CryptError,UNSUPPORTED_KEY_FORMAT,_TR("Unsupported key format","crypt")) \
    Do(CryptError,X509_ERROR_SERIALIZE_FAILED,_TR("Failed to serialize error of X509 certificate verification","crypt")) \
    Do(CryptError,ENGINES_NOT_SUPPORTED,_TR("Cryptographic engines not supported by backend","crypt")) \
    Do(CryptError,INVALID_CRYPTFILE_FORMAT,_TR("Invalid format of crypt file","crypt")) \
    Do(CryptError,KEY_NOT_VALID_FOR_HKDF,_TR("Key can't be used for HKDF","crypt")) \

HATN_CRYPT_NAMESPACE_BEGIN

//! Error codes of hatndataunit lib.
enum class CryptError : int
{
    HATN_CRYPT_ERRORS(HATN_ERROR_CODE)
};

//! Unit errors codes as strings.
constexpr const char* const CryptErrorStrings[] = {
    HATN_CRYPT_ERRORS(HATN_ERROR_STR)
};

//! Unit error code to string.
inline const char* cryptErrorString(CryptError code)
{
    return errorString(code,CryptErrorStrings);
}

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
        virtual const char* codeString(int code) const override;

        //! Get category
        static CryptErrorCategory& getCategory() noexcept;
};

//! Create crypt Error object from error code
inline Error cryptError(CryptError code) noexcept
{
    return Error(code,&CryptErrorCategory::getCategory());
}

/**
 * @brief Create crypt error with some other error on the stack.
 * @param code Error code.
 * @param ec Previous error on the stack.
 * @return New error object.
 */
inline Error cryptError(CryptError code, Error ec) noexcept
{
    //! @todo Implement cryptError from other error
    std::ignore=ec;
    return Error(code,&CryptErrorCategory::getCategory());
}

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTERROR_H
