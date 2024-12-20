/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

#include <hatn_test_config.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/format.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/fileutils.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/aead.h>
#include <hatn/crypt/cipher.h>
#include <hatn/crypt/signature.h>
#include <hatn/crypt/mac.h>
#include <hatn/crypt/keyprotector.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/ciphersuite.h>

#include <hatn/test/multithreadfixture.h>

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

namespace
{

struct AlgConfig
{
    CryptAlgorithm::Type type;
    std::string typeStr;
    Crypt::Feature feature;
    ContainerFormat format;
    std::function<std::vector<std::string> (std::shared_ptr<CryptPlugin>& plugin)> listAlgsFn;
    std::function<void (std::shared_ptr<CryptPlugin>&, AlgConfig& config, const std::string&, const std::string&)> testHandler;
    std::function<common::SharedPtr<SecureKey> (const CryptAlgorithm*)> createKeyFn;
    std::function<void (CipherSuite*,SecureKey*,ByteArray&,ByteArray&,AlgConfig*)> checkAlgorithmFn;
    bool unprotected;
    const PublicKey* pubkey=nullptr;
    bool expectedFailure=false;
    bool expectedFailureSigPub=false;
};

}

// #define PRINT_CRYPT_ENGINE_ALGS
#define TEST_KEY_EXPORT_SAVE

BOOST_FIXTURE_TEST_SUITE(TestKeyExportImport,CryptTestFixture)

BOOST_AUTO_TEST_CASE(CheckMultipleRawEncrypted)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::SymmetricEncryption))
            {
                std::vector<std::string> algs=plugin->listCiphers();
                for (auto&& it:algs)
                {
                    CipherSuites::instance().reset();
                    std::string algPathName=it;
                    boost::algorithm::replace_all(algPathName,std::string("/"),std::string("-"));
                    boost::algorithm::replace_all(algPathName,std::string(":"),std::string("-"));

                    auto handler=[&plugin](
                            const std::string& path
                        )
                    {
                        std::string suiteFile=fmt::format("{}-suite.json",path);
                        if (!boost::filesystem::exists(suiteFile))
                        {
                            return;
                        }

                        BOOST_TEST_MESSAGE(fmt::format("Testing {}",path));

                        // load cipher suite
                        auto suite=std::make_shared<CipherSuite>();
                        auto ec=suite->loadFromFile(suiteFile);
                        BOOST_CHECK(!ec);
                        CipherSuites::instance().addSuite(suite);
                        auto engine=std::make_shared<CryptEngine>(plugin.get());
                        CipherSuites::instance().setDefaultEngine(std::move(engine));

                        // find algorithm
                        const CryptAlgorithm* alg=nullptr;
                        ec=suite->aeadAlgorithm(alg);
                        if (ec)
                        {
                            HATN_TEST_MESSAGE_TS(ec.message());
                        }
                        HATN_REQUIRE(!ec);
                        HATN_REQUIRE(alg!=nullptr);

                        for (size_t i=0;i<2;i++)
                        {
                            // create passhrase
                            auto passphrase=suite->createPassphraseKey(ec,alg);
                            HATN_REQUIRE(!ec);
                            HATN_REQUIRE(passphrase);
                            ec=passphrase->importFromBuf("Hello from hatn!");
                            HATN_REQUIRE(!ec);

                            // create HKDF master key
                            auto hkdfMasterKey=makeShared<SymmetricKey>();
                            ec=hkdfMasterKey->importFromBuf("Hello from hatn!");
                            HATN_REQUIRE(!ec);

                            // create key protector
                            SharedPtr<KeyProtector> protector;
                            if (i==0)
                            {
                                protector=makeShared<KeyProtector>(passphrase,suite.get());
                            }
                            else
                            {
                                protector=makeShared<KeyProtector>(hkdfMasterKey,suite.get());
                            }

                            // create and generate first key
                            auto key1=alg->createSymmetricKey();
                            HATN_REQUIRE(key1);
                            ec=key1->generate();
                            HATN_REQUIRE(!ec);

                            // set key protector
                            key1->setProtector(protector.get());

                            // export key to first buffer
                            MemoryLockedArray buf1;
                            ec=key1->exportToBuf(buf1,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(!ec);

                            // create second key
                            auto key2=alg->createSymmetricKey();
                            HATN_REQUIRE(key2);
                            key2->setProtector(protector.get());

                            // import key from first buffer
                            ec=key2->importFromBuf(buf1,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(!ec);

                            // export key to second buffer
                            MemoryLockedArray buf2;
                            ec=key2->exportToBuf(buf2,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(!ec);

                            // create third key
                            auto key3=alg->createSymmetricKey();
                            HATN_REQUIRE(key3);
                            key3->setProtector(protector.get());

                            // import third key from second buffer
                            ec=key3->importFromBuf(buf2,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(!ec);

                            // import mailformed key
                            MemoryLockedArray mailformedBuf=buf2;
                            auto keyM1=alg->createSymmetricKey();
                            HATN_REQUIRE(keyM1);
                            keyM1->setProtector(protector.get());
                            ec=keyM1->importFromBuf(mailformedBuf,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(!ec);
                            auto keyM2=alg->createSymmetricKey();
                            HATN_REQUIRE(keyM2);
                            keyM2->setProtector(protector.get());
                            mailformedBuf[mailformedBuf.size()/2]='a';
                            mailformedBuf[mailformedBuf.size()/2-1]='b';
                            mailformedBuf[mailformedBuf.size()/2+1]='c';
                            ec=keyM2->importFromBuf(mailformedBuf,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(ec);

                            // export/import to/from file
                            auto tmpKeyFile=fmt::format("{}/keydata.dat",hatn::test::MultiThreadFixture::tmpPath());
                            ec=key1->exportToFile(tmpKeyFile,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(!ec);
                            auto key4=alg->createSymmetricKey();
                            HATN_REQUIRE(key3);
                            key4->setProtector(protector.get());
                            ec=key4->importFromFile(tmpKeyFile,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(!ec);
                            MemoryLockedArray buf3;
                            ec=key4->exportToBuf(buf3,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(!ec);
                            auto key5=alg->createSymmetricKey();
                            HATN_REQUIRE(key5);
                            key5->setProtector(protector.get());
                            ec=key5->importFromBuf(buf3,ContainerFormat::RAW_ENCRYPTED);
                            HATN_REQUIRE(!ec);

                            // remove tmp file
                            std::ignore=FileUtils::remove(tmpKeyFile);
                        }

                        BOOST_TEST_MESSAGE(fmt::format("Done {}",path));
                    };

                    std::string path=fmt::format("{}/key-export-import-cipher-{}",PluginList::assetsPath("crypt"),algPathName);
                    handler(path);
                    path=fmt::format("{}/key-export-import-cipher-{}",PluginList::assetsPath("crypt",plugin->info()->name),algPathName);
                    handler(path);

                    CipherSuites::instance().reset();
                }
            }
        }
    );
}

static void algHandler(
        std::shared_ptr<CryptPlugin>& plugin,
        AlgConfig& config,
        const std::string& algName,
        const std::string& path
    )
{
#ifdef PRINT_CRYPT_ENGINE_ALGS
    BOOST_TEST_MESSAGE(algName);
#endif

    std::string encryptedStr=config.unprotected?"plain":"encrypted";

    std::string suiteFile=fmt::format("{}-suite.json",path);
    std::string passphraseFile=fmt::format("{}-passphrase.txt",path);

    std::string keyFile=fmt::format("{}-key-{}.{}",path,encryptedStr,ContainerFormatToStr(config.format));
    std::string inputFile=fmt::format("{}-in-pkey_{}_{}.dat",path,ContainerFormatToStr(config.format),encryptedStr);
    std::string outputFile=fmt::format("{}-out-pkey_{}_{}.dat",path,ContainerFormatToStr(config.format),encryptedStr);

    if (!boost::filesystem::exists(suiteFile)
            ||
#ifndef TEST_KEY_EXPORT_SAVE
        !boost::filesystem::exists(inputFile)
            ||
        !boost::filesystem::exists(outputFile)
            ||
        !boost::filesystem::exists(keyFile)
            ||
#endif
        !boost::filesystem::exists(passphraseFile)
       )
    {
        return;
    }

    size_t cycles=1;
    if (config.type==CryptAlgorithm::Type::SIGNATURE)
    {
        cycles=2;
    }
    for (size_t i=0;i<cycles;i++)
    {
        if (i==0)
        {
            if (config.type==CryptAlgorithm::Type::SIGNATURE)
            {
                BOOST_TEST_MESSAGE(fmt::format("Testing algorithm {} with {} format {}, pubkey PEM",algName,encryptedStr,ContainerFormatToStr(config.format)));
            }
            else
            {
                BOOST_TEST_MESSAGE(fmt::format("Testing algorithm {} with {} format {}",algName,encryptedStr,ContainerFormatToStr(config.format)));
            }
        }
        else
        {
            BOOST_TEST_MESSAGE(fmt::format("Testing algorithm {} with {} format {}, pubkey DER",algName,encryptedStr,ContainerFormatToStr(config.format)));
        }

        auto pubKeyFormat=(i==0)?ContainerFormat::PEM:ContainerFormat::DER;
        std::string pubkeyFile=fmt::format("{}-pubkey-pkey_{}_{}.{}",path,ContainerFormatToStr(config.format),encryptedStr,ContainerFormatToStr(pubKeyFormat));
        common::SharedPtr<PublicKey> pubKey;

        // find algorithm
        const CryptAlgorithm* alg=nullptr;
        auto ec=plugin->findAlgorithm(alg,config.type,algName);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(alg!=nullptr);

        // load cipher suite
        auto cipherSuite=std::make_shared<CipherSuite>();
        ec=cipherSuite->loadFromFile(suiteFile);
        BOOST_CHECK(!ec);
        CipherSuites::instance().addSuite(cipherSuite);
        auto engine=std::make_shared<CryptEngine>(plugin.get());
        CipherSuites::instance().setDefaultEngine(std::move(engine));

        // create passhrase
        auto passphrase=cipherSuite->createPassphraseKey(ec,alg);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(passphrase);
        if (config.format!=ContainerFormat::RAW_PLAIN)
        {
            ByteArray passphraseData;
            ec=passphraseData.loadFromFile(passphraseFile);
            HATN_REQUIRE(!ec);
            ec=passphrase->importFromBuf(passphraseData);
            HATN_REQUIRE(!ec);
        }

        // create key
        auto key1=config.createKeyFn(alg);
        HATN_REQUIRE(key1);

        // create key protector
        auto protector=makeShared<KeyProtector>(passphrase,cipherSuite.get());
        if (config.format!=ContainerFormat::RAW_PLAIN)
        {
            key1->setProtector(protector.get());
        }

        // check with generated key and data
        ByteArray plaintext,algResult;
        ec=key1->generate();
        HATN_REQUIRE(!ec);
        ec=plugin->randContainer(plaintext,0x10000,1);
        HATN_REQUIRE(!ec);
        config.checkAlgorithmFn(cipherSuite.get(),key1.get(),plaintext,algResult,&config);
        if (!algResult.isEmpty())
        {
            // export key to buffer
            MemoryLockedArray keyBuf;
            ec=key1->exportToBuf(keyBuf,config.format,config.unprotected);
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            HATN_REQUIRE(!ec);

            // derive and export public key
            if (config.type==CryptAlgorithm::Type::SIGNATURE)
            {
                auto privateKey=dynamic_cast<const PrivateKey*>(key1.get());
                HATN_REQUIRE(privateKey);

                pubKey=alg->createPublicKey();
                HATN_REQUIRE(pubKey);
                ec=pubKey->derive(*privateKey);
                HATN_REQUIRE(!ec);
                config.pubkey=pubKey.get();

                ByteArray pubKeyBuf;
                ec=pubKey->exportToBuf(pubKeyBuf,pubKeyFormat);
                HATN_REQUIRE(!ec);
            }

#ifdef TEST_KEY_EXPORT_SAVE
            if (i==0)
            {
                // export testing data
                ec=plaintext.saveToFile(inputFile);
                HATN_REQUIRE(!ec);
                ec=algResult.saveToFile(outputFile);
                HATN_REQUIRE(!ec);
                ec=keyBuf.saveToFile(keyFile);
                HATN_REQUIRE(!ec);
            }
#endif

            // create key
            auto key2=config.createKeyFn(alg);
            HATN_REQUIRE(key2);
            if (config.format!=ContainerFormat::RAW_PLAIN)
            {
                key2->setProtector(protector.get());
            }

            // import key from buffer
            ec=key2->importFromBuf(keyBuf,config.format);
            HATN_REQUIRE(!ec);

            // derive public key
            if (config.type==CryptAlgorithm::Type::SIGNATURE)
            {
                auto privateKey=dynamic_cast<const PrivateKey*>(key2.get());
                HATN_REQUIRE(privateKey);

                pubKey=alg->createPublicKey();
                HATN_REQUIRE(pubKey);
                ec=pubKey->derive(*privateKey);
                HATN_REQUIRE(!ec);
                config.pubkey=pubKey.get();
            }

            // check algorithm
            config.checkAlgorithmFn(cipherSuite.get(),key2.get(),plaintext,algResult,&config);

            // export keys to files
            auto tmpKeyFile=fmt::format("{}/keydata.dat",hatn::test::MultiThreadFixture::tmpPath());
            auto tmpPubKeyFile=fmt::format("{}/pubkeydata.dat",hatn::test::MultiThreadFixture::tmpPath());
            ec=key1->exportToFile(tmpKeyFile,config.format,config.unprotected);
            HATN_REQUIRE(!ec);
            if (config.type==CryptAlgorithm::Type::SIGNATURE)
            {
                ec=pubKey->exportToFile(tmpPubKeyFile,pubKeyFormat);
                HATN_REQUIRE(!ec);
            }

            // create key
            auto key3=config.createKeyFn(alg);
            HATN_REQUIRE(key3);
            if (config.format!=ContainerFormat::RAW_PLAIN)
            {
                key3->setProtector(protector.get());
            }

            // import key from file
            ec=key3->importFromFile(tmpKeyFile,config.format);
            HATN_REQUIRE(!ec);

            // import public key
            if (config.type==CryptAlgorithm::Type::SIGNATURE)
            {
                pubKey=alg->createPublicKey();
                HATN_REQUIRE(pubKey);
                ec=pubKey->importFromFile(tmpPubKeyFile,pubKeyFormat);
                HATN_REQUIRE(!ec);
                config.pubkey=pubKey.get();
            }

            // check algorithm
            config.checkAlgorithmFn(cipherSuite.get(),key3.get(),plaintext,algResult,&config);

            // remove tmp file
            std::ignore=FileUtils::remove(tmpKeyFile);
        }

        // check with prepared test vectors

        // import keys from files
        key1=config.createKeyFn(alg);
        HATN_REQUIRE(key1);
        if (config.format!=ContainerFormat::RAW_PLAIN)
        {
            key1->setProtector(protector.get());
        }
        ec=key1->importFromFile(keyFile,config.format);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        HATN_REQUIRE(!ec);
        if (config.type==CryptAlgorithm::Type::SIGNATURE)
        {
            pubKey=alg->createPublicKey();
            HATN_REQUIRE(pubKey);

#ifdef TEST_KEY_EXPORT_SAVE
            auto privateKey=dynamic_cast<const PrivateKey*>(key1.get());
            HATN_REQUIRE(privateKey);

            ec=pubKey->derive(*privateKey);
            HATN_REQUIRE(!ec);

            ByteArray pubKeyBuf;
            ec=pubKey->exportToBuf(pubKeyBuf,pubKeyFormat);
            HATN_REQUIRE(!ec);

            ec=pubKeyBuf.saveToFile(pubkeyFile);
            HATN_REQUIRE(!ec);
#endif
            ec=pubKey->importFromFile(pubkeyFile,pubKeyFormat);
            HATN_REQUIRE(!ec);

            config.pubkey=pubKey.get();
        }

        // load test data
        ByteArray inputData;
        ec=inputData.loadFromFile(inputFile);
        HATN_REQUIRE(!ec);
        ByteArray outputData;
        ec=outputData.loadFromFile(outputFile);
        HATN_REQUIRE(!ec);

        // check algorithm
        config.checkAlgorithmFn(cipherSuite.get(),key1.get(),inputData,outputData,&config);

        // export key to buffer and import back
        MemoryLockedArray keyBuf;
        ByteArray pubkeyBuf;
        ec=key1->exportToBuf(keyBuf,config.format,config.unprotected);
        HATN_REQUIRE(!ec);
        if (config.type==CryptAlgorithm::Type::SIGNATURE)
        {
            ec=pubKey->exportToBuf(pubkeyBuf,pubKeyFormat);
            HATN_REQUIRE(!ec);
        }

        // create key and import
        auto key2=config.createKeyFn(alg);
        HATN_REQUIRE(key2);
        if (config.format!=ContainerFormat::RAW_PLAIN)
        {
            key2->setProtector(protector.get());
        }
        ec=key2->importFromBuf(keyBuf,config.format);
        HATN_REQUIRE(!ec);
        if (config.type==CryptAlgorithm::Type::SIGNATURE)
        {
            pubKey=alg->createPublicKey();
            HATN_REQUIRE(pubKey);
            ec=pubKey->importFromBuf(pubkeyBuf,pubKeyFormat);
            HATN_REQUIRE(!ec);
            config.pubkey=pubKey.get();
        }

        // check algorithm
        config.checkAlgorithmFn(cipherSuite.get(),key2.get(),inputData,outputData,&config);

        // create key and import with keeping content
        auto key3=config.createKeyFn(alg);
        HATN_REQUIRE(key3);
        if (config.format!=ContainerFormat::RAW_PLAIN)
        {
            key3->setProtector(protector.get());
        }
        ec=key3->importFromBuf(keyBuf,config.format,true);
        HATN_REQUIRE(!ec);

        // check algorithm
        config.checkAlgorithmFn(cipherSuite.get(),key3.get(),inputData,outputData,&config);

        // import mailformed key
        auto keyM=config.createKeyFn(alg);
        HATN_REQUIRE(keyM);
        if (config.format!=ContainerFormat::RAW_PLAIN)
        {
            keyM->setProtector(protector.get());
        }
        MemoryLockedArray keyBufM=keyBuf;
        keyBufM[keyBufM.size()-4]='a';
        keyBufM[keyBufM.size()-5]='b';
        keyBufM[keyBufM.size()-6]='c';
        keyBufM[keyBufM.size()/2-4]='a';
        keyBufM[keyBufM.size()/2-5]='b';
        keyBufM[keyBufM.size()/2-6]='c';
        keyBufM[keyBufM.size()/2+4]='a';
        keyBufM[keyBufM.size()/2+5]='b';
        keyBufM[keyBufM.size()/2+6]='c';
        if (boost::algorithm::istarts_with(algName,"rsa/3072"))
        {
            // rsa algorithm works with prime numbers, so mush more bytes must be mailformed to detect failure
            for (size_t i=keyBufM.size()/2-100;i<keyBufM.size()/2+100;i++)
            {
                keyBufM[i]='a';
            }
        }
        ec=keyM->importFromBuf(keyBufM,config.format);
        if (config.format==ContainerFormat::RAW_ENCRYPTED)
        {
            BOOST_CHECK(ec);
        }
        else
        {
            if (config.type==CryptAlgorithm::Type::SIGNATURE)
            {
                pubKey=alg->createPublicKey();
                HATN_REQUIRE(pubKey);
                ec=pubKey->importFromBuf(pubkeyBuf,pubKeyFormat);
                HATN_REQUIRE(!ec);
                config.pubkey=pubKey.get();
            }
            config.expectedFailure=true;
            config.checkAlgorithmFn(cipherSuite.get(),keyM.get(),inputData,outputData,&config);
            config.expectedFailure=false;
            if (config.type==CryptAlgorithm::Type::SIGNATURE)
            {
                pubKey=alg->createPublicKey();
                HATN_REQUIRE(pubKey);
                auto pubKeyMailFormed=pubkeyBuf;
                pubKeyMailFormed[pubKeyMailFormed.size()/2-4]='a';
                pubKeyMailFormed[pubKeyMailFormed.size()/2-5]='b';
                pubKeyMailFormed[pubKeyMailFormed.size()/2-6]='c';
                pubKeyMailFormed[pubKeyMailFormed.size()/2+4]='a';
                pubKeyMailFormed[pubKeyMailFormed.size()/2+5]='b';
                pubKeyMailFormed[pubKeyMailFormed.size()/2+6]='c';
                ec=pubKey->importFromBuf(pubKeyMailFormed,pubKeyFormat);
                if (!ec)
                {
                    config.pubkey=pubKey.get();
                    config.expectedFailureSigPub=true;
                    config.checkAlgorithmFn(cipherSuite.get(),key3.get(),inputData,outputData,&config);
                    config.expectedFailureSigPub=false;
                }
            }
        }
    }

    BOOST_TEST_MESSAGE(fmt::format("Done algorithm {} with {} format {}",algName,encryptedStr,ContainerFormatToStr(config.format)));
}

static void testAlgKey(AlgConfig& config)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [&config](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(config.feature))
            {
                auto algs=config.listAlgsFn(plugin);
                HATN_REQUIRE(!algs.empty());
                for (auto&& it:algs)
                {
                    CipherSuites::instance().reset();

                    std::string algPathName=it;
                    boost::algorithm::replace_all(algPathName,std::string("/"),std::string("-"));
                    boost::algorithm::replace_all(algPathName,std::string(":"),std::string("-"));

                    std::string path=fmt::format("{}/key-export-import-{}-{}",PluginList::assetsPath("crypt"),config.typeStr,algPathName);
                    config.testHandler(plugin,config,it,path);
                    path=fmt::format("{}/key-export-import-{}-{}",PluginList::assetsPath("crypt",plugin->info()->name),config.typeStr,algPathName);
                    config.testHandler(plugin,config,it,path);

                    CipherSuites::instance().reset();
                }
            }
        }
    );
}

static void testCipher(CipherSuite*, const SecureKey* keyIn, ByteArray& inputData, ByteArray& outputData, AlgConfig* config)
{
    auto key=dynamic_cast<const SymmetricKey*>(keyIn);
    HATN_REQUIRE(key);

    if (outputData.empty())
    {
        auto ec=Cipher::encrypt(key,inputData,outputData);
        HATN_REQUIRE(!ec);
    }
    else
    {
        ByteArray plaintext,ciphertext;

        auto ec=Cipher::decrypt(key,outputData,plaintext);
        if (config->expectedFailure)
        {
            BOOST_CHECK(plaintext!=inputData);
        }
        else
        {
            BOOST_CHECK(!ec);
            BOOST_CHECK(plaintext==inputData);
        }

        ec=Cipher::encrypt(key,inputData,ciphertext);
        HATN_REQUIRE(!ec);

        plaintext.clear();
        ec=Cipher::decrypt(key,ciphertext,plaintext);
        HATN_REQUIRE(!ec);
        BOOST_CHECK(plaintext==inputData);
    }
}

static void checkCipherKey(ContainerFormat format)
{
    std::function<std::vector<std::string> (std::shared_ptr<CryptPlugin>& plugin)>
            f1=[](std::shared_ptr<CryptPlugin>& plugin){return plugin->listCiphers();};
    std::function<void (std::shared_ptr<CryptPlugin>&, AlgConfig& config, const std::string&, const std::string&)> f2=
    [](std::shared_ptr<CryptPlugin>& plugin, AlgConfig& config, const std::string& algName, const std::string& path)
    {
        algHandler(plugin,config,algName,path);
    };
    std::function<common::SharedPtr<SecureKey> (const CryptAlgorithm*)>
    f3=[](const CryptAlgorithm *alg)
    {
        return alg->createSymmetricKey();
    };
    std::function<void (CipherSuite*,SecureKey*,ByteArray&,ByteArray&,AlgConfig*)>
    f4=[](CipherSuite* suite, const SecureKey* key, ByteArray& inputData, ByteArray& outputData,AlgConfig* config)
    {
        testCipher(suite,key,inputData,outputData,config);
    };

    AlgConfig config{CryptAlgorithm::Type::SENCRYPTION,
                std::string("cipher"),
                Crypt::Feature::SymmetricEncryption,
                format,
                f1,
                f2,
                f3,
                f4,
                format!=ContainerFormat::RAW_ENCRYPTED
                };
    testAlgKey(config);
}

static void testAead(CipherSuite* suite, const SecureKey* keyIn, ByteArray& inputData, ByteArray& outputData, AlgConfig* config)
{
    auto key=dynamic_cast<const SymmetricKey*>(keyIn);
    HATN_REQUIRE(key);

    if (outputData.empty())
    {
        Error ec;
        auto enc=suite->createAeadEncryptor(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(enc);
        ec=AEAD::encryptPack(enc.get(),key,inputData,SpanBuffer(),outputData);
        HATN_REQUIRE(!ec);
    }
    else
    {
        ByteArray plaintext,ciphertext;

        Error ec;
        auto dec=suite->createAeadDecryptor(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(dec);

        ec=AEAD::decryptPack(dec.get(),key,outputData,SpanBuffer(),plaintext);
        if (config->expectedFailure)
        {
            BOOST_CHECK(ec);
        }
        else
        {
            BOOST_CHECK(!ec);
            BOOST_CHECK(plaintext==inputData);
        }

        auto enc=suite->createAeadEncryptor(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(enc);
        ec=AEAD::encryptPack(enc.get(),key,inputData,SpanBuffer(),ciphertext);
        HATN_REQUIRE(!ec);

        plaintext.clear();
        ec=AEAD::decryptPack(dec.get(),key,ciphertext,SpanBuffer(),plaintext);
        HATN_REQUIRE(!ec);
        BOOST_CHECK(plaintext==inputData);
    }
}

static void checkAeadKey(ContainerFormat format)
{
    std::function<std::vector<std::string> (std::shared_ptr<CryptPlugin>& plugin)>
            f1=[](std::shared_ptr<CryptPlugin>& plugin){return plugin->listAEADs();};
    std::function<void (std::shared_ptr<CryptPlugin>&, AlgConfig& config, const std::string&, const std::string&)> f2=
    [](std::shared_ptr<CryptPlugin>& plugin, AlgConfig& config, const std::string& algName, const std::string& path)
    {
        algHandler(plugin,config,algName,path);
    };
    std::function<common::SharedPtr<SecureKey> (const CryptAlgorithm*)>
    f3=[](const CryptAlgorithm *alg)
    {
        return alg->createSymmetricKey();
    };
    std::function<void (CipherSuite*,SecureKey*,ByteArray&,ByteArray&,AlgConfig*)>
    f4=[](CipherSuite* suite, const SecureKey* key, ByteArray& inputData, ByteArray& outputData,AlgConfig* config)
    {
        testAead(suite,key,inputData,outputData,config);
    };

    AlgConfig config{CryptAlgorithm::Type::AEAD,"aead",Crypt::Feature::AEAD,
                format,
                f1,
                f2,
                f3,
                f4,
                format!=ContainerFormat::RAW_ENCRYPTED
                };
    testAlgKey(config);
}

static void testMAC(CipherSuite* suite, const SecureKey* keyIn, ByteArray& inputData, ByteArray& outputData, AlgConfig* config)
{
    auto key=dynamic_cast<const MACKey*>(keyIn);
    HATN_REQUIRE(key);

    if (outputData.empty())
    {
        Error ec;
        auto mac1=suite->createMAC(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(mac1);
        mac1->setKey(key);
        ec=mac1->runSign(inputData,outputData);
        HATN_REQUIRE(!ec);
    }
    else
    {
        ByteArray tag;
        Error ec;
        auto mac1=suite->createMAC(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(mac1);
        mac1->setKey(key);
        ec=mac1->runSign(inputData,tag);
        HATN_REQUIRE(!ec);
        if (config->expectedFailure)
        {
            BOOST_CHECK(tag!=outputData);
        }
        else
        {
            BOOST_CHECK(tag==outputData);
        }

        auto mac2=suite->createMAC(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(mac2);
        mac2->setKey(key);
        ec=mac2->runVerify(inputData,outputData);
        if (config->expectedFailure)
        {
            BOOST_CHECK(ec);
        }
        else
        {
            BOOST_CHECK(!ec);
        }
    }
}

static void checkMacKey(ContainerFormat format)
{
    std::function<std::vector<std::string> (std::shared_ptr<CryptPlugin>& plugin)>
            f1=[](std::shared_ptr<CryptPlugin>& plugin){return plugin->listMACs();};
    std::function<void (std::shared_ptr<CryptPlugin>&, AlgConfig& config, const std::string&, const std::string&)> f2=
    [](std::shared_ptr<CryptPlugin>& plugin, AlgConfig& config, const std::string& algName, const std::string& path)
    {
        algHandler(plugin,config,algName,path);
    };
    std::function<common::SharedPtr<SecureKey> (const CryptAlgorithm*)>
    f3=[](const CryptAlgorithm *alg)
    {
        return alg->createMACKey();
    };
    std::function<void (CipherSuite*,SecureKey*,ByteArray&,ByteArray&,AlgConfig*)>
    f4=[](CipherSuite* suite, const SecureKey* key, ByteArray& inputData, ByteArray& outputData,AlgConfig* config)
    {
        testMAC(suite,key,inputData,outputData,config);
    };

    AlgConfig config{CryptAlgorithm::Type::MAC,"mac",Crypt::Feature::MAC,
                format,
                f1,
                f2,
                f3,
                f4,
                format!=ContainerFormat::RAW_ENCRYPTED
                };
    testAlgKey(config);
}

static void testSignature(CipherSuite* suite, const SecureKey* keyIn, ByteArray& inputData, ByteArray& outputData, AlgConfig* config)
{
    auto key=dynamic_cast<const PrivateKey*>(keyIn);
    HATN_REQUIRE(key);
    const CryptAlgorithm* alg=nullptr;
    auto ec=suite->signatureAlgorithm(alg);
    HATN_REQUIRE(!ec);
    const CryptAlgorithm* digestAlg=nullptr;
    ec=suite->digestAlgorithm(digestAlg);
    HATN_REQUIRE(!ec);

    if (outputData.empty())
    {
        auto signProcessor=alg->createSignatureSign();
        BOOST_REQUIRE(signProcessor);
        signProcessor->setKey(key);
        signProcessor->setDigestAlg(digestAlg);
        ec=signProcessor->sign(SpanBuffer(inputData),outputData);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_CHECK(!ec);
    }
    else
    {
        auto signProcessor=alg->createSignatureSign();
        BOOST_REQUIRE(signProcessor);
        signProcessor->setKey(key);
        signProcessor->setDigestAlg(digestAlg);
        ByteArray sig;
        ec=signProcessor->sign(SpanBuffer(inputData),sig);
        if (!config->expectedFailure)
        {
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
        }
        else if (ec)
        {
            return;
        }

        auto verifyProcessor=alg->createSignatureVerify();
        BOOST_REQUIRE(verifyProcessor);
        verifyProcessor->setKey(config->pubkey);
        verifyProcessor->setDigestAlg(digestAlg);
        ec=verifyProcessor->verify(SpanBuffer(inputData),outputData);
        if (config->expectedFailureSigPub)
        {
            BOOST_CHECK(ec);
        }
        else
        {
            BOOST_CHECK(!ec);
        }
        ec=verifyProcessor->verify(SpanBuffer(inputData),sig);
        if (config->expectedFailure || config->expectedFailureSigPub)
        {
            BOOST_CHECK(ec);
        }
        else
        {
            BOOST_CHECK(!ec);
        }
    }
}

static void checkSignatureKey(ContainerFormat format, bool encrypted)
{
    std::function<std::vector<std::string> (std::shared_ptr<CryptPlugin>& plugin)>
            f1=[](std::shared_ptr<CryptPlugin>& plugin){return plugin->listSignatures();};
    std::function<void (std::shared_ptr<CryptPlugin>&, AlgConfig& config, const std::string&, const std::string&)> f2=
    [](std::shared_ptr<CryptPlugin>& plugin, AlgConfig& config, const std::string& algName, const std::string& path)
    {
        algHandler(plugin,config,algName,path);
    };
    std::function<common::SharedPtr<SecureKey> (const CryptAlgorithm*)>
    f3=[](const CryptAlgorithm *alg)
    {
        return alg->createPrivateKey();
    };
    std::function<void (CipherSuite*,SecureKey*,ByteArray&,ByteArray&,AlgConfig*)>
    f4=[](CipherSuite* suite, const SecureKey* key, ByteArray& inputData, ByteArray& outputData, AlgConfig* config)
    {
        testSignature(suite,key,inputData,outputData,config);
    };

    AlgConfig config{CryptAlgorithm::Type::SIGNATURE,"signature",Crypt::Feature::Signature,
                format,
                f1,
                f2,
                f3,
                f4,
                !encrypted
                };

    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [&config](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(config.feature))
            {
                std::vector<std::string> algs{
                    "rsa/3072","dsa/2048","ec/prime256v1",
                    "ed/25519"
                };
                for (auto&& it:algs)
                {
                    CipherSuites::instance().reset();
                    std::string algPathName=it;
                    boost::algorithm::replace_all(algPathName,std::string("/"),std::string("-"));
                    boost::algorithm::replace_all(algPathName,std::string(":"),std::string("-"));

                    std::string path=fmt::format("{}/key-export-import-{}-{}",PluginList::assetsPath("crypt"),config.typeStr,algPathName);
                    config.testHandler(plugin,config,it,path);
                    path=fmt::format("{}/key-export-import-{}-{}",PluginList::assetsPath("crypt",plugin->info()->name),config.typeStr,algPathName);
                    config.testHandler(plugin,config,it,path);

                    CipherSuites::instance().reset();
                }
            }
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckCipherKeyPlain)
{
    checkCipherKey(ContainerFormat::RAW_PLAIN);
}

BOOST_AUTO_TEST_CASE(CheckCipherKeyEncrypted)
{
    checkCipherKey(ContainerFormat::RAW_ENCRYPTED);
}

BOOST_AUTO_TEST_CASE(CheckAeadKeyPlain)
{
    checkAeadKey(ContainerFormat::RAW_PLAIN);
}

BOOST_AUTO_TEST_CASE(CheckAeadKeyEncrypted)
{
    checkAeadKey(ContainerFormat::RAW_ENCRYPTED);
}

BOOST_AUTO_TEST_CASE(CheckMacKeyPlain)
{
    checkMacKey(ContainerFormat::RAW_PLAIN);
}

BOOST_AUTO_TEST_CASE(CheckMacKeyEncrypted)
{
    checkMacKey(ContainerFormat::RAW_ENCRYPTED);
}

BOOST_AUTO_TEST_CASE(CheckSignatureKeyPem)
{
    checkSignatureKey(ContainerFormat::PEM,false);
}

BOOST_AUTO_TEST_CASE(CheckSignatureKeyPemEncrypted)
{
    checkSignatureKey(ContainerFormat::PEM,true);
}

BOOST_AUTO_TEST_CASE(CheckSignatureKeyDer)
{
    checkSignatureKey(ContainerFormat::DER,false);
}

BOOST_AUTO_TEST_SUITE_END()

}
}
