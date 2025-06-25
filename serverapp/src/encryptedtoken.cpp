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
    Error ec;
    auto passphraseKey=suite->createPassphraseKey(ec);
    HATN_CHECK_EC(ec)
    passphraseKey->set(secret);
    return OK;
}

//--------------------------------------------------------------------------

Error EncryptedToken::encrypt(
        const common::ByteArray* serializedToken,
        common::ByteArray& encryptedToken,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    crypt::CryptContainer cipher{m_tokenEncryptionKey.get(),m_suite,factory};
    auto ec=cipher.pack(*serializedToken,encryptedToken);
    if (ec)
    {
        //! @todo critical: chain errors
        return ec;
    }

    return OK;
}

//--------------------------------------------------------------------------

Error EncryptedToken::decrypt(
        const common::ByteArray& encryptedToken,
        common::ByteArray& serializedToken,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    crypt::CryptContainer decryptor{factory};
    decryptor.setCipherSuites(m_suite->suites());
    decryptor.setMasterKey(m_tokenEncryptionKey.get());
    auto ec=decryptor.unpack(encryptedToken,serializedToken);
    if (ec)
    {
        //! @todo critical: chain errors
        return ec;
    }

    return OK;
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END
