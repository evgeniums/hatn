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

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/cryptcontainer.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

#ifdef _WIN32
#ifndef _MSC_VER
#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#endif
#endif

// #define HATN_SAVE_TEST_FILES

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

BOOST_FIXTURE_TEST_SUITE(TestCryptContainer,CryptTestFixture)

static void checkCryptContainer(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    BOOST_TEST_MESSAGE(fmt::format("Checking test vectors in {}",path));
    size_t runCount=0;

    // up to 16 test bundles
    for (size_t i=1u;i<=16u;i++)
    {
        std::string cipherSuiteFile=fmt::format("{}/cryptcontainer-ciphersuite{}.json",path,i);
        std::string plainTextFile=fmt::format("{}/cryptcontainer-plaintext{}.dat",path,i);
        std::string cipherTextFile=fmt::format("{}/cryptcontainer-ciphertext{}.dat",path,i);
        std::string configFile=fmt::format("{}/cryptcontainer-config{}.dat",path,i);

        if (boost::filesystem::exists(cipherSuiteFile)
            &&
            boost::filesystem::exists(plainTextFile)
            &&
            boost::filesystem::exists(cipherTextFile)
            &&
            boost::filesystem::exists(configFile)
           )
        {
            BOOST_TEST_MESSAGE(fmt::format("Testing vector #{}",i));

            // parse config
            auto configStr=PluginList::linefromFile(configFile);
            std::vector<std::string> parts;
            Utils::trimSplit(parts,configStr,':');
            HATN_REQUIRE_GE(parts.size(),7u);
            const auto& suiteID=parts[0];
            const auto& masterKeyStr=parts[1];
            const auto& kdfTypeStr=parts[2];
            auto kdfTypeInt=std::stoul(kdfTypeStr);
            HATN_REQUIRE(kdfTypeInt==0 || kdfTypeInt==1 || kdfTypeInt==2);
            auto kdfType=static_cast<container_descriptor::KdfType>(kdfTypeInt);
            const auto& salt=parts[3];
            const auto& maxFirstChunkSizeStr=parts[4];
            auto maxFirstChunkSize=std::stoul(maxFirstChunkSizeStr);
            const auto& maxChunkSizeStr=parts[5];
            auto maxChunkSize=std::stoul(maxChunkSizeStr);
            const auto& attachCipherSuiteStr=parts[6];
            auto attachCipherSuite=static_cast<bool>(std::stoul(attachCipherSuiteStr));
            bool defaultChunkSizes=false;
            if (parts.size()>7)
            {
                const auto& defaultChunkSizesStr=parts[7];
                defaultChunkSizes=static_cast<bool>(std::stoul(defaultChunkSizesStr));
            }

            // load suite from json
            ByteArray cipherSuiteJson;
            auto ec=cipherSuiteJson.loadFromFile(cipherSuiteFile);
            HATN_REQUIRE(!ec);
            auto createSuite=std::make_shared<CipherSuite>();
            ec=createSuite->loadFromJSON(cipherSuiteJson);
            HATN_REQUIRE(!ec);
            BOOST_CHECK_EQUAL(suiteID,std::string(createSuite->id()));
            // check json
            std::string json;
            bool jsonParsed=createSuite->toJSON(json,true);
            BOOST_CHECK(jsonParsed);
#if 0
            BOOST_TEST_MESSAGE(json);
#endif
            // check serializing/deserializing of the suite
            ByteArray suiteData;
            ec=createSuite->store(suiteData);
            BOOST_CHECK(!ec);
            auto checkSuite=std::make_shared<CipherSuite>();
            ec=checkSuite->load(suiteData);
            BOOST_CHECK(!ec);
            std::string json1;
            jsonParsed=createSuite->toJSON(json1,true);
            BOOST_CHECK(jsonParsed);
            BOOST_CHECK_EQUAL(json,json1);
            checkSuite.reset();

            // add suite to table of suites
            CipherSuitesGlobal::instance().addSuite(createSuite);
            auto suiteCopy=createSuite;

            // set engine
            auto engine=std::make_shared<CryptEngine>(plugin.get());
            CipherSuitesGlobal::instance().setDefaultEngine(std::move(engine));

            // check suite
            auto suite=CipherSuitesGlobal::instance().suite(suiteID.c_str());
            HATN_REQUIRE(suite);

            // check AEAD algorithm
            const CryptAlgorithm* aeadAlg=nullptr;
            ec=suite->aeadAlgorithm(aeadAlg);
            if (ec)
            {
                continue;
            }
            HATN_REQUIRE(aeadAlg);

            // check KDF algorithm and setup master key
            common::SharedPtr<SymmetricKey> masterKey;
            common::SharedPtr<SymmetricKey> masterKey2;
            const CryptAlgorithm* kdfAlg=nullptr;
            if (kdfType==container_descriptor::KdfType::PBKDF)
            {
                ec=suite->pbkdfAlgorithm(kdfAlg);
                if (ec)
                {
                    continue;
                }
                masterKey=plugin->createPassphraseKey();
                HATN_REQUIRE(masterKey);
                ec=masterKey->importFromBuf(masterKeyStr,ContainerFormat::RAW_PLAIN);
                HATN_REQUIRE(!ec)
                auto masterKey2_=plugin->createPassphraseKey();
                masterKey2=masterKey2_.staticCast<SymmetricKey>();
                HATN_REQUIRE(masterKey2);
                auto gen=plugin->createPasswordGenerator();
                HATN_REQUIRE(gen);
                ec=masterKey2_->generatePassword(gen.get());
                BOOST_CHECK(!ec);
            }
            else
            {
                ec=suite->hkdfAlgorithm(kdfAlg);
                if (ec)
                {
                    continue;
                }
                masterKey=aeadAlg->createSymmetricKey();

                HATN_REQUIRE(masterKey);
                ByteArray masterKeyData;
                ContainerUtils::hexToRaw(masterKeyStr,masterKeyData);
                ec=masterKey->importFromBuf(masterKeyData,ContainerFormat::RAW_PLAIN);
                HATN_REQUIRE(!ec)
                masterKey2=aeadAlg->createSymmetricKey();
                HATN_REQUIRE(masterKey2);
                ec=masterKey2->generate();
                BOOST_CHECK(!ec);
            }
            HATN_REQUIRE(kdfAlg);

            // prepare crypt container
            CryptContainer container1(masterKey.get(),suite);
            BOOST_CHECK_EQUAL(container1.chunkMaxSize(),0x40000u);
            BOOST_CHECK_EQUAL(container1.firstChunkMaxSize(),0x1000u);
            if (!defaultChunkSizes)
            {
                container1.setChunkMaxSize(maxChunkSize);
                BOOST_CHECK_EQUAL(container1.chunkMaxSize(),maxChunkSize);
                container1.setFirstChunkMaxSize(maxFirstChunkSize);
                BOOST_CHECK_EQUAL(container1.firstChunkMaxSize(),maxFirstChunkSize);
            }
            auto containerSalt=container1.salt();
            std::string containerSaltStr(containerSalt.data(),containerSalt.size());
            BOOST_CHECK_EQUAL(containerSaltStr,std::string());
            container1.setSalt(salt);
            auto containerSalt1=container1.salt();
            std::string containerSaltStr1(containerSalt1.data(),containerSalt1.size());
            BOOST_CHECK_EQUAL(containerSaltStr1,salt);
            BOOST_CHECK_EQUAL(container1.isAttachCipherSuiteEnabled(),false);
            container1.setAttachCipherSuiteEnabled(attachCipherSuite);
            BOOST_CHECK_EQUAL(container1.isAttachCipherSuiteEnabled(),attachCipherSuite);
            BOOST_CHECK_EQUAL(static_cast<int>(container1.kdfType()),static_cast<int>(container_descriptor::KdfType::PbkdfThenHkdf));
            container1.setKdfType(kdfType);
            BOOST_CHECK_EQUAL(static_cast<int>(container1.kdfType()),static_cast<int>(kdfType));

            // pack data
            ByteArray plaintext1;
            ByteArray ciphertext1;
            ec=plaintext1.loadFromFile(plainTextFile);
            BOOST_CHECK(!ec);
            ec=container1.pack(plaintext1,ciphertext1);
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            HATN_REQUIRE(!ec);

#ifdef HATN_SAVE_TEST_FILES
            ec=ciphertext1.saveToFile(cipherTextFile);
            BOOST_CHECK(!ec);
#endif
            // check packed data
            ByteArray sampleCiphertext1;
            ec=sampleCiphertext1.loadFromFile(cipherTextFile);
            BOOST_CHECK(!ec);

            // unpack data
            ByteArray plaintext2;
            ec=container1.unpack(sampleCiphertext1,plaintext2);
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            HATN_REQUIRE(!ec);

            // check unpacked data
            BOOST_CHECK(plaintext1==plaintext2);

            // check different container ctors
            CryptContainer container2(suite);
            BOOST_CHECK(container2.cipherSuite()==suite);
            BOOST_CHECK(container2.masterKey()==nullptr);
            container2.setMasterKey(masterKey.get());
            BOOST_CHECK(container2.masterKey()==masterKey.get());
            container2.setKdfType(kdfType);
            BOOST_CHECK_EQUAL(static_cast<int>(container2.kdfType()),static_cast<int>(kdfType));
            ByteArray plaintext3;
            ec=container2.unpack(sampleCiphertext1,plaintext3);
            BOOST_CHECK(!ec);
            BOOST_CHECK(plaintext1==plaintext3);

            CryptContainer container3;
            BOOST_CHECK(container3.cipherSuite()==nullptr);
            BOOST_CHECK(container3.masterKey()==nullptr);
            container3.setMasterKey(masterKey.get());
            container3.setCipherSuite(suite);
            BOOST_CHECK(container3.cipherSuite()==suite);
            BOOST_CHECK(container3.masterKey()==masterKey.get());
            container3.setKdfType(kdfType);
            BOOST_CHECK_EQUAL(static_cast<int>(container3.kdfType()),static_cast<int>(kdfType));
            ByteArray plaintext4;
            ec=container3.unpack(sampleCiphertext1,plaintext4);
            BOOST_CHECK(!ec);
            BOOST_CHECK(plaintext1==plaintext4);

            CryptContainer container4(masterKey.get());
            BOOST_CHECK(container4.cipherSuite()==nullptr);
            BOOST_CHECK(container4.masterKey()==masterKey.get());
            ByteArray plaintext5;
            ec=container4.unpack(sampleCiphertext1,plaintext5);
            BOOST_CHECK(!ec);
            BOOST_CHECK(plaintext1==plaintext5);

            // check unknown key
            CryptContainer container5;
            ByteArray plaintext6;
            ec=container5.unpack(sampleCiphertext1,plaintext6);
            BOOST_CHECK(ec);

            // check invalid key
            CryptContainer container6(masterKey2.get());
            ByteArray plaintext7;
            ec=container6.unpack(sampleCiphertext1,plaintext7);
            BOOST_CHECK(ec);

            // check mailformed ciphertext
            CryptContainer container7(masterKey.get());
            ByteArray plaintext8;
            ByteArray mailformedCipherText1=sampleCiphertext1;
            if (mailformedCipherText1.size()>10)
            {
                mailformedCipherText1[mailformedCipherText1.size()/2-4]='a';
                mailformedCipherText1[mailformedCipherText1.size()/2-3]='5';
                mailformedCipherText1[mailformedCipherText1.size()/2-2]='a';
                mailformedCipherText1[mailformedCipherText1.size()/2-1]='5';
                mailformedCipherText1[mailformedCipherText1.size()/2]='a';
                mailformedCipherText1[mailformedCipherText1.size()/2+1]='b';
                mailformedCipherText1[mailformedCipherText1.size()/2+2]='c';
                mailformedCipherText1[mailformedCipherText1.size()/2+3]='d';
                mailformedCipherText1[mailformedCipherText1.size()/2+4]='e';

                ec=container7.unpack(mailformedCipherText1,plaintext8);
                BOOST_CHECK(ec);
            }

            // check invalid size of cipher test
            CryptContainer container8(masterKey.get());
            ByteArray plaintext9;
            ByteArray mailformedCipherText2=sampleCiphertext1;
            mailformedCipherText2.left(mailformedCipherText2.size()-5);
            ec=container8.unpack(mailformedCipherText2,plaintext9);
            BOOST_CHECK(ec);

            // check invalid header of cipher test
            CryptContainer container9(masterKey.get());
            ByteArray plaintext10;
            ByteArray mailformedCipherText3=sampleCiphertext1;
            HATN_REQUIRE(mailformedCipherText3.size()>=container9.headerSize());
            mailformedCipherText3[5]='a';
            mailformedCipherText3[6]='b';
            mailformedCipherText3[7]='c';
            ec=container9.unpack(mailformedCipherText3,plaintext10);
            BOOST_CHECK(ec);
            CryptContainer container10(masterKey.get());
            ByteArray plaintext11;
            ByteArray mailformedCipherText4=sampleCiphertext1;
            HATN_REQUIRE(mailformedCipherText4.size()>=container10.headerSize());
            mailformedCipherText4[0]='Q';
            ec=container10.unpack(mailformedCipherText4,plaintext11);
            BOOST_CHECK(ec);
            CryptContainer container11(masterKey.get());
            ByteArray plaintext12;
            ByteArray mailformedCipherText5=sampleCiphertext1;
            HATN_REQUIRE(mailformedCipherText5.size()>=container11.headerSize());
            mailformedCipherText5[CryptContainerHeader::plaintextSizeOffset()+3]=static_cast<char>(0x55);
            ec=container11.unpack(mailformedCipherText5,plaintext12);
            BOOST_CHECK(ec);
            CryptContainer container12(masterKey.get());
            ByteArray plaintext13;
            ByteArray mailformedCipherText6=sampleCiphertext1;
            HATN_REQUIRE(mailformedCipherText6.size()>=container12.headerSize());
            mailformedCipherText6[CryptContainerHeader::descriptorSizeOffset()]=static_cast<char>(0xaa);
            ec=container12.unpack(mailformedCipherText6,plaintext13);
            BOOST_CHECK(ec);

            // check invalid descriptor
            CryptContainer container13(masterKey.get());
            ByteArray plaintext14;
            ByteArray mailformedCipherText7=sampleCiphertext1;
            HATN_REQUIRE(mailformedCipherText7.size()>=container13.headerSize()+5);
            mailformedCipherText7[container13.headerSize()+4]=static_cast<char>(0xee);
            ec=container13.unpack(mailformedCipherText7,plaintext14);
            BOOST_CHECK(ec);

            //! check not existing cipher suite
            CipherSuitesGlobal::instance().reset();
            CryptContainer container14(masterKey.get());
            ByteArray plaintext15;
            ec=container14.unpack(ciphertext1,plaintext15);
            BOOST_CHECK(ec);
            if (attachCipherSuite)
            {
                engine=std::make_shared<CryptEngine>(plugin.get());
                CipherSuitesGlobal::instance().setDefaultEngine(std::move(engine));
                plaintext15.clear();
                ec=container14.unpack(ciphertext1,plaintext15);
                BOOST_CHECK(!ec);
                CipherSuitesGlobal::instance().reset();
            }

            //! check default cipher suite
            CipherSuitesGlobal::instance().setDefaultSuite(std::move(suiteCopy));
            CryptContainer container15(masterKey.get());
            container15.setCipherSuite(CipherSuitesGlobal::instance().defaultSuite());
            container15.setKdfType(kdfType);
            if (!defaultChunkSizes)
            {
                container15.setChunkMaxSize(maxChunkSize);
                container15.setFirstChunkMaxSize(maxFirstChunkSize);
            }
            container15.setAttachCipherSuiteEnabled(attachCipherSuite);
            ByteArray ciphertext15;
            ec=container15.pack(plaintext1,ciphertext15,salt);
            BOOST_CHECK(!ec);
            if (attachCipherSuite)
            {
                engine=std::make_shared<CryptEngine>(plugin.get());
                CipherSuitesGlobal::instance().setDefaultEngine(std::move(engine));
            }
            CryptContainer container16(masterKey.get());
            ByteArray plaintext16;
            ec=container16.unpack(ciphertext15,plaintext16);
            BOOST_CHECK(!ec);
            BOOST_CHECK(plaintext1==plaintext16);

            // reset suites
            CipherSuitesGlobal::instance().reset();

            BOOST_TEST_MESSAGE(fmt::format("Done vector #{}",i));
            ++runCount;
        }
    }
    BOOST_TEST_MESSAGE(fmt::format("Tested {} vectors",runCount));
}

BOOST_AUTO_TEST_CASE(CheckCryptContainer)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            CipherSuitesGlobal::instance().reset();
            checkCryptContainer(plugin,PluginList::assetsPath("crypt"));
            CipherSuitesGlobal::instance().reset();
            checkCryptContainer(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            CipherSuitesGlobal::instance().reset();
        }
    );
}

static void checkAutoSalt(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    std::string cipherSuiteFile=fmt::format("{}/cryptcontainer-ciphersuite1.json",path);
    std::string plainTextFile=fmt::format("{}/cryptcontainer-plaintext1.dat",path);
    std::string cipherTextFile=fmt::format("{}/cryptcontainer-ciphertext1.dat",path);
    std::string configFile=fmt::format("{}/cryptcontainer-config1.dat",path);

    if (boost::filesystem::exists(cipherSuiteFile)
        &&
        boost::filesystem::exists(plainTextFile)
        &&
        boost::filesystem::exists(cipherTextFile)
        &&
        boost::filesystem::exists(configFile)
        )
    {
        // parse config
        auto configStr=PluginList::linefromFile(configFile);
        std::vector<std::string> parts;
        Utils::trimSplit(parts,configStr,':');
        BOOST_REQUIRE_GE(parts.size(),7u);
        const auto& masterKeyStr=parts[1];
        const auto& kdfTypeStr=parts[2];
        auto kdfTypeInt=std::stoul(kdfTypeStr);
        auto kdfType=static_cast<container_descriptor::KdfType>(kdfTypeInt);

        // load suite from json
        ByteArray cipherSuiteJson;
        auto ec=cipherSuiteJson.loadFromFile(cipherSuiteFile);
        BOOST_REQUIRE(!ec);
        auto suite=std::make_shared<CipherSuite>();
        ec=suite->loadFromJSON(cipherSuiteJson);
        BOOST_REQUIRE(!ec);

        // add suite to table of suites
        CipherSuitesGlobal::instance().addSuite(suite);

        // set engine
        auto engine=std::make_shared<CryptEngine>(plugin.get());
        CipherSuitesGlobal::instance().setDefaultEngine(std::move(engine));

        // check AEAD algorithm
        const CryptAlgorithm* aeadAlg=nullptr;
        ec=suite->aeadAlgorithm(aeadAlg);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE(aeadAlg);

        // load key
        common::SharedPtr<SymmetricKey> masterKey;
        const CryptAlgorithm* kdfAlg=nullptr;
        if (kdfType==container_descriptor::KdfType::PBKDF)
        {
            ec=suite->pbkdfAlgorithm(kdfAlg);
            BOOST_REQUIRE(!ec);
            masterKey=plugin->createPassphraseKey();
            BOOST_REQUIRE(masterKey);
            ec=masterKey->importFromBuf(masterKeyStr,ContainerFormat::RAW_PLAIN);
            BOOST_REQUIRE(!ec);
        }
        else
        {
            ec=suite->hkdfAlgorithm(kdfAlg);
            BOOST_REQUIRE(!ec);
            masterKey=aeadAlg->createSymmetricKey();
            BOOST_REQUIRE(masterKey);
            ByteArray masterKeyData;
            ContainerUtils::hexToRaw(masterKeyStr,masterKeyData);
            ec=masterKey->importFromBuf(masterKeyData,ContainerFormat::RAW_PLAIN);
            BOOST_REQUIRE(!ec);
        }

        // pack data
        ByteArray plaintext1;
        ByteArray ciphertext1;
        ec=plaintext1.loadFromFile(plainTextFile);
        BOOST_CHECK(!ec);
        CryptContainer container1(masterKey.get(),suite.get());
        ec=container1.pack(plaintext1,ciphertext1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // check if salt not empty
        ByteArray salt1(container1.salt().data(),container1.salt().size());
        BOOST_CHECK(!salt1.isEmpty());
        BOOST_CHECK(salt1.size()>=8 && salt1.size()<=16);

        // unpack data
        ByteArray plaintext2;
        CryptContainer container2(masterKey.get(),suite.get());
        ec=container2.unpack(ciphertext1,plaintext2);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(plaintext1==plaintext2);
    }
}

BOOST_AUTO_TEST_CASE(CheckAutoSalt)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            CipherSuitesGlobal::instance().reset();
            checkAutoSalt(plugin,PluginList::assetsPath("crypt"));
            CipherSuitesGlobal::instance().reset();
            checkAutoSalt(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            CipherSuitesGlobal::instance().reset();
        }
        );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
