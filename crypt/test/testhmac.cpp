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

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

BOOST_FIXTURE_TEST_SUITE(TestHmac,CryptTestFixture)

void checkHMAC(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    std::string keyFile=fmt::format("{}/hmac-key.txt",path);
    std::string hmacAlgFile=fmt::format("{}/hmac-alg.txt",path);
    std::string dataFile=fmt::format("{}/hmac-data.txt",path);
    std::string resultFile=fmt::format("{}/hmac-result.txt",path);

    if (boost::filesystem::exists(keyFile)
        &&
        boost::filesystem::exists(hmacAlgFile)
        &&
        boost::filesystem::exists(dataFile)
        &&
        boost::filesystem::exists(resultFile)
       )
    {
        const CryptAlgorithm* alg=nullptr;
        auto algName=PluginList::linefromFile(hmacAlgFile);

        BOOST_TEST_MESSAGE(fmt::format("HMAC alg {}",algName));

        auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::HMAC,algName);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("HMAC alg {} is not supported by plugin {}",algName,plugin->info()->name));
            return;
        }

        auto dataHex=PluginList::linefromFile(dataFile);
        HATN_REQUIRE(!dataHex.empty());
        ByteArray data;
        ContainerUtils::hexToRaw(dataHex,data);

        ByteArray result;

        // using HMAC key with direct data

        auto keyHex=PluginList::linefromFile(keyFile);
        HATN_REQUIRE(!keyHex.empty());
        ByteArray keyData;
        ContainerUtils::hexToRaw(keyHex,keyData);
        auto key=std::make_shared<MACKey>(keyData);
        HATN_REQUIRE(key);
        key->setFormat(ContainerFormat::RAW_PLAIN);

        auto hmac=plugin->createHMAC();
        HATN_REQUIRE(hmac);
        hmac->setKey(key.get());
        ec=hmac->init(alg);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        HATN_REQUIRE(!ec);
        ec=hmac->processAndFinalize(data,result);
        HATN_REQUIRE(!result.isEmpty());

        auto checkResultHex=PluginList::linefromFile(resultFile);
        ByteArray checkResult;
        ContainerUtils::hexToRaw(checkResultHex,checkResult);

        BOOST_CHECK(result==checkResult);

        // re-run again with the same object
        result="Bla bla blablabla";
        ec=hmac->run(data,result);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(!result.isEmpty());
        BOOST_CHECK(result==checkResult);

        // using hmac as special backend's HMAC key
        const CryptAlgorithm* alg1=nullptr;
        ec=plugin->findAlgorithm(alg1,CryptAlgorithm::Type::MAC,algName);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(alg1!=nullptr);

        auto key1=alg1->createMACKey();
        HATN_REQUIRE(key1);
        ec=key1->importFromBuf(keyData,ContainerFormat::RAW_PLAIN);
        HATN_REQUIRE(!ec);

        result.clear();
        auto hmac1=plugin->createMAC();
        HATN_REQUIRE(hmac1);
        hmac1->setKey(key1.get());
        ec=hmac1->init(alg1);
        HATN_REQUIRE(!ec);
        ec=hmac1->processAndFinalize(data,result);
        HATN_REQUIRE(!result.isEmpty());
        BOOST_CHECK(result==checkResult);

        // re-run again with the same object
        result="Bla bla blablabla";
        ec=hmac1->run(data,result);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(!result.isEmpty());
        BOOST_CHECK(result==checkResult);

        // one-shot hmac
        ec=HMAC::hmac(alg,key.get(),data,result);
        BOOST_CHECK(!ec);
        BOOST_CHECK(result==checkResult);

        // one-shot hmac check
        ec=HMAC::checkHMAC(alg,key.get(),data,checkResult);
        BOOST_CHECK(!ec);
    }
}

BOOST_AUTO_TEST_CASE(CheckHmac)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::HMAC))
            {
                checkHMAC(plugin,PluginList::assetsPath("crypt"));
                checkHMAC(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
