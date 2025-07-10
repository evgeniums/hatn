/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/encryptedtoken.cpp
  */

/****************************************************************************/

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/context.h>

#include <hatn/crypt/cryptcontainer.h>

#include <hatn/serverapp/encryptedtoken.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

Error EncryptedToken::init(
        const crypt::CipherSuite* suite,
        lib::string_view secret
    )
{
    m_suite=suite;

    crypt::CryptAlgorithmConstP aeadAlg;
    auto ec=m_suite->aeadAlgorithm(aeadAlg);
    HATN_CHECK_CHAIN_LOG_EC(ec,"failed to create find AEAD algorithm")

    auto passphraseKey=suite->createPassphraseKey(ec,aeadAlg);
    HATN_CHECK_CHAIN_LOG_EC(ec,"failed to create passphrase key")
    passphraseKey->set(secret);
    ec=passphraseKey->deriveKey();
    HATN_CHECK_CHAIN_LOG_EC(ec,"failed to derive encryption key")

    m_tokenEncryptionKey=passphraseKey->derivedKeyHolder();
    return OK;
}

//--------------------------------------------------------------------------

Error EncryptedToken::encrypt(
        const common::ByteArray* serializedToken,
        common::ByteArray& encryptedToken,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    HATN_CTX_SCOPE("encryptedtoken::encrypt")

    crypt::CryptContainer cipher{m_tokenEncryptionKey.get(),m_suite,factory};
    auto ec=cipher.pack(*serializedToken,encryptedToken);
    HATN_CHECK_CHAIN_LOG_EC(ec,"failed to pack crypt container")

    return OK;
}

//--------------------------------------------------------------------------

Error EncryptedToken::decrypt(
        const common::ByteArray& encryptedToken,
        common::ByteArray& serializedToken,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    HATN_CTX_SCOPE("encryptedtoken::decrypt")

    crypt::CryptContainer decryptor{factory};
    decryptor.setCipherSuites(m_suite->suites());
    decryptor.setMasterKey(m_tokenEncryptionKey.get());
    auto ec=decryptor.unpack(encryptedToken,serializedToken);
    HATN_CTX_CHECK_EC(ec)

    return OK;
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END
