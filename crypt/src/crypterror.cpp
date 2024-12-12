/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/crypterror.cpp
  *
  *   Definitions for errors of crypt module
  *
  */

/****************************************************************************/

#include <hatn/common/translate.h>

#include <hatn/crypt/crypterror.h>

namespace hatn {

using namespace common;

namespace crypt {

inline std::string _TR(
    const std::string& phrase,
    const std::string& context="generic"
    )
{
    return common::_TR(phrase,context);
}

/********************** CryptErrorCategory **************************/

static CryptErrorCategory CryptErrorCategoryInstance;

//---------------------------------------------------------------
CryptErrorCategory& CryptErrorCategory::getCategory() noexcept
{
    return CryptErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string CryptErrorCategory::message(int code) const
{
    std::string result;
    if (result.empty())
    {
        switch (code)
        {
            case (static_cast<int>(CryptErrorCode::OK)):
                result=CommonErrorCategory::getCategory().message(code);
            break;

            case (static_cast<int>(CryptErrorCode::GENERAL_FAIL)):
                result=_TR("Request or operation failed");
            break;
            case (static_cast<int>(CryptErrorCode::ENCRYPTION_FAILED)):
                result=_TR("Failed to encrypt data");
            break;
            case (static_cast<int>(CryptErrorCode::DECRYPTION_FAILED)):
                result=_TR("Failed to decrypt data");
            break;
            case (static_cast<int>(CryptErrorCode::DIGEST_FAILED)):
                result=_TR("Failed to calculate digest");
            break;
            case (static_cast<int>(CryptErrorCode::DIGEST_MISMATCH)):
                result=_TR("Calculated data and check value of digest mismatch");
            break;
            case (static_cast<int>(CryptErrorCode::MAC_FAILED)):
                result=_TR("HMAC calculating failed");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_CONTENT_FORMAT)):
                result=_TR("Invalid format of container's content");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_MAC_STATE)):
                result=_TR("HMAC is either not initialized yet or HMAC key is not set");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_CIPHER)):
                result=_TR("Cipher algorithm is either invalid or not implemented by backend");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_DH_STATE)):
                result=_TR("DH is not ready");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_KEY_PROTECTION)):
                result=_TR("Key is protected but protector is either not set or invalid");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_DIGEST)):
                result=_TR("Digest algorithm is either invalid or not implemented by backend");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_KEY_LENGTH)):
                result=_TR("Invalid length of cryptographic key");
            break;
            case (static_cast<int>(CryptErrorCode::NOT_SUPPORTED_BY_PLUGIN)):
                result=_TR("Requested operation is not supported by cryptographic plugin");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_DIGEST_STATE)):
                result=_TR("Digest is not initialized yet");
            break;
            case (static_cast<int>(CryptErrorCode::KDF_FAILED)):
                result=_TR("Failed to derive key using requested KDF");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_KEY_TYPE)):
                result=_TR("Invalid type of key");
            break;
            case (static_cast<int>(CryptErrorCode::SIGN_FAILED)):
                result=_TR("Failed to sign");
            break;
            case (static_cast<int>(CryptErrorCode::VERIFY_FAILED)):
                result=_TR("Verify failed");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_ALGORITHM)):
                result=_TR("Invalid algorithm");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_OPERATION)):
                result=_TR("Invalid operation");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_ENGINE)):
                result=_TR("Invalid engine");
            break;
            case (static_cast<int>(CryptErrorCode::KEY_INITIALIZATION_FAILED)):
                result=_TR("Failed to generate or initialize a key");
            break;
            case (static_cast<int>(CryptErrorCode::KEYS_MISMATCH)):
                result=_TR("Keys in a pair mismatch");
            break;
            case (static_cast<int>(CryptErrorCode::NOT_PERMITTED)):
                result=_TR("Operation is not permitted");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_CIPHER_STATE)):
                result=_TR("Invalid state of cipher, try to initialize it first");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_MAC_KEY)):
                result=_TR("Invalid HMAC key");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_MAC_SIZE)):
                result=_TR("HMAC size exceeds size of digest's hash");
            break;
            case (static_cast<int>(CryptErrorCode::DH_FAILED)):
                result=_TR("Failed to compute DH secret");
            break;
            case (static_cast<int>(CryptErrorCode::EXPORT_KEY_FAILED)):
                result=_TR("Failed to export key");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_SIGNATURE_STATE)):
                result=_TR("Invalid state of signature processor");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_SIGNATURE_SIZE)):
                result=_TR("Invalid size of signature");
            break;
            case (static_cast<int>(CryptErrorCode::NOT_SUPPORTED_BY_CIPHER_SUITE)):
                result=_TR("Not supported by cipher suite");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_KDF_TYPE)):
                result=_TR("Invalid KDF type");
            break;
            case (static_cast<int>(CryptErrorCode::UNSUPPORTED_FORMAT_VERSION)):
                result=_TR("Unsupported format version");
            break;
            case (static_cast<int>(CryptErrorCode::PARSE_CONTAINER_FAILED)):
                result=_TR("Failed to parse container");
            break;
            case (static_cast<int>(CryptErrorCode::SERIALIZE_CONTAINER_FAILED)):
                result=_TR("Failed to serialize container");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_CIPHER_SUITE)):
                result=_TR("Invalid cipher suite");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_KEY)):
                result=_TR("Invalid key");
            break;
            case (static_cast<int>(CryptErrorCode::CIPHER_SUITE_JSON_FAILED)):
                result=_TR("Failed to load cipher suite from JSON");
            break;
            case (static_cast<int>(CryptErrorCode::CIPHER_SUITE_LOAD_FAILED)):
                result=_TR("Failed to load cipher suite");
            break;
            case (static_cast<int>(CryptErrorCode::CIPHER_SUITE_STORE_FAILED)):
                result=_TR("Failed to store cipher suite");
            break;
            case (static_cast<int>(CryptErrorCode::DH_PARAMS_NOT_FOUND)):
                result=_TR("DH parameters not found");
            break;
            case (static_cast<int>(CryptErrorCode::FILE_STAMP_FAILED)):
                result=_TR("Failed to process file stamp");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_OBJECT_TYPE)):
                result=_TR("Invalid object type");
            break;
            case (static_cast<int>(CryptErrorCode::X509_STORE_FAILED)):
                result=_TR("Failed operation on X509 certificates store");
            break;
            case (static_cast<int>(CryptErrorCode::X509_VERIFICATION_FAILED)):
                result=_TR("Failed to verify X509 certificate");
            break;
            case (static_cast<int>(CryptErrorCode::INVALID_X509_CERTIFICATE)):
                result=_TR("Invalid X509 certificate");
            break;
            case (static_cast<int>(CryptErrorCode::X509_PKEY_MISMATCH)):
                result=_TR("X509 certificate and private key mismatch");
            break;
            case (static_cast<int>(CryptErrorCode::TLS_ERROR)):
                result=_TR("TLS error");
            break;
            case (static_cast<int>(CryptErrorCode::TLS_CLOSED)):
                result=_TR("TLS connection closed");
            break;
            case (static_cast<int>(CryptErrorCode::X509_ERROR_SERIALIZE_FAILED)):
                result=_TR("Failed to serialize error of X509 certificate verification");
                break;

            default:
                result=_TR("Unknown error");
        }
    }
    return result;
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
