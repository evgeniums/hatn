/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file crypt/plugins/openssl/opensslcontext.cpp
  *
  *   TLS context with OpenSSL backend
  *
  */

/****************************************************************************/

#include <openssl/evp.h>
#include <openssl/core_names.h>

#include <boost/endian/conversion.hpp>
#include <boost/algorithm/string/join.hpp>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/tls1.h>

#include <hatn/common/logger.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/meta/dynamiccastwithsample.h>

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslx509.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslstream.h>
#include <hatn/crypt/plugins/openssl/opensslecdh.h>
#include <hatn/crypt/plugins/openssl/opensslx509chain.h>
#include <hatn/crypt/plugins/openssl/opensslx509certificatestore.h>
#include <hatn/crypt/plugins/openssl/opensslec.h>
#include <hatn/crypt/plugins/openssl/opensslplugin.h>

#include <hatn/crypt/plugins/openssl/opensslcontext.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//---------------------------------------------------------------
static int createVerifyMode(OpenSslContext::VerifyMode mode, OpenSslContext::EndpointType endpointType)
{
    auto verifyMode=SSL_VERIFY_NONE;
    switch (mode)
    {
        case(SecureStreamContext::VerifyMode::Peer):
            verifyMode=SSL_VERIFY_PEER;
            if (endpointType==OpenSslContext::EndpointType::Server)
            {
                verifyMode|=SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE;
            }
        break;
        default:
            break;
    }
    return verifyMode;
}

//---------------------------------------------------------------
static const SSL_METHOD* selectSslMethod(SecureStreamTypes::Endpoint endpointType)
{
    switch (endpointType)
    {
        case(SecureStreamTypes::Endpoint::Client):
            return ::TLS_client_method();
        break;

        case(SecureStreamTypes::Endpoint::Server):
            return ::TLS_server_method();
        break;

        default:
            return ::TLS_method();
        break;
    }
    return ::TLS_method();
}

/*********************** OpenSslContext **************************/

//---------------------------------------------------------------
OpenSslContext::OpenSslContext(
        EndpointType endpointType,
        VerifyMode verifyMode
    ) : SecureStreamContext(endpointType,verifyMode),
        m_sslCtx(::SSL_CTX_new(selectSslMethod(endpointType))),
        m_nativeVerifyMode(createVerifyMode(verifyMode,endpointType)),
        m_pkeySet(false),
        m_crtSet(false)
{
    if (!m_sslCtx)
    {
        throw common::ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
    }
    ::SSL_CTX_set_min_proto_version(m_sslCtx,TLS1_3_VERSION);
    ::SSL_CTX_set_verify(m_sslCtx,
                         m_nativeVerifyMode,
                         NULL);
}

//---------------------------------------------------------------
OpenSslContext::~OpenSslContext()
{
    ::SSL_CTX_free(m_sslCtx);
}

//---------------------------------------------------------------
common::SharedPtr<SecureStreamV> OpenSslContext::createSecureStream(const lib::string_view& id, common::Thread *thread)
{
    return common::makeShared<SecureStreamTmplV<OpenSslStream>>(this,thread,id);
}

//---------------------------------------------------------------
SecureStreamError OpenSslContext::createError(const common::ByteArray &content) const
{
    int32_t code=0;
    if (content.size()>=sizeof(code))
    {
        boost::endian::little_to_native_inplace(code);
        memcpy(&code,content.data(),sizeof(code));
        if (content.size()==sizeof(code))
        {
            return makeSslError(code);
        }
        return makeSslError(code,common::makeShared<OpenSslX509>(content.data()+sizeof(code),content.size()-sizeof(code),ContainerFormat::DER));
    }
    return SecureStreamError();
}

//---------------------------------------------------------------
Error OpenSslContext::setX509Certificate(const common::SharedPtr<X509Certificate> &certificate) noexcept
{
    OpenSslX509* crt=common::dynamicCastWithSample(certificate.get(),&OpenSslX509::masterReference());
    if (!crt->isValid())
    {
        return cryptError(CryptError::INVALID_X509_CERTIFICATE);
    }

    ::ERR_clear_error();
    if (::SSL_CTX_use_certificate(m_sslCtx,crt->nativeHandler().handler) != 1)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }

    if (crt->isSelfSigned())
    {
        if (::SSL_CTX_build_cert_chain(m_sslCtx,SSL_BUILD_CHAIN_FLAG_CHECK) != 1)
        {
            return makeLastSslError(CryptError::GENERAL_FAIL);
        }
    }
    m_crtSet=true;
    return checkPkeyMatchCrt();
}

//---------------------------------------------------------------
Error OpenSslContext::setX509CertificateChain(const common::SharedPtr<X509CertificateChain> &chain) noexcept
{
    OpenSslX509Chain* ch=common::dynamicCastWithSample(chain.get(),&OpenSslX509Chain::masterReference());
    if (::SSL_CTX_clear_chain_certs(m_sslCtx) != 1)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    if (::SSL_CTX_set1_chain(m_sslCtx,ch->nativeHandler().handler) != 1)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslContext::setX509CertificateStore(const common::SharedPtr<X509CertificateStore> &store) noexcept
{
    OpenSslX509CertificateStore* st=common::dynamicCastWithSample(store.get(),&OpenSslX509CertificateStore::masterReference());
    ::SSL_CTX_set1_cert_store(m_sslCtx,st->nativeHandler().handler);
    return Error();
}

//---------------------------------------------------------------
Error OpenSslContext::setPrivateKey(const common::SharedPtr<PrivateKey> &key) noexcept
{
    OpenSslPrivateKey* nK=common::dynamicCastWithSample(key.get(),&OpenSslPrivateKey::masterReference());
    if (!nK->isValid())
    {
        return cryptError(CryptError::INVALID_KEY);
    }
    if (::SSL_CTX_use_PrivateKey(m_sslCtx,nK->nativeHandler().handler) != 1)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    m_pkeySet=true;
    return checkPkeyMatchCrt();
}

#if OPENSSL_API_LEVEL >= 30100

//---------------------------------------------------------------
Error OpenSslContext::setDH(const common::SharedPtr<DH> &dh) noexcept
{
    auto osslDh=dynamic_cast<OpenSslDH*>(dh.get());
    if (osslDh==nullptr)
    {
        return commonError(CommonError::INVALID_ARGUMENT);
    }

    auto privKey=osslDh->privKey();
    if (!privKey)
    {
        return cryptError(CryptError::INVALID_DH_STATE);
    }
    auto* pKey=privKey->nativeHandler().handler;
    if (pKey==nullptr)
    {
        return cryptError(CryptError::INVALID_DH_STATE);
    }
    auto* pkeyDup=EVP_PKEY_dup(pKey);
    if (pkeyDup==nullptr)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    if (SSL_CTX_set0_tmp_dh_pkey(m_sslCtx,pkeyDup)!=OPENSSL_OK)
    {   EVP_PKEY_free(pkeyDup);
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslContext::setDH(bool enableAuto) noexcept
{
    if (SSL_CTX_set_dh_auto(m_sslCtx,static_cast<int>(enableAuto))!=OPENSSL_OK)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    return Error();
}

#else

//---------------------------------------------------------------
Error OpenSslContext::setDH(const common::SharedPtr<DH> &dh) noexcept
{
    OpenSslDH* nDH=common::dynamicCastWithSample(dh.get(),&OpenSslDH::masterReference());
    if (!nDH->isValid())
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    if (::SSL_CTX_set_tmp_dh(m_sslCtx,nDH->nativeHandler().handler)!=1)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    return Error();
}

#endif

//---------------------------------------------------------------
Error OpenSslContext::setECDHAlgs(const std::vector<const CryptAlgorithm *> &algs) noexcept
{
    std::vector<int> algNIDs;
    for (auto&& it:algs)
    {
        auto alg=dynamic_cast<const ECAlg*>(it);
        if (alg)
        {
            algNIDs.push_back(alg->curveNID());
        }
    }
    if (::SSL_CTX_set1_groups(m_sslCtx,algNIDs.data(),static_cast<int>(algNIDs.size()))!=1)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslContext::setECDHAlgs(const std::vector<std::string> &algs) noexcept
{
    std::string algStr=boost::algorithm::join(algs,":");
    if (::SSL_CTX_set1_groups_list(m_sslCtx,algStr.c_str())!=1)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslContext::setCipherSuites(const std::vector<std::string> &suites) noexcept
{
    std::string suitesStr=boost::algorithm::join(suites,":");
    if (::SSL_CTX_set_ciphersuites(m_sslCtx,suitesStr.c_str())!=1)
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    return Error();
}

//---------------------------------------------------------------
void OpenSslContext::doUpdateVerifyMode()
{
    ::SSL_CTX_set_verify(m_sslCtx,
                         createVerifyMode(verifyMode(),endpointType()),
                         ::SSL_CTX_get_verify_callback(m_sslCtx));
}

//---------------------------------------------------------------
void OpenSslContext::doUpdateEndpointType()
{
    ::ERR_clear_error();

#if OPENSSL_API_LEVEL < 30100

    if (::SSL_CTX_set_ssl_version(m_sslCtx,selectSslMethod(endpointType()))!=1)
    {
        throw ErrorException(makeLastSslError());
    }

#endif

    doUpdateVerifyMode();
}

//---------------------------------------------------------------
Error OpenSslContext::setVerifyDepth(int depth) noexcept
{
    ::SSL_CTX_set_verify_depth(m_sslCtx,depth);
    return Error();
}

#if OPENSSL_API_LEVEL >= 30100

//---------------------------------------------------------------
static int ssl_tlsext_ticket_key_cb(SSL *s, unsigned char key_name[16],
                                    unsigned char iv[EVP_MAX_IV_LENGTH],
                                    EVP_CIPHER_CTX *ctx, EVP_MAC_CTX *hctx, int enc)
{
    OpenSslStreamTraits* streamTraits=static_cast<OpenSslStreamTraits*>(::SSL_get_ex_data(s,OpenSslPlugin::sslCtxIdx()));
    if (!streamTraits)
    {
        return 0;
    }

    if (enc==1)
    {
        const OpenSslSessionTicketKey* key=&streamTraits->ctx()->sessionTicketKey();
        if (!key->isValid())
        {
            return 0;
        }

        if (::RAND_bytes(iv, EVP_MAX_IV_LENGTH) <= 0)
        {
            return -1; /* insufficient random */
        }
        memcpy(&key_name[0],key->name().data(),16);

        if (EVP_EncryptInit_ex(ctx, key->cipher(), NULL, reinterpret_cast<const unsigned char*>(key->cipherKey().data()), iv) != OPENSSL_OK)
        {
            /* error in cipher initialisation */
            return -1;
        }

        OSSL_PARAM params[3];
        params[0] = OSSL_PARAM_construct_octet_string(OSSL_MAC_PARAM_KEY,
                                                      const_cast<char*>(key->macKey().data()),
                                                      key->macKey().size());
        params[1] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST,
                                                     const_cast<char*>(key->hmacName().c_str()),
                                                     0);
        params[2] = OSSL_PARAM_construct_end();
        if (EVP_MAC_CTX_set_params(hctx, params) != OPENSSL_OK)
        {
            /* error in mac initialisation */
            return -1;
        }

        return 1;
    }
    else
    {
        const OpenSslSessionTicketKey* key=&streamTraits->ctx()->sessionTicketKey();
        if (!key->isValid())
        {
            return 0;
        }

        int ret=1;
        if (memcmp(key->name().data(),&key_name[0],16)!=0)
        {
            key=&streamTraits->ctx()->prevSessionTicketKey();
            if (!key->isValid() || memcmp(key->name().data(),&key_name[0],16)!=0)
            {
                return 0;
            }
            ret=2;
        }

        OSSL_PARAM params[3];
        params[0] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY,
                                                      const_cast<char*>(key->macKey().data()),
                                                      key->macKey().size());
        params[1] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST,
                                                     const_cast<char*>(key->hmacName().c_str()),
                                                     0);
        params[2] = OSSL_PARAM_construct_end();
        if (EVP_MAC_CTX_set_params(hctx, params) != OPENSSL_OK)
        {
            /* error in mac initialisation */
            return -1;
        }

        if (EVP_DecryptInit_ex(ctx, key->cipher(), NULL, reinterpret_cast<const unsigned char*>(key->cipherKey().data()),iv) != OPENSSL_OK)
        {
            /* error in cipher initialisation */
            return -1;
        }

        //! @todo Implement ticket expiration
#if 0
        if (key->expire < t - RENEW_TIME) { /* RENEW_TIME: implement */
            /*
             * return 2 - This session will get a new ticket even though the
             * current one is still valid.
             */
            return 2;
        }
#endif

        return ret;
    }

    return 0;
}

//---------------------------------------------------------------
void OpenSslContext::updateSessionTicketEncCb()
{
    if (m_sessionTicketKey.isValid())
    {
        ::SSL_CTX_set_tlsext_ticket_key_evp_cb(m_sslCtx,ssl_tlsext_ticket_key_cb);
    }
    else
    {
        ::SSL_CTX_set_tlsext_ticket_key_evp_cb(m_sslCtx,NULL);
    }

    ::ERR_clear_error();
}

#else

//---------------------------------------------------------------
static int ssl_tlsext_ticket_key_cb(SSL *s, unsigned char key_name[16],
                                    unsigned char *iv, EVP_CIPHER_CTX *ctx,
                                    HMAC_CTX *hctx, int enc)
{
    OpenSslStreamTraits* streamTraits=static_cast<OpenSslStreamTraits*>(::SSL_get_ex_data(s,OpenSslPlugin::sslCtxIdx()));
    if (!streamTraits)
    {
        return 0;
    }

    if (enc==1)
    {
        const OpenSslSessionTicketKey* key=&streamTraits->ctx()->sessionTicketKey();
        if (!key->isValid())
        {
            return 0;
        }

        if (::RAND_bytes(iv, EVP_MAX_IV_LENGTH) <= 0)
        {
            return -1; /* insufficient random */
        }
        memcpy(&key_name[0],key->name().data(),16);

        ::EVP_EncryptInit_ex(ctx, key->cipher(), NULL, reinterpret_cast<const unsigned char*>(key->cipherKey().data()), iv);
        ::HMAC_Init_ex(hctx, key->macKey().data(),SessionTicketKey::HMAC_NAME_MAXLEN, key->hmac(), NULL);

        return 1;
    }
    else
    {
        const OpenSslSessionTicketKey* key=&streamTraits->ctx()->sessionTicketKey();
        if (!key->isValid())
        {
            return 0;
        }

        int ret=1;
        if (memcmp(key->name().data(),&key_name[0],16)!=0)
        {
            key=&streamTraits->ctx()->prevSessionTicketKey();
            if (!key->isValid() || memcmp(key->name().data(),&key_name[0],16)!=0)
            {
                return 0;
            }
            ret=2;
        }

        ::HMAC_Init_ex(hctx, key->macKey().data(),SessionTicketKey::HMAC_NAME_MAXLEN, key->hmac(), NULL);
        ::EVP_DecryptInit_ex(ctx, key->cipher(), NULL, reinterpret_cast<const unsigned char*>(key->cipherKey().data()), iv);

        return ret;
    }

    return 0;
}

//---------------------------------------------------------------
void OpenSslContext::updateSessionTicketEncCb()
{
    if (m_sessionTicketKey.isValid())
    {
        ::SSL_CTX_set_tlsext_ticket_key_cb(m_sslCtx,ssl_tlsext_ticket_key_cb);
    }
    else
    {
        ::SSL_CTX_set_tlsext_ticket_key_cb(m_sslCtx,NULL);
    }

    ::ERR_clear_error();
}

#endif

//---------------------------------------------------------------
void OpenSslContext::doUpdateProtocolVersion()
{
    auto toNative=[](SecureStreamTypes::ProtocolVersion version)
    {
        int nativeVersion=TLS1_3_VERSION;
        switch (version)
        {
            case(SecureStreamTypes::ProtocolVersion::TLS1_3):
            break;
            case(SecureStreamTypes::ProtocolVersion::TLS1_2):
                nativeVersion=TLS1_2_VERSION;
            break;
            case(SecureStreamTypes::ProtocolVersion::TLS1_1):
                nativeVersion=TLS1_1_VERSION;
            break;
            case(SecureStreamTypes::ProtocolVersion::TLS1):
                nativeVersion=TLS1_VERSION;
            break;
            default:
                break;
        }
        return nativeVersion;
    };

    auto minVersion=minProtocolVersion();
    auto maxVersion=maxProtocolVersion();
    ::SSL_CTX_set_min_proto_version(m_sslCtx,toNative(minVersion));
    ::SSL_CTX_set_max_proto_version(m_sslCtx,toNative(maxVersion));
}

//---------------------------------------------------------------
common::Error OpenSslContext::loadSessionTicketKey(const char *buf, size_t size, bool keepPrev, KeyProtector *protector) noexcept
{
    try
    {
        if (size==0 || buf==nullptr)
        {
            m_sessionTicketKey.reset();
            m_prevSessionTicketKey.reset();
            updateSessionTicketEncCb();
        }
        else
        {
            if (keepPrev && m_sessionTicketKey.isValid())
            {
                m_prevSessionTicketKey=m_sessionTicketKey;
            }
            else
            {
                m_prevSessionTicketKey.reset();
            }

            auto ec=m_sessionTicketKey.load(buf,size,protector);
            if (ec)
            {
                return ec;
            }
            updateSessionTicketEncCb();
        }
    }
    catch (const std::overflow_error&)
    {
        return Error(CommonError::INVALID_SIZE);
    }
    catch (...)
    {
        return Error(CommonError::UNKNOWN);
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslContext::checkPkeyMatchCrt()
{
    if (!m_crtSet||!m_pkeySet)
    {
        return Error();
    }

    if (::SSL_CTX_check_private_key(m_sslCtx)!=1)
    {
        return makeLastSslError(CryptError::X509_PKEY_MISMATCH);
    }

    return Error();
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
