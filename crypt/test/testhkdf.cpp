/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/hex.hpp>

#include <rapidjson/document.h>

#include <hatn_test_config.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/format.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/crypt/cryptplugin.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

namespace
{

struct HKDFDescriptor
{
    std::string hash;
    std::string ikm;
    std::string salt;
    std::string info;
    std::string prk;
    std::string okm;

    std::string cipher;
    std::string hmac;

    ByteArray ikmBin() const
    {
        ByteArray b;
        ContainerUtils::hexToRaw(ikm,b);
        return b;
    }
    ByteArray saltBin() const
    {
        ByteArray b;
        ContainerUtils::hexToRaw(salt,b);
        return b;
    }
    ByteArray infoBin() const
    {
        ByteArray b;
        ContainerUtils::hexToRaw(info,b);
        return b;
    }
};

bool loadHKDFDescriptor(HKDFDescriptor& descr, const std::string& path)
{
    std::string fileName=fmt::format("{}/hkdf-json.txt",path);
    if (!boost::filesystem::exists(fileName))
    {
        return false;
    }

    ByteArray b;
    auto ec=b.loadFromFile(fileName);
    if (ec)
    {
        BOOST_TEST_MESSAGE("Failed to load HKDF descriptor from file");
        return false;
    }

    rapidjson::Document doc;
    doc.Parse(b.c_str());
    if (!doc.IsObject())
    {
        BOOST_TEST_MESSAGE("Failed to parse HKDF descriptor JSON");
        return false;
    }

    auto checkField=[&doc](const std::string& field)
    {
        return doc.HasMember(field.c_str())&&doc[field.c_str()].IsString();
    };
    bool ok=checkField("hash")&&
            checkField("ikm")&&
            checkField("salt")&&
            checkField("info")&&
            checkField("prk")&&
            checkField("okm")&&
            checkField("cipher");
    if (!ok)
    {
        BOOST_TEST_MESSAGE("Failed to find required fields in HKDF descriptor");
        return false;
    }

    descr.hash=doc["hash"].GetString();
    descr.ikm=doc["ikm"].GetString();
    descr.salt=doc["salt"].GetString();
    descr.info=doc["info"].GetString();
    descr.prk=doc["prk"].GetString();
    descr.okm=doc["okm"].GetString();
    descr.cipher=doc["cipher"].GetString();

    if (checkField("hmac"))
    {
        descr.hmac=doc["hmac"].GetString();
    }

    return true;
}

}

BOOST_FIXTURE_TEST_SUITE(TestHkdf,CryptTestFixture)

static void checkHKDFVectors(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    HKDFDescriptor descr;
    if (!loadHKDFDescriptor(descr,path))
    {
        return;
    }

    const CryptAlgorithm* cipherAlg=nullptr;
    BOOST_TEST_MESSAGE(fmt::format("Cipher alg {}",descr.cipher));
    auto ec=plugin->findAlgorithm(cipherAlg,CryptAlgorithm::Type::SENCRYPTION,descr.cipher);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Cipher alg {} is not supported by plugin {}",descr.cipher,plugin->info()->name));
        return;
    }

    const CryptAlgorithm* digestAlg=nullptr;
    BOOST_TEST_MESSAGE(fmt::format("Digest alg {}",descr.hash));
    ec=plugin->findAlgorithm(digestAlg,CryptAlgorithm::Type::DIGEST,descr.hash);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Digest alg {} is not supported by plugin {}",descr.hash,plugin->info()->name));
        return;
    }

    auto masterKey1=cipherAlg->createSymmetricKey();
    HATN_REQUIRE(masterKey1);
    ec=masterKey1->importFromBuf(descr.ikmBin(),ContainerFormat::RAW_PLAIN);
    BOOST_CHECK(!ec);
    auto kdf1=plugin->createHKDF(cipherAlg,digestAlg);
    HATN_REQUIRE(!kdf1.isNull());

    kdf1->setMode(HKDF::Mode::Extract_Expand_All);
    ec=kdf1->init(masterKey1.get(),descr.saltBin());
    BOOST_CHECK(!ec);
    common::SharedPtr<SymmetricKey> derivedKey1;
    ec=kdf1->derive(derivedKey1,descr.infoBin());
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!derivedKey1.isNull());
    BOOST_CHECK(!derivedKey1->isNull());
    MemoryLockedArray okmBin;
    ec=derivedKey1->exportToBuf(okmBin,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!okmBin.isEmpty());
    std::string okm;
    ContainerUtils::rawToHex(okmBin.stringView(),okm,true);
    BOOST_CHECK(descr.okm.size()>=okm.size());
    if (descr.okm.size()>=okm.size())
    {
        std::string okmSample=descr.okm.substr(0,okm.size());
        BOOST_CHECK_EQUAL(okmSample,okm);
    }

    kdf1->setMode(HKDF::Mode::Extract_All);
    ec=kdf1->init(masterKey1.get(),descr.saltBin());
    BOOST_CHECK(!ec);
    ec=kdf1->derive(derivedKey1,descr.infoBin());
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!derivedKey1.isNull());
    BOOST_CHECK(!derivedKey1->isNull());
    MemoryLockedArray prkBin;
    ec=derivedKey1->exportToBuf(prkBin,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!prkBin.isEmpty());
    std::string prk;
    ContainerUtils::rawToHex(prkBin.stringView(),prk,true);
    BOOST_CHECK(descr.prk.size()>=prk.size());
    if (descr.prk.size()>=prk.size())
    {
        std::string prkSample=descr.prk.substr(0,prk.size());
        BOOST_CHECK_EQUAL(prkSample,prk);
    }

    kdf1->setMode(HKDF::Mode::First_Extract_Then_Expand);
    ec=kdf1->init(masterKey1.get(),descr.saltBin());
    BOOST_CHECK(!ec);
    ec=kdf1->derive(derivedKey1,descr.infoBin());
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!derivedKey1.isNull());
    BOOST_CHECK(!derivedKey1->isNull());
    okmBin.clear();
    ec=derivedKey1->exportToBuf(okmBin,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!okmBin.isEmpty());
    okm.clear();
    ContainerUtils::rawToHex(okmBin.stringView(),okm,true);
    BOOST_CHECK(descr.okm.size()>=okm.size());
    if (descr.okm.size()>=okm.size())
    {
        std::string okmSample=descr.okm.substr(0,okm.size());
        BOOST_CHECK_EQUAL(okmSample,okm);
    }

    kdf1->setMode(HKDF::Mode::Expand_All);
    ByteArray samplePrkBin;
    ContainerUtils::hexToRaw(descr.prk,samplePrkBin);
    ec=masterKey1->importFromBuf(samplePrkBin);
    BOOST_CHECK(!ec);
    ec=kdf1->init(masterKey1.get(),descr.saltBin());
    BOOST_CHECK(!ec);
    ec=kdf1->derive(derivedKey1,descr.infoBin());
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!derivedKey1.isNull());
    BOOST_CHECK(!derivedKey1->isNull());
    okmBin.clear();
    ec=derivedKey1->exportToBuf(okmBin,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!okmBin.isEmpty());
    okm.clear();
    ContainerUtils::rawToHex(okmBin.stringView(),okm,true);
    BOOST_CHECK(descr.okm.size()>=okm.size());
    if (descr.okm.size()>=okm.size())
    {
        std::string okmSample=descr.okm.substr(0,okm.size());
        BOOST_CHECK_EQUAL(okmSample,okm);
    }
}

static void checkHKDFKeys(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    HKDFDescriptor descr;
    if (!loadHKDFDescriptor(descr,path))
    {
        return;
    }

    const CryptAlgorithm* cipherAlg=nullptr;
    BOOST_TEST_MESSAGE(fmt::format("Cipher alg {}",descr.cipher));
    auto ec=plugin->findAlgorithm(cipherAlg,CryptAlgorithm::Type::SENCRYPTION,descr.cipher);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Cipher alg {} is not supported by plugin {}",descr.cipher,plugin->info()->name));
        return;
    }

    const CryptAlgorithm* digestAlg=nullptr;
    BOOST_TEST_MESSAGE(fmt::format("Digest alg {}",descr.hash));
    ec=plugin->findAlgorithm(digestAlg,CryptAlgorithm::Type::DIGEST,descr.hash);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Digest alg {} is not supported by plugin {}",descr.hash,plugin->info()->name));
        return;
    }

    ByteArray keyData="Hello world from Dracosha!";

    auto masterKey1=cipherAlg->createSymmetricKey();
    HATN_REQUIRE(masterKey1);
    ec=masterKey1->importFromBuf(keyData,ContainerFormat::RAW_PLAIN);
    BOOST_CHECK(!ec);

    auto kdf1=plugin->createHKDF(cipherAlg,digestAlg);
    HATN_REQUIRE(!kdf1.isNull());
    ec=kdf1->init(masterKey1.get(),descr.saltBin());
    BOOST_CHECK(!ec);
    common::SharedPtr<SymmetricKey> derivedKey1;
    ec=kdf1->derive(derivedKey1,descr.infoBin());
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!derivedKey1.isNull());
    BOOST_CHECK(!derivedKey1->isNull());
    MemoryLockedArray buf1;
    ec=derivedKey1->exportToBuf(buf1,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!buf1.isEmpty());
    BOOST_CHECK_EQUAL(buf1.size(),cipherAlg->keySize());

    // check key derivation for hmac alg

    const CryptAlgorithm* hmacAlg=nullptr;
    BOOST_TEST_MESSAGE(fmt::format("HMAC alg {}",descr.hmac));
    ec=plugin->findAlgorithm(hmacAlg,CryptAlgorithm::Type::HMAC,descr.hmac);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("HMAC alg {} is not supported by plugin {}",descr.hmac,plugin->info()->name));
        return;
    }
    auto masterKey2=hmacAlg->createMACKey();
    HATN_REQUIRE(masterKey2);
    ec=masterKey2->importFromBuf(keyData,ContainerFormat::RAW_PLAIN);
    BOOST_CHECK(!ec);

    auto kdf2=plugin->createHKDF(hmacAlg,digestAlg);
    HATN_REQUIRE(!kdf2.isNull());
    ec=kdf2->init(masterKey2.get(),descr.saltBin());
    BOOST_CHECK(!ec);
    common::SharedPtr<SymmetricKey> derivedKey2;
    ec=kdf2->derive(derivedKey2,descr.infoBin());
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!derivedKey2.isNull());
    BOOST_CHECK(!derivedKey2->isNull());
    MemoryLockedArray buf2;
    ec=derivedKey2->exportToBuf(buf2,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!buf2.isEmpty());
    BOOST_CHECK_EQUAL(buf2.size(),hmacAlg->keySize());
    MemoryLockedArray buf3;
    buf3.load(buf2);
    auto keySize=(std::max)(buf1.size(),buf2.size());
    buf1.resize(keySize);
    buf2.resize(keySize);
    BOOST_CHECK(buf1==buf2);

    ec=kdf2->derive(derivedKey2,ByteArray("hello info"));
    MemoryLockedArray buf2_1;
    ec=derivedKey2->exportToBuf(buf2_1,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!buf2_1.isEmpty());
    BOOST_CHECK_EQUAL(buf2_1.size(),hmacAlg->keySize());
    BOOST_CHECK(buf2_1!=buf3);

    // check derive and replace

    ec=kdf2->deriveAndReplace(derivedKey2,ByteArray("info"),ByteArray("salt"));
    MemoryLockedArray buf4;
    ec=derivedKey2->exportToBuf(buf4,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!buf4.isEmpty());
    BOOST_CHECK_EQUAL(buf4.size(),hmacAlg->keySize());
    BOOST_CHECK(buf3!=buf4);

    ec=kdf2->derive(derivedKey2,ByteArray("hello info"));
    MemoryLockedArray buf5;
    ec=derivedKey2->exportToBuf(buf5,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!buf5.isEmpty());
    BOOST_CHECK_EQUAL(buf5.size(),hmacAlg->keySize());
    BOOST_CHECK(buf3!=buf5);
    BOOST_CHECK(buf4!=buf5);

    // check reproducibility

    auto kdf3=plugin->createHKDF(hmacAlg,digestAlg);
    HATN_REQUIRE(!kdf3.isNull());
    ec=kdf3->init(masterKey2.get(),descr.saltBin());
    BOOST_CHECK(!ec);
    common::SharedPtr<SymmetricKey> derivedKey3;
    ec=kdf3->derive(derivedKey3,descr.infoBin());
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!derivedKey3.isNull());
    BOOST_CHECK(!derivedKey3->isNull());
    MemoryLockedArray buf_3;
    ec=derivedKey3->exportToBuf(buf_3,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!buf_3.isEmpty());
    BOOST_CHECK_EQUAL(buf_3.size(),hmacAlg->keySize());
    BOOST_CHECK(buf_3==buf3);

    ec=kdf3->derive(derivedKey3,ByteArray("hello info"));
    MemoryLockedArray buf3_1;
    ec=derivedKey3->exportToBuf(buf3_1,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!buf3_1.isEmpty());
    BOOST_CHECK_EQUAL(buf3_1.size(),hmacAlg->keySize());
    BOOST_CHECK(buf3_1==buf2_1);

    ec=kdf3->derive(derivedKey3,ByteArray("info"));
    MemoryLockedArray buf3_2;
    ec=derivedKey3->exportToBuf(buf3_2,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!buf3_2.isEmpty());
    BOOST_CHECK_EQUAL(buf3_2.size(),hmacAlg->keySize());
    BOOST_CHECK(buf3_2==buf4);

    ec=kdf3->derive(derivedKey3,ByteArray("hello info"));
    MemoryLockedArray buf3_3;
    ec=derivedKey3->exportToBuf(buf3_3,ContainerFormat::RAW_PLAIN,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!buf3_3.isEmpty());
    BOOST_CHECK_EQUAL(buf3_3.size(),hmacAlg->keySize());
    BOOST_CHECK(buf3_3!=buf5);
}

BOOST_AUTO_TEST_CASE(CheckHkdfVectors)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::HKDF))
            {
                checkHKDFVectors(plugin,PluginList::assetsPath("crypt"));
                checkHKDFVectors(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            }
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckHkdfKeys)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::HKDF))
            {
                checkHKDFKeys(plugin,PluginList::assetsPath("crypt"));
                checkHKDFKeys(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
