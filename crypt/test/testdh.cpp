/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/hex.hpp>

#include <hatn_test_config.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/format.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/keyprotector.h>

#include <hatn/common/makeshared.h>

#include "initcryptplugin.h"

#ifdef _WIN32
#ifndef _MSC_VER
#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#endif
#endif

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

// #define TEST_KEY_EXPORT_SAVE

BOOST_FIXTURE_TEST_SUITE(TestDH,CryptTestFixture)

void checkDh(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    std::string generatedSizeFile=fmt::format("{}/dh-generated-size.txt",path);

    // list algotihms
    BOOST_TEST_MESSAGE("DH algorithms begin");
    auto algs=plugin->listDHs();
    for (auto&& it: algs)
    {
        BOOST_TEST_MESSAGE(it);
    }
    BOOST_TEST_MESSAGE("DH algorithms end");

    // find algorithm
    const CryptAlgorithm* alg1=nullptr;
    std::string algName1="ffdhe4096";
    auto ec=plugin->findAlgorithm(alg1,CryptAlgorithm::Type::DH,algName1);
    BOOST_REQUIRE(!ec);

    // create dh1 and dh2
    auto dh1=plugin->createDH(alg1);
    BOOST_REQUIRE(dh1);
    auto dh2=plugin->createDH(alg1);
    BOOST_REQUIRE(dh2);

    // generate first pair
    SharedPtr<PublicKey> pubKey1;
    ec=dh1->generateKey(pubKey1);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!pubKey1->isNull());
    SharedPtr<PrivateKey> privKey1;
    SharedPtr<PublicKey> pubKeyExp1;
    ec=dh1->exportState(privKey1,pubKeyExp1);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!pubKeyExp1.isNull());
    BOOST_REQUIRE(!privKey1.isNull());
    BOOST_CHECK(pubKey1->content()==pubKeyExp1->content());

    // generate second pair
    SharedPtr<PublicKey> pubKey2;
    ec=dh2->generateKey(pubKey2);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!pubKey2.isNull());

    // compute shared secret
    SharedPtr<DHSecret> genSecret1;
    SharedPtr<DHSecret> genSecret2;
    ec=dh1->computeSecret(pubKey2,genSecret1);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!genSecret1.isNull());
    ec=dh2->computeSecret(pubKey1,genSecret2);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!genSecret2.isNull());
    BOOST_CHECK(genSecret1->content()==genSecret2->content());
    if (boost::filesystem::exists(generatedSizeFile))
    {
        auto genSize=std::stoi(PluginList::linefromFile(generatedSizeFile));
        BOOST_CHECK_EQUAL(genSecret1->content().size(),genSize);
    }

    // import state and check that computed secret is the same
    auto dh3=plugin->createDH(alg1);
    HATN_REQUIRE(dh3);
    ec=dh3->importState(privKey1,pubKey1);
    BOOST_CHECK(!ec);
    SharedPtr<DHSecret> genSecret3;
    ec=dh3->computeSecret(pubKey2,genSecret3);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!genSecret3.isNull());
    BOOST_CHECK(genSecret1->content()==genSecret3->content());

    // import state only from private key and check that computed secret is the same
    auto dh4=plugin->createDH(alg1);
    HATN_REQUIRE(dh4);
    ec=dh4->importState(privKey1);
    BOOST_CHECK(!ec);
    SharedPtr<DHSecret> genSecret4;
    ec=dh4->computeSecret(pubKey2,genSecret4);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!genSecret4.isNull());
    BOOST_CHECK(genSecret1->content()==genSecret4->content());

    // derive public key from private key
    auto dh5=plugin->createDH(alg1);
    HATN_REQUIRE(dh5);
    ec=dh5->importState(privKey1);
    BOOST_CHECK(!ec);
    SharedPtr<PrivateKey> privKeyExp5;
    SharedPtr<PublicKey> pubKeyExp5;
    ec=dh5->exportState(privKeyExp5,pubKeyExp5);
    BOOST_CHECK(!ec);
    BOOST_CHECK(pubKeyExp5->content()==pubKeyExp1->content());


    // derive public key from private key, second approach
    auto dh6=plugin->createDH(alg1);
    HATN_REQUIRE(dh6);
    ec=dh6->importState(privKey1);
    BOOST_CHECK(!ec);
    SharedPtr<PublicKey> pubKey6;
    ec=dh6->generateKey(pubKey6);
    BOOST_CHECK(!ec);
    BOOST_CHECK(pubKey1->content()==pubKey6->content());

    // invalid public keys
    auto dh7=plugin->createDH(alg1);
    HATN_REQUIRE(dh7);
    ec=dh7->importState(privKey1);
    BOOST_CHECK(!ec);
    SharedPtr<PublicKey> pubKey7=makeShared<PublicKey>();
    ec=pubKey7->importFromBuf("blablabla",ContainerFormat::RAW_PLAIN);
    BOOST_CHECK(!ec);
    SharedPtr<DHSecret> genSecret7;
    ec=dh7->computeSecret(pubKey7,genSecret7);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Expected failuire to compute secret with invalid pubkey: {}",ec.message()));
    }
    BOOST_CHECK(ec); // calculated but useless, because other side will never derive secret as the public key was not derived from private
    SharedPtr<DHSecret> genSecret8;
    SharedPtr<PublicKey> pubKey8=makeShared<PublicKey>();
    ec=dh7->computeSecret(pubKey8,genSecret8);
    BOOST_CHECK(ec);

    // reuse processor with with correct peer pubkey
    SharedPtr<DHSecret> genSecret9;
    ec=dh7->computeSecret(pubKey2,genSecret9);
    BOOST_CHECK(!ec);
    BOOST_CHECK(genSecret1->content()==genSecret9->content());

    // arbitrary private key
    auto dh10=plugin->createDH(alg1);
    HATN_REQUIRE(dh10);
    SharedPtr<PrivateKey> privKey10=makeShared<PrivateKey>();
    ec=privKey10->importFromBuf("Hello world lalalalallalala!");
    BOOST_CHECK(!ec);
    ec=dh10->importState(privKey10);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Expected failuire to import invalid privkey: {}",ec.message()));
    }
    BOOST_CHECK(ec);
}

BOOST_AUTO_TEST_CASE(CheckDH)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::DH))
            {
                checkDh(plugin,PluginList::assetsPath("crypt"));
            }
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckDHKeyExportImport)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::DH))
            {
                auto eachPath=[plugin](const std::string& path)
                {
                    std::string suiteFile=fmt::format("{}/dh-suite.json",path);
                    std::string passphraseFile=fmt::format("{}/dh-passphrase.txt",path);
                    std::string pkeyPlainFile1=fmt::format("{}/dh-pkey1-plain.dat",path);
                    std::string pubkeyFile1=fmt::format("{}/dh-pubkey1.dat",path);
                    std::string pkeyEncryptedFile2=fmt::format("{}/dh-pkey2-encrypted.hcc",path);
                    std::string pubkeyFile2=fmt::format("{}/dh-pubkey2.dat",path);
                    std::string generatedSecretFile=fmt::format("{}/dh-generated.dat",path);

                    if (
                            boost::filesystem::exists(suiteFile)
                            &&
                            boost::filesystem::exists(passphraseFile)
                       )
                    {
                        // load cipher suite
                        auto cipherSuite=std::make_shared<CipherSuite>();
                        auto ec=cipherSuite->loadFromFile(suiteFile);
                        BOOST_CHECK(!ec);
                        CipherSuitesGlobal::instance().addSuite(cipherSuite);
                        auto engine=std::make_shared<CryptEngine>(plugin.get());
                        CipherSuitesGlobal::instance().setDefaultEngine(std::move(engine));

                        // find key protection algorithm
                        const CryptAlgorithm* protectAlg=nullptr;
                        ec=cipherSuite->aeadAlgorithm(protectAlg);
                        if (ec)
                        {
                            return;
                        }
                        HATN_REQUIRE(protectAlg);

                        // find DH algorithm
                        const CryptAlgorithm* dhAlg=nullptr;
                        ec=cipherSuite->dhAlgorithm(dhAlg);
                        if (ec)
                        {
                            return;
                        }
                        HATN_REQUIRE(dhAlg);

                        BOOST_TEST_MESSAGE(fmt::format("Testing {} with protecting algorithm {}",dhAlg->name(),protectAlg->name()));

                        // create passphrase
                        auto passphrase=cipherSuite->createPassphraseKey(ec,protectAlg);
                        HATN_REQUIRE(!ec);
                        HATN_REQUIRE(passphrase);
                        ByteArray passphraseData;
                        ec=passphraseData.loadFromFile(passphraseFile);
                        HATN_REQUIRE(!ec);
                        ec=passphrase->importFromBuf(passphraseData);
                        HATN_REQUIRE(!ec);

                        // create key protector
                        auto protector=makeShared<KeyProtector>(passphrase,cipherSuite.get());

                        // create dh processor 1
                        auto dh1=cipherSuite->createDH(ec);
                        HATN_REQUIRE(!ec);
                        HATN_REQUIRE(dh1);
                        ec=dh1->init();
                        HATN_REQUIRE(!ec);

                        // generate and export keys 1
                        SharedPtr<PublicKey> pubKey1_1,pubKey1_2;
                        ec=dh1->generateKey(pubKey1_1);
                        BOOST_CHECK(!ec);
                        HATN_REQUIRE(pubKey1_1);
                        common::SharedPtr<PrivateKey> privKey1_1;
                        ec=dh1->exportState(privKey1_1,pubKey1_2);
                        BOOST_CHECK(!ec);
                        HATN_REQUIRE(privKey1_1);
                        HATN_REQUIRE(pubKey1_2);
                        MemoryLockedArray privKeyBufPlain1,privKeyBufEncrypted1;
                        ec=privKey1_1->exportToBuf(privKeyBufPlain1,ContainerFormat::PEM,true);
                        if (ec)
                        {
                            BOOST_TEST_MESSAGE(ec.message());
                        }
                        HATN_REQUIRE(!ec);
                        privKey1_1->setProtector(protector.get());
                        ec=privKey1_1->exportToBuf(privKeyBufEncrypted1,ContainerFormat::PEM);
                        HATN_REQUIRE(!ec);
                        ByteArray pubKeyBuf1_1,pubKeyBuf1_2;
                        ec=pubKey1_1->exportToBuf(pubKeyBuf1_1,ContainerFormat::PEM);
                        BOOST_CHECK(!ec);
                        ec=pubKey1_2->exportToBuf(pubKeyBuf1_2,ContainerFormat::PEM);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK(pubKeyBuf1_1==pubKeyBuf1_2);
#ifdef TEST_KEY_EXPORT_SAVE
                        ec=pubKeyBuf1_1.saveToFile(pubkeyFile1);
                        BOOST_CHECK(!ec);
                        ec=privKey1_1->exportToFile(pkeyPlainFile1,ContainerFormat::PEM,true);
                        BOOST_CHECK(!ec);
#endif

                        // create dh processor 2
                        auto dh2=cipherSuite->createDH(ec);
                        HATN_REQUIRE(!ec);
                        HATN_REQUIRE(dh2);
                        ec=dh2->init();
                        HATN_REQUIRE(!ec);

                        // generate and export keys 2
                        SharedPtr<PublicKey> pubKey2_1,pubKey2_2;
                        ec=dh2->generateKey(pubKey2_1);
                        BOOST_CHECK(!ec);
                        HATN_REQUIRE(pubKey2_1);
                        common::SharedPtr<PrivateKey> privKey2_1;
                        ec=dh2->exportState(privKey2_1,pubKey2_2);
                        BOOST_CHECK(!ec);
                        HATN_REQUIRE(privKey2_1);
                        HATN_REQUIRE(pubKey2_2);
                        MemoryLockedArray privKeyBufPlain2,privKeyBufEncrypted2;
                        ec=privKey2_1->exportToBuf(privKeyBufPlain2,ContainerFormat::PEM,true);
                        HATN_REQUIRE(!ec);
                        privKey2_1->setProtector(protector.get());
                        ec=privKey2_1->exportToBuf(privKeyBufEncrypted2,ContainerFormat::PEM);
                        HATN_REQUIRE(!ec);
                        ByteArray pubKeyBuf2_1,pubKeyBuf2_2;
                        ec=pubKey2_1->exportToBuf(pubKeyBuf2_1,ContainerFormat::RAW_PLAIN);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK(pubKeyBuf2_1.empty());
                        ec=pubKey2_1->exportToBuf(pubKeyBuf2_1,ContainerFormat::PEM);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK(!pubKeyBuf2_1.empty());
                        ec=pubKey2_2->exportToBuf(pubKeyBuf2_2,ContainerFormat::PEM);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK(pubKeyBuf2_1==pubKeyBuf2_2);
#ifdef TEST_KEY_EXPORT_SAVE
                        ec=pubKeyBuf2_1.saveToFile(pubkeyFile2);
                        BOOST_CHECK(!ec);
                        ec=privKey2_1->exportToFile(pkeyEncryptedFile2,ContainerFormat::PEM);
                        BOOST_CHECK(!ec);
#endif

                        // compute secret
                        common::SharedPtr<DHSecret> generatedSecret1;
                        ec=dh1->computeSecret(pubKey2_2,generatedSecret1);
                        BOOST_CHECK(!ec);
                        common::SharedPtr<DHSecret> generatedSecret2;
                        ec=dh2->computeSecret(pubKey1_2,generatedSecret2);
                        BOOST_CHECK(!ec);
                        MemoryLockedArray generatedSecretBuf1,generatedSecretBuf2;
                        ec=generatedSecret1->exportToBuf(generatedSecretBuf1,ContainerFormat::RAW_PLAIN);
                        BOOST_CHECK(!ec);
                        ec=generatedSecret2->exportToBuf(generatedSecretBuf2,ContainerFormat::RAW_PLAIN);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK(generatedSecretBuf1==generatedSecretBuf2);
#ifdef TEST_KEY_EXPORT_SAVE
                        ec=generatedSecretBuf1.saveToFile(generatedSecretFile);
                        BOOST_CHECK(!ec);
#endif

                        // test imported keys
                        for (size_t i=0;i<2;i++)
                        {
                            bool fromFile=i==1;
                            if (fromFile)
                            {
                                BOOST_TEST_MESSAGE(fmt::format("Testing from file"));
                            }
                            else
                            {
                                BOOST_TEST_MESSAGE(fmt::format("Testing from buffer"));
                            }

                            // create and import plain private key 3
                            auto privKey3=dhAlg->createPrivateKey();
                            HATN_REQUIRE(privKey3);
                            if (fromFile)
                            {
                                ec=privKey3->importFromFile(pkeyPlainFile1,ContainerFormat::PEM,true);
                            }
                            else
                            {
                                ec=privKey3->importFromBuf(privKeyBufPlain1,ContainerFormat::PEM);
                            }
                            HATN_REQUIRE(!ec);

                            // import public key 3
                            auto pubKey3=dhAlg->createPublicKey();
                            HATN_REQUIRE(pubKey3);
                            if (fromFile)
                            {
                                ec=pubKey3->importFromFile(pubkeyFile1,ContainerFormat::PEM);
                            }
                            else
                            {
                                ec=pubKey3->importFromBuf(pubKeyBuf1_2,ContainerFormat::PEM);
                            }
                            if (ec)
                            {
                                BOOST_TEST_MESSAGE(ec.message());
                            }
                            HATN_REQUIRE(!ec);

                            // create dh processor 3
                            auto dh3=cipherSuite->createDH(ec);
                            HATN_REQUIRE(!ec);
                            HATN_REQUIRE(dh3);
                            ec=dh3->init();
                            HATN_REQUIRE(!ec);
                            ec=dh3->importState(privKey3,pubKey3);
                            HATN_REQUIRE(!ec);

                            // create and import encrypted private key 4
                            auto privKey4=dhAlg->createPrivateKey();
                            HATN_REQUIRE(privKey4);
                            privKey4->setProtector(protector.get());
                            if (fromFile)
                            {
                                ec=privKey4->importFromFile(pkeyEncryptedFile2,ContainerFormat::PEM);
                                HATN_REQUIRE(!ec);
                            }
                            else
                            {
                                ec=privKey4->importFromBuf(privKeyBufEncrypted2,ContainerFormat::PEM);
                                HATN_REQUIRE(!ec);

                                auto tmpPrivKey=dhAlg->createPrivateKey();
                                HATN_REQUIRE(tmpPrivKey);
                                tmpPrivKey->setProtector(protector.get());
                                auto tmpBuf=privKeyBufEncrypted2;
                                tmpBuf[tmpBuf.size()/2]=static_cast<char>(0xaa);
                                tmpBuf[tmpBuf.size()/2-1]=static_cast<char>(0x55);
                                tmpBuf[tmpBuf.size()/2+1]=static_cast<char>(0x55);
                                ec=tmpPrivKey->importFromBuf(tmpBuf,ContainerFormat::PEM);
                                BOOST_CHECK(ec);
                            }

                            // import public key 4
                            auto pubKey4=dhAlg->createPublicKey();
                            HATN_REQUIRE(pubKey4);
                            auto pubKey5=dhAlg->createPublicKey();
                            HATN_REQUIRE(pubKey5);
                            if (fromFile)
                            {
                                ec=pubKey4->importFromFile(pubkeyFile2,ContainerFormat::PEM);
                                HATN_REQUIRE(!ec);
                            }
                            else
                            {
                                //! @todo Check buf size when importing, at least must not be empty
                                ec=pubKey4->importFromBuf(pubKeyBuf2_2,ContainerFormat::PEM);
                                HATN_REQUIRE(!ec);

                                auto tmpBuf=pubKeyBuf2_2;
                                tmpBuf[tmpBuf.size()/2]=static_cast<char>(0xaa);
                                tmpBuf[tmpBuf.size()/2-1]=static_cast<char>(0x55);
                                tmpBuf[tmpBuf.size()/2+1]=static_cast<char>(0x55);
                                ec=pubKey5->importFromBuf(tmpBuf,ContainerFormat::PEM);
                                BOOST_CHECK(ec);
                            }

                            // create dh processor 4
                            auto dh4=cipherSuite->createDH(ec);
                            HATN_REQUIRE(!ec);
                            HATN_REQUIRE(dh4);
                            ec=dh4->init();
                            HATN_REQUIRE(!ec);
                            ec=dh4->importState(privKey4,pubKey4);
                            HATN_REQUIRE(!ec);

                            // compute and check secrets
                            common::SharedPtr<DHSecret> generatedSecret3;
                            ec=dh3->computeSecret(pubKey4,generatedSecret3);
                            BOOST_CHECK(!ec);
                            common::SharedPtr<DHSecret> generatedSecret4;
                            ec=dh4->computeSecret(pubKey3,generatedSecret4);
                            BOOST_CHECK(!ec);
                            MemoryLockedArray generatedSecretBuf3,generatedSecretBuf4;
                            ec=generatedSecret3->exportToBuf(generatedSecretBuf3,ContainerFormat::RAW_PLAIN);
                            BOOST_CHECK(!ec);
                            ec=generatedSecret4->exportToBuf(generatedSecretBuf4,ContainerFormat::RAW_PLAIN);
                            BOOST_CHECK(!ec);
                            BOOST_CHECK(generatedSecretBuf3==generatedSecretBuf4);
                            if (fromFile)
                            {
                                MemoryLockedArray storedBuf;
                                ec=storedBuf.loadFromFile(generatedSecretFile);
                                BOOST_CHECK(!ec);
                                BOOST_CHECK(storedBuf==generatedSecretBuf3);
                            }
                            else
                            {
                                BOOST_CHECK(generatedSecretBuf1==generatedSecretBuf3);
                            }

                            // check wrong public key
                            if (!fromFile)
                            {
                                auto dh5_0=cipherSuite->createDH(ec);
                                HATN_REQUIRE(!ec);
                                HATN_REQUIRE(dh5_0);
                                ec=dh5_0->generateKey(pubKey5);
                                HATN_REQUIRE(!ec);

                                auto dh5=cipherSuite->createDH(ec);
                                HATN_REQUIRE(!ec);
                                HATN_REQUIRE(dh5);
                                ec=dh5->init();
                                HATN_REQUIRE(!ec);
                                ec=dh5->importState(privKey3,pubKey3);
                                HATN_REQUIRE(!ec);
                                common::SharedPtr<DHSecret> generatedSecret5;
                                ec=dh5->computeSecret(pubKey5,generatedSecret5);
                                BOOST_CHECK(!ec);
                                MemoryLockedArray generatedSecretBuf5;
                                ec=generatedSecret5->exportToBuf(generatedSecretBuf5,ContainerFormat::RAW_PLAIN);
                                BOOST_CHECK(!ec);
                                BOOST_CHECK(generatedSecretBuf4!=generatedSecretBuf5);
                            }
                        }
                        BOOST_TEST_MESSAGE(fmt::format("Done {} with protecting algorithm {}",dhAlg->name(),protectAlg->name()));
                    }
                };
                CipherSuitesGlobal::instance().reset();
                eachPath(PluginList::assetsPath("crypt"));
                CipherSuitesGlobal::instance().reset();
                eachPath(PluginList::assetsPath("crypt",plugin->info()->name));
                CipherSuitesGlobal::instance().reset();
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
