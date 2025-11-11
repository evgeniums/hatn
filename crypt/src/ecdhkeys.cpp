/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/ecdhkeys.cpp
 *
 */
/****************************************************************************/

#include <hatn/crypt/ecdhkeys.h>

HATN_CRYPT_NAMESPACE_BEGIN

//---------------------------------------------------------------

Error ECDHKeys::init(const CipherSuite* suite, common::SharedPtr<KeyProtector> keyProtector, const common::pmr::AllocatorFactory *factory)
{
    m_suite=suite;
    m_factory=factory;

    Error ec;
    m_keyProtector=std::move(keyProtector);
    m_processor=suite->createECDH(ec);
    HATN_CHECK_EC(ec)

    return OK;
}

//---------------------------------------------------------------

Error ECDHKeys::init(const CipherSuite* suite, common::SharedPtr<PassphraseKey> passphrase, const common::pmr::AllocatorFactory *factory)
{
    auto protector=common::makeShared<KeyProtector>(std::move(passphrase),suite,factory);
    return init(suite,std::move(protector),factory);
}

//---------------------------------------------------------------

Error ECDHKeys::init(const CipherSuite* suite, lib::string_view passphrase, const common::pmr::AllocatorFactory *factory)
{
    Error ec;

    auto passphraseKey=suite->createPassphraseKey(ec);
    HATN_CHECK_EC(ec)

    passphraseKey->set(passphrase);

    return init(suite,std::move(passphraseKey),factory);
}

//---------------------------------------------------------------

Error ECDHKeys::generateKeys()
{
    Assert(m_processor,"ECDHKeys not initialized");

    if (m_ecdhReady)
    {
        return OK;
    }
    auto ec=m_processor->generateKey(m_pubKey);
    HATN_CHECK_EC(ec)
    m_ecdhReady=true;
    return OK;
}

//---------------------------------------------------------------

Result<common::SharedPtr<PublicKey>> ECDHKeys::getPublicKey()
{
    if (!m_ecdhReady)
    {
        return cryptError(CryptError::INVALID_DH_STATE);
    }
    if (m_pubKey)
    {
        return m_pubKey;
    }
    auto ec=m_processor->exportState(m_privKey,m_pubKey);
    HATN_CHECK_EC(ec)
    return m_pubKey;
}

//---------------------------------------------------------------

Result<common::SharedPtr<PrivateKey>> ECDHKeys::getPrivateKey()
{
    if (!m_ecdhReady)
    {
        return cryptError(CryptError::INVALID_DH_STATE);
    }
    if (m_privKey)
    {
        return m_privKey;
    }
    auto ec=m_processor->exportState(m_privKey,m_pubKey);
    HATN_CHECK_EC(ec)
    return m_privKey;
}

//---------------------------------------------------------------

Result<common::SharedPtr<DHSecret>> ECDHKeys::computeSecret(const common::SharedPtr<PublicKey>& peerPubKey)
{
    if (!m_ecdhReady)
    {
        return cryptError(CryptError::INVALID_DH_STATE);
    }
    common::SharedPtr<DHSecret> result;
    auto ec=m_processor->computeSecret(peerPubKey,result);
    HATN_CHECK_EC(ec)

    return result;
}

//---------------------------------------------------------------

Error ECDHKeys::importPrivateKey(lib::string_view keyData, ContainerFormat keyFormat)
{
    Assert(m_processor,"ECDHKeys not initialized");

    // get algorithm
    auto alg=m_processor->alg();
    if (alg==nullptr)
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }

    // create private key
    m_privKey=alg->createPrivateKey();
    if (!m_privKey)
    {
        return cryptError(CryptError::KEY_INITIALIZATION_FAILED);
    }

    // import private key
    m_privKey->setProtector(m_keyProtector.get());
    auto ec=m_privKey->importFromBuf(keyData,keyFormat);
    if (ec)
    {
        m_privKey.reset();
        return ec;
    }

    // import ecdh state
    ec=m_processor->importState(m_privKey);
    if (ec)
    {
        m_privKey.reset();
        return ec;
    }

    // done
    m_ecdhReady=true;
    return OK;
}

//---------------------------------------------------------------

Result<common::ByteArrayShared> ECDHKeys::exportPublicKey(ContainerFormat keyFormat)
{
    if (!m_ecdhReady)
    {
        return cryptError(CryptError::INVALID_DH_STATE);
    }
    if (!m_pubKey)
    {
        auto r=getPublicKey();
        HATN_CHECK_RESULT(r)
    }

    auto buf=m_factory->createObject<common::ByteArray>(m_factory);

    auto ec=m_pubKey->exportToBuf(*buf,keyFormat);
    HATN_CHECK_EC(ec)

    return buf;
}

//---------------------------------------------------------------

Result<common::ByteArrayShared> ECDHKeys::exportPrivateKey(ContainerFormat keyFormat)
{
    if (!m_ecdhReady)
    {
        return cryptError(CryptError::INVALID_DH_STATE);
    }
    if (!m_privKey)
    {
        auto r=getPrivateKey();
        HATN_CHECK_RESULT(r)
    }

    m_privKey->setProtector(m_keyProtector.get());
    common::MemoryLockedArray mbuf;
    auto ec=m_privKey->exportToBuf(mbuf,keyFormat);
    HATN_CHECK_EC(ec)

    auto buf=m_factory->createObject<common::ByteArray>(m_factory);
    buf->load(mbuf.data(),mbuf.size());

    return buf;
}

//---------------------------------------------------------------

HATN_CRYPT_NAMESPACE_END
