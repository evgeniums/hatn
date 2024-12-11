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
#include <hatn/crypt/cipher.h>
#include <hatn/crypt/encrypthmac.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

BOOST_FIXTURE_TEST_SUITE(TestEncryptHmac,CryptTestFixture)

static void checkEncryptHmac(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    std::string cipherAlgFile=fmt::format("{}/encrypthmac-cipher.txt",path);
    std::string hmacAlgFile=fmt::format("{}/encrypthmac-hmac.txt",path);

    if (boost::filesystem::exists(cipherAlgFile)
        &&
        boost::filesystem::exists(hmacAlgFile)
       )
    {
        const CryptAlgorithm* cipherAlg=nullptr;
        auto cipherName=PluginList::linefromFile(cipherAlgFile);
        BOOST_TEST_MESSAGE(fmt::format("Cipher alg {}",cipherName));
        auto ec=plugin->findAlgorithm(cipherAlg,CryptAlgorithm::Type::SENCRYPTION,cipherName);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Cipher alg {} is not supported by plugin {}",cipherName,plugin->info()->name));
            return;
        }

        const CryptAlgorithm* hmacAlg=nullptr;
        auto algName=PluginList::linefromFile(hmacAlgFile);
        BOOST_TEST_MESSAGE(fmt::format("HMAC alg {}",algName));
        ec=plugin->findAlgorithm(hmacAlg,CryptAlgorithm::Type::HMAC,algName);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("HMAC alg {} is not supported by plugin {}",algName,plugin->info()->name));
            return;
        }

        auto cipherKey=cipherAlg->createSymmetricKey();
        HATN_REQUIRE(cipherKey);
        ec=cipherKey->generate();
        HATN_REQUIRE(!ec);

        auto macKey=hmacAlg->createMACKey();
        HATN_REQUIRE(macKey);
        ec=macKey->generate();
        HATN_REQUIRE(!ec);

        ByteArray plain("Hello world from Dracosha! Let's encrypt and HMAC this phrase.");
        ByteArray packed;
        ByteArray unpacked;

        auto encryptHmac=std::make_shared<EncryptHMAC>(std::move(cipherKey),std::move(macKey));
        ec=encryptHmac->pack(plain,packed);
        BOOST_CHECK(!ec);
        BOOST_CHECK(!packed.isEmpty());
        ec=encryptHmac->unpack(packed,unpacked);
        BOOST_CHECK(!ec);
        BOOST_CHECK_EQUAL(plain.size(),unpacked.size());
        BOOST_CHECK_EQUAL(plain.c_str(),unpacked.c_str());

        ByteArray stub;
        ec=plugin->randContainer(stub,1024);
        BOOST_CHECK(!ec);
        ec=encryptHmac->unpack(stub,unpacked);
        BOOST_CHECK(ec);

        ByteArray unpacked2;
        ec=encryptHmac->unpack(packed,unpacked2);
        BOOST_CHECK(!ec);
        BOOST_CHECK_EQUAL(plain.size(),unpacked2.size());
        BOOST_CHECK_EQUAL(plain.c_str(),unpacked2.c_str());

        auto cipherKey1=cipherAlg->createSymmetricKey();
        HATN_REQUIRE(cipherKey1);
        ec=cipherKey1->generate();
        HATN_REQUIRE(!ec);

        auto macKey1=hmacAlg->createMACKey();
        HATN_REQUIRE(macKey1);
        ec=macKey1->generate();
        HATN_REQUIRE(!ec);

        encryptHmac->setCipherKey(cipherKey1);

        ByteArray unpacked3;
        ec=encryptHmac->unpack(packed,unpacked3);
        BOOST_CHECK(plain!=unpacked3);

        encryptHmac->setMacKey(macKey1);

        ByteArray unpacked4;
        ec=encryptHmac->unpack(packed,unpacked4);
        BOOST_CHECK(ec);

        ByteArray packed1;
        ByteArray unpacked5;
        ec=encryptHmac->pack(plain,packed1);
        BOOST_CHECK(!ec);
        BOOST_CHECK(!packed1.isEmpty());
        ec=encryptHmac->unpack(packed1,unpacked5);
        BOOST_CHECK(!ec);
        BOOST_CHECK_EQUAL(plain.size(),unpacked5.size());
        BOOST_CHECK_EQUAL(plain.c_str(),unpacked5.c_str());
    }
}

BOOST_AUTO_TEST_CASE(CheckEncryptHmac)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::SymmetricEncryption)
                &&
                plugin->isFeatureImplemented(Crypt::Feature::HMAC)
               )
            {
                checkEncryptHmac(plugin,PluginList::assetsPath("crypt"));
                checkEncryptHmac(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
