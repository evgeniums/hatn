/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/sslerror.cpp
  *
  *   SSL/TLS error
  *
  */

/****************************************************************************/

#include <openssl/err.h>

#include <hatn/common/translate.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/************** OpenSslError *****************/

//---------------------------------------------------------------
std::string OpenSslError::codeToString(int code)
{
    static std::map<int,std::string> strings;
    if (strings.empty())
    {
        strings[X509_V_OK]=_TR("No error","SslError");
        strings[X509_V_ERR_UNSPECIFIED]=_TR("Unspecified certificate verification error","SslError");
        strings[X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT]=_TR("Unable to get issuer certificate","SslError");
        strings[X509_V_ERR_UNABLE_TO_GET_CRL]=_TR("Unable to get certificate CRL","SslError");
        strings[X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE]=_TR("Unable to decrypt certificate's signature","SslError");
        strings[X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE]=_TR("Unable to decrypt CRL's signature","SslError");
        strings[X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY]=_TR("Unable to decode issuer public key","SslError");
        strings[X509_V_ERR_CERT_SIGNATURE_FAILURE]=_TR("Certificate signature failure","SslError");
        strings[X509_V_ERR_CRL_SIGNATURE_FAILURE]=_TR("CRL signature failure","SslError");
        strings[X509_V_ERR_CERT_NOT_YET_VALID]=_TR("Certificate is not yet valid","SslError");
        strings[X509_V_ERR_CERT_HAS_EXPIRED]=_TR("Certificate has expired","SslError");
        strings[X509_V_ERR_CRL_NOT_YET_VALID]=_TR("CRL is not yet valid","SslError");
        strings[X509_V_ERR_CRL_HAS_EXPIRED]=_TR("CRL has expired","SslError");
        strings[X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD]=_TR("Format error in certificate's notBefore field","SslError");
        strings[X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD]=_TR("Format error in certificate's notAfter field","SslError");
        strings[X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD]=_TR("Format error in CRL's lastUpdate field","SslError");
        strings[X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD]=_TR("Format error in CRL's nextUpdate field","SslError");
        strings[X509_V_ERR_OUT_OF_MEM]=_TR("Out of memory","SslError");
        strings[X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT]=_TR("Self signed certificate","SslError");
        strings[X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN]=_TR("Self signed certificate in certificate chain","SslError");
        strings[X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY]=_TR("Unable to get local issuer certificate","SslError");
        strings[X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE]=_TR("Unable to verify the first certificate","SslError");
        strings[X509_V_ERR_CERT_CHAIN_TOO_LONG]=_TR("Certificate chain too long","SslError");
        strings[X509_V_ERR_CERT_REVOKED]=_TR("Certificate revoked","SslError");
        strings[X509_V_ERR_INVALID_CA]=_TR("Invalid CA certificate","SslError");
        strings[X509_V_ERR_PATH_LENGTH_EXCEEDED]=_TR("Path length constraint exceeded","SslError");
        strings[X509_V_ERR_INVALID_PURPOSE]=_TR("Unsupported certificate purpose","SslError");
        strings[X509_V_ERR_CERT_UNTRUSTED]=_TR("Certificate not trusted","SslError");
        strings[X509_V_ERR_CERT_REJECTED]=_TR("Certificate rejected","SslError");
        strings[X509_V_ERR_SUBJECT_ISSUER_MISMATCH]=_TR("Subject issuer mismatch","SslError");
        strings[X509_V_ERR_AKID_SKID_MISMATCH]=_TR("Authority and subject key identifier mismatch","SslError");
        strings[X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH]=_TR("Authority and issuer serial number mismatch","SslError");
        strings[X509_V_ERR_KEYUSAGE_NO_CERTSIGN]=_TR("Key usage does not include certificate signing","SslError");
        strings[X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER]=_TR("Unable to get CRL issuer certificate","SslError");
        strings[X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION]=_TR("Unhandled critical extension","SslError");
        strings[X509_V_ERR_KEYUSAGE_NO_CRL_SIGN]=_TR("Key usage does not include CRL signing","SslError");
        strings[X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION]=_TR("Unhandled critical CRL extension","SslError");
        strings[X509_V_ERR_INVALID_NON_CA]=_TR("Invalid non-CA certificate (has CA markings)","SslError");
        strings[X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED]=_TR("Proxy path length constraint exceeded","SslError");
        strings[X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE]=_TR("Key usage does not include digital signature","SslError");
        strings[X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED]=_TR("Proxy certificates not allowed, please set the appropriate flag","SslError");
        strings[X509_V_ERR_INVALID_EXTENSION]=_TR("Invalid or inconsistent certificate extension","SslError");
        strings[X509_V_ERR_INVALID_POLICY_EXTENSION]=_TR("Invalid or inconsistent certificate policy extension","SslError");
        strings[X509_V_ERR_NO_EXPLICIT_POLICY]=_TR("No explicit policy","SslError");
        strings[X509_V_ERR_DIFFERENT_CRL_SCOPE]=_TR("Different CRL scope","SslError");
        strings[X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE]=_TR("Unsupported extension feature","SslError");
        strings[X509_V_ERR_UNNESTED_RESOURCE]=_TR("RFC 3779 resource not subset of parent's resources","SslError");
        strings[X509_V_ERR_PERMITTED_VIOLATION]=_TR("Permitted subtree violation","SslError");
        strings[X509_V_ERR_EXCLUDED_VIOLATION]=_TR("Excluded subtree violation","SslError");
        strings[X509_V_ERR_SUBTREE_MINMAX]=_TR("Name constraints minimum and maximum not supported","SslError");
        strings[X509_V_ERR_APPLICATION_VERIFICATION]=_TR("Application verification failure","SslError");
        strings[X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE]=_TR("Unsupported name constraint type","SslError");
        strings[X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX]=_TR("Unsupported or invalid name constraint syntax","SslError");
        strings[X509_V_ERR_UNSUPPORTED_NAME_SYNTAX]=_TR("Unsupported or invalid name syntax","SslError");
        strings[X509_V_ERR_CRL_PATH_VALIDATION_ERROR]=_TR("CRL path validation error","SslError");
        strings[X509_V_ERR_PATH_LOOP]=_TR("Path Loop","SslError");
        strings[X509_V_ERR_SUITE_B_INVALID_VERSION]=_TR("Suite B: certificate version invalid","SslError");
        strings[X509_V_ERR_SUITE_B_INVALID_ALGORITHM]=_TR("Suite B: invalid public key algorithm","SslError");
        strings[X509_V_ERR_SUITE_B_INVALID_CURVE]=_TR("Suite B: invalid ECC curve","SslError");
        strings[X509_V_ERR_SUITE_B_INVALID_SIGNATURE_ALGORITHM]=_TR("Suite B: invalid signature algorithm","SslError");
        strings[X509_V_ERR_SUITE_B_LOS_NOT_ALLOWED]=_TR("Suite B: curve not allowed for this LOS","SslError");
        strings[X509_V_ERR_SUITE_B_CANNOT_SIGN_P_384_WITH_P_256]=_TR("Suite B: cannot sign P-384 with P-256","SslError");
        strings[X509_V_ERR_HOSTNAME_MISMATCH]=_TR("Hostname mismatch","SslError");
        strings[X509_V_ERR_EMAIL_MISMATCH]=_TR("Email address mismatch","SslError");
        strings[X509_V_ERR_IP_ADDRESS_MISMATCH]=_TR("IP address mismatch","SslError");
        strings[X509_V_ERR_DANE_NO_MATCH]=_TR("No matching DANE TLSA records","SslError");
        strings[X509_V_ERR_EE_KEY_TOO_SMALL]=_TR("EE certificate key too weak","SslError");
        strings[X509_V_ERR_CA_KEY_TOO_SMALL]=_TR("CA certificate key too weak","SslError");
        strings[X509_V_ERR_CA_MD_TOO_WEAK]=_TR("CA signature digest algorithm too weak","SslError");
        strings[X509_V_ERR_INVALID_CALL]=_TR("Invalid certificate verification context","SslError");
        strings[X509_V_ERR_STORE_LOOKUP]=_TR("Issuer certificate lookup error","SslError");
        strings[X509_V_ERR_NO_VALID_SCTS]=_TR("Certificate Transparency required, but no valid SCTs found","SslError");
        strings[X509_V_ERR_PROXY_SUBJECT_NAME_VIOLATION]=_TR("Proxy subject name violation","SslError");
        strings[X509_V_ERR_OCSP_VERIFY_NEEDED]=_TR("OCSP verification needed","SslError");
        strings[X509_V_ERR_OCSP_VERIFY_FAILED]=_TR("OCSP verification failed","SslError");
        strings[X509_V_ERR_OCSP_CERT_UNKNOWN]=_TR("OCSP unknown cert","SslError");
    }

    auto it=strings.find(code);
    if (it==strings.end())
    {
        const char* s = ERR_reason_error_string(code);
        return s ? s : _TR("Unknown TLS error");
    }
    return it->second;
}

/********************** OpenSslErrorCategory **************************/

//---------------------------------------------------------------
OpenSslErrorCategory& OpenSslErrorCategory::getCategory() noexcept
{
    static OpenSslErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------
std::string OpenSslErrorCategory::message(int code) const
{
    return OpenSslError::codeToString(code);
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
