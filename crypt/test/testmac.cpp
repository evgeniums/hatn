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

#include <hatn_test_config.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/format.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/mac.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

//#define PRINT_CRYPT_ENGINE_ALGS
BOOST_FIXTURE_TEST_SUITE(TestMAC,CryptTestFixture)

static void testVectors(std::shared_ptr<CryptPlugin>& plugin, const CryptAlgorithm* alg, const std::string& fileName)
{
    BOOST_TEST_MESSAGE("Checking test vectors");

    size_t runCount=0;
    auto handler=[&runCount,plugin,alg](std::string& line)
    {
        BOOST_TEST_MESSAGE(fmt::format("Vector #{}",++runCount));

        std::vector<std::string> parts;
        Utils::trimSplit(parts,line,':');
        HATN_REQUIRE_EQUAL(parts.size(),3);
        const auto& keyHex=parts[0];
        const auto& textHex=parts[1];
        const auto& tagHex=parts[2];

        ByteArray keyData,text,tag;
        ContainerUtils::hexToRaw(keyHex,keyData);
        ContainerUtils::hexToRaw(textHex,text);
        ContainerUtils::hexToRaw(tagHex,tag);

        // create key
        auto key=alg->createMACKey();
        HATN_REQUIRE(key);
        auto ec=key->importFromBuf(keyData,ContainerFormat::RAW_PLAIN);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Failed to import key: {}",ec.message()));
        }
        HATN_REQUIRE(!ec);

        // create mac
        auto mac1=plugin->createMAC(key.get());
        HATN_REQUIRE(mac1);

        // simple check
        ByteArray tag1;
        ec=mac1->runFinalize(SpanBuffer{text},tag1);
        BOOST_CHECK(!ec);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Failed to runFinalize: {}",ec.message()));
        }
        BOOST_CHECK(tag1==tag);
#if 0
        std::string rTagHex1;
        ContainerUtils::rawToHex(tag1.stringView(),rTagHex1);
        BOOST_TEST_MESSAGE(rTagHex1);
#endif
        auto mac2=plugin->createMAC(key.get());
        HATN_REQUIRE(mac2);
        ec=mac2->runVerify(SpanBuffer{text},tag);
        BOOST_CHECK(!ec);
        ec=mac1->runVerify(SpanBuffer{text},tag);
        BOOST_CHECK(!ec);
        ec=mac1->runVerify(SpanBuffer{text},tag);
        BOOST_CHECK(!ec);

        // with offset
        ByteArray tag2("with offset");
        size_t offset=tag2.size();
        ec=mac2->runFinalize(SpanBuffer{text},tag2,offset);
        BOOST_CHECK(tag.isEqual(tag2.data()+offset,tag2.size()-offset));
        BOOST_CHECK(!ec);
        ec=mac2->runVerify(SpanBuffer{text},tag2.data()+offset,tag2.size()-offset);
        BOOST_CHECK(!ec);
        ec=mac2->runVerify(SpanBuffer{text},SpanBuffer{tag2,offset});
        BOOST_CHECK(!ec);

        // invalid offset
        ec=mac2->runFinalize(SpanBuffer{text,10000},tag2,offset);
        BOOST_CHECK(ec);
        ec=mac2->runVerify(SpanBuffer{text,10000},tag2.data()+offset,tag2.size()-offset);
        BOOST_CHECK(ec);
        ec=mac2->runVerify(SpanBuffer{text,10000},SpanBuffer{tag2,offset});
        BOOST_CHECK(ec);
        ec=mac2->runVerify(SpanBuffer{text},SpanBuffer{tag2,10000});
        BOOST_CHECK(ec);

        // test vectors
        SpanBuffers buffers;
        if (!text.isEmpty())
        {
            buffers=CryptPluginTest::split(text,3);
        }
        ByteArray tag3;
        ec=mac2->runFinalize(buffers,tag3);
        BOOST_CHECK(tag==tag3);
        BOOST_CHECK(!ec);
        ec=mac2->runVerify(buffers,tag);
        BOOST_CHECK(!ec);

        // invalid tag
        if (tag1.size()>2)
        {
            tag1[tag1.size()-1]=static_cast<char>(0xaa);
            tag1[tag1.size()-2]= static_cast<char>(0x55);
            ec=mac2->runVerify(SpanBuffer{text},tag1);
            BOOST_CHECK(ec);
        }

        // spoil data
        if (text.size()>8)
        {
            if (tagHex!="00000000000000000000000000000000") // exclude special case of poly1305 with zero key
            {
                auto text1=text;
                text1[text1.size()/2]=static_cast<char>(0x19);
                text1[text1.size()/2+1]=static_cast<char>(0xc9);
                text1[text1.size()/2+2]=static_cast<char>(0x71);
                text1[text1.size()/2+2]=static_cast<char>(0x8e);
                ec=mac2->runVerify(SpanBuffer{text1},tag);
                BOOST_CHECK(ec);
            }
        }

        // spoil key
        auto keyData2=keyData;
        keyData2[keyData.size()/2]=static_cast<char>(0xaa);
        keyData2[keyData.size()/2+1]=static_cast<char>(0x55);
        auto key2=alg->createMACKey();
        HATN_REQUIRE(key2);
        ec=key2->importFromBuf(keyData2,ContainerFormat::RAW_PLAIN);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Failed to import key: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
        mac2->setKey(key2.get());
        ec=mac2->runVerify(SpanBuffer{text},tag);
        BOOST_CHECK(ec);

        // no key
        auto mac3=plugin->createMAC();
        HATN_REQUIRE(mac3);
        ByteArray tag4;
        ec=mac3->runFinalize(SpanBuffer{text},tag4);
        BOOST_CHECK(ec);
        ec=mac3->runVerify(SpanBuffer{text},tag);
        BOOST_CHECK(ec);

        // no alg
        auto mac4=plugin->createMAC(key.get());
        mac4->setAlgorithm(nullptr);
        HATN_REQUIRE(mac4);
        ByteArray tag5;
        ec=mac4->runFinalize(SpanBuffer{text},tag5);
        BOOST_CHECK(ec);
        ec=mac4->runVerify(SpanBuffer{text},tag);
        BOOST_CHECK(ec);
    };
    PluginList::eachLinefromFile(fileName,handler);
    BOOST_CHECK_GT(runCount,0u);
    BOOST_TEST_MESSAGE(fmt::format("Tested {} vectors",runCount).c_str());
}

static void checkMac(std::shared_ptr<CryptPlugin>& plugin, const std::string& algName, const std::string& fileName)
{
    if (!boost::filesystem::exists(fileName))
    {
        return;
    }

    std::string msg=fmt::format("Begin checking MAC {} with {}",algName,fileName);
    BOOST_TEST_MESSAGE(msg);

    // check alg
    const CryptAlgorithm* alg=nullptr;
    auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::MAC,algName);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    HATN_REQUIRE(!ec);
    auto keySize=alg->keySize();
    BOOST_CHECK_GT(keySize,static_cast<size_t>(0));

    // check key creation
    auto key=alg->createMACKey();
    HATN_REQUIRE(key);
    ec=key->generate();
    HATN_REQUIRE(!ec);

    // check MAC creation
    auto mac=plugin->createMAC(key.get());
    HATN_REQUIRE(mac);

    testVectors(plugin,alg,fileName);

    msg=fmt::format("End checking MAC {} with {}",algName,fileName);
    BOOST_TEST_MESSAGE(msg);
}

BOOST_AUTO_TEST_CASE(CheckMac)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::MAC))
            {
                const CryptAlgorithm* alg=nullptr;
                auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::MAC,"_dummy_");
                BOOST_CHECK(ec);

                auto ciphers=plugin->listMACs();
                HATN_REQUIRE(!ciphers.empty());
                for (auto&& it:ciphers)
                {
#ifdef PRINT_CRYPT_ENGINE_ALGS
                    BOOST_TEST_MESSAGE(it);
#endif
                    std::string fileName=it;
                    boost::algorithm::replace_all(fileName,std::string("/"),std::string("-"));

                    std::string path=fmt::format("{}/mac-{}.txt",PluginList::assetsPath("crypt"),fileName);
                    checkMac(plugin,it,path);
                    path=fmt::format("{}/mac-{}.txt",PluginList::assetsPath("crypt",plugin->info()->name),fileName);
                    checkMac(plugin,it,path);
                }
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
