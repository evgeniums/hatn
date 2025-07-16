/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/clientauthprotocolsharedsecret.сpp
  *
  */

#include <hatn/logcontext/contextlogger.h>

#include <hatn/clientserver/auth/clientauthprotocolsharedsecret.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

namespace {

Result<std::tuple<const crypt::CipherSuite*,const crypt::CryptAlgorithm*>> prepareMAC(
        const crypt::CipherSuites* cipherSuites,
        lib::string_view cipherSuiteId
    )
{
    HATN_CTX_SCOPE("preparemac")

    // find cipher suite
    const auto* suite=cipherSuites->suite(cipherSuiteId);
    if (suite==nullptr)
    {
        HATN_CTX_SCOPE_LOCK()
        auto ec=crypt::cryptError(crypt::CryptError::UNKNOWN_CIPHER_SUITE);
        HATN_CTX_CHECK_EC(ec)
    }

    // find MAC algorithm
    crypt::CryptAlgorithmConstP macAlg;
    auto ec=suite->macAlgorithm(macAlg);
    HATN_CTX_CHECK_EC_MSG(ec,"failed to find MAC alg in suite")

    // done
    return std::make_tuple(suite,macAlg);
}

}

//--------------------------------------------------------------------------

ClientAuthProtocolSharedSecretImpl::ClientAuthProtocolSharedSecretImpl(const crypt::CipherSuites* cipherSuites)
    : ClientAuthProtocol(
          AUTH_PROTOCOL_HATN_SHARED_SECRET,
          AUTH_PROTOCOL_HATN_SHARED_SECRET_VERSION
          ),
    m_cipherSuites(cipherSuites)
{
    if (m_cipherSuites==nullptr)
    {
        m_cipherSuites=&crypt::CipherSuitesGlobal::instance();
    }
}

//--------------------------------------------------------------------------

Result<crypt::SecurePlainContainer> ClientAuthProtocolSharedSecretImpl::calculateSharedSecret(
        lib::string_view login,
        lib::string_view password,
        lib::string_view cipherSuiteId
    ) const
{
    HATN_CTX_SCOPE("calcsharedsecret")
    HATN_CTX_SCOPE_PUSH("cipher_suite",cipherSuiteId)

    // prepare MAC
    auto r=prepareMAC(m_cipherSuites,cipherSuiteId);
    HATN_CHECK_RESULT(r)
    const auto* suite=std::get<0>(*r);
    const auto& macAlg=std::get<1>(*r);

    // create passphrase key
    Error ec;
    auto key=suite->createPassphraseKey(ec,macAlg);
    HATN_CTX_CHECK_EC_MSG(ec,"failed to create passphrase key")
    key->set(password);
    key->setSalt(login);

    // derive shared secret
    ec=key->deriveKey();
    HATN_CTX_CHECK_EC_MSG(ec,"failed to derive MAC key")

    // export shared secret to container
    crypt::SecurePlainContainer secretContainer;
    ec=key->derivedKey()->exportToBuf(secretContainer.content(),crypt::ContainerFormat::RAW_PLAIN);
    HATN_CTX_CHECK_EC_MSG(ec,"failed to export shared secret key to container")

    // done
    return secretContainer;
}

//--------------------------------------------------------------------------

Error ClientAuthProtocolSharedSecretImpl::calculateMAC(
        lib::string_view challenge,
        common::ByteArray& targetBuf,
        lib::string_view cipherSuiteId
    ) const
{
    HATN_CTX_SCOPE("calcsharedsecretmac")
    HATN_CTX_SCOPE_PUSH("cipher_suite",cipherSuiteId)

    // prepare MAC
    auto r=prepareMAC(m_cipherSuites,cipherSuiteId);
    HATN_CHECK_RESULT(r)
    const auto* suite=std::get<0>(*r);
    const auto& macAlg=std::get<1>(*r);
    auto macKey=macAlg->createMACKey();
    macKey->loadContent(m_sharedSecret);

    // create MAC processor
    Error ec;
    auto mac=suite->createMAC(ec);
    HATN_CTX_CHECK_EC_MSG(ec,"failed to create MAC processor")
    mac->setKey(macKey.get());

    // calculate MAC
    ec=mac->runSign(challenge,targetBuf);
    HATN_CTX_CHECK_EC_MSG(ec,"failed to calculate MAC")

    // done
    return OK;
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
