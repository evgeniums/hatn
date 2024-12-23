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

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/asymmetricworker.h>
#include <hatn/crypt/symmetricworker.ipp>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

// #define PRINT_CRYPT_ENGINE_ALGS

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

namespace {

void algHandler(std::shared_ptr<CryptPlugin>& plugin,
                const std::string& fileNamePrefix,
                const std::string& asymAlgName)
{
    BOOST_TEST_MESSAGE(fmt::format("Asymmetric encrypt/decrypt with {}",asymAlgName));

    std::string privateKeyPath1=fmt::format("{}/{}-private-key1.pem",PluginList::assetsPath("crypt"),fileNamePrefix);
    std::string privateKeyPath2=fmt::format("{}/{}-private-key2.pem",PluginList::assetsPath("crypt"),fileNamePrefix);
    std::string plaintextPath=fmt::format("{}/{}-plaintext1.dat",PluginList::assetsPath("crypt"),fileNamePrefix);

    if (!boost::filesystem::exists(privateKeyPath1)
        ||
        !boost::filesystem::exists(privateKeyPath2)
        ||
        !boost::filesystem::exists(plaintextPath)
       )
    {
        privateKeyPath1=fmt::format("{}/{}-private-key2.pem",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix);
        privateKeyPath2=fmt::format("{}/{}-private-key1.pem",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix);
        plaintextPath=fmt::format("{}/{}-plaintext1.dat",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix);
    }

    if (!boost::filesystem::exists(privateKeyPath1)
        ||
        !boost::filesystem::exists(privateKeyPath2)
        ||
        !boost::filesystem::exists(plaintextPath)
        )
    {
        return;
    }

    // find algorithms
    const CryptAlgorithm* alg=nullptr;
    auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::AENCRYPTION,asymAlgName);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(alg);
    const CryptAlgorithm* symAlg=nullptr;
    ec=plugin->findAlgorithm(symAlg,CryptAlgorithm::Type::SENCRYPTION,"aes-256-cbc");
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(symAlg);

    // load private key
    auto privateKey1=alg->createPrivateKey();
    BOOST_REQUIRE(privateKey1);
    ByteArray privateKeyData1;
    ec=privateKeyData1.loadFromFile(privateKeyPath1);
    BOOST_REQUIRE(!ec);
    ec=privateKey1->importFromBuf(privateKeyData1,ContainerFormat::PEM);
    BOOST_REQUIRE(!ec);

    // derive public key 1 from private key 1
    auto publicKey1=plugin->createPublicKey();
    BOOST_REQUIRE(publicKey1);
    ec=publicKey1->derive(*privateKey1);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Failed to derive public key {}",ec.message()));
    }
    BOOST_REQUIRE(!ec);

    // encrypt message
    BOOST_TEST_MESSAGE(fmt::format("Asymmetric encrypt with {}",asymAlgName));
    ByteArray plaintext1;
    ec=plaintext1.loadFromFile(plaintextPath);
    BOOST_REQUIRE(!ec);
    auto enc=plugin->createAEncryptor();
    BOOST_REQUIRE(enc);
    enc->setAlg(symAlg);
    ByteArray encKey1;
    ByteArray ciphertext1;
    ec=enc->runPack(publicKey1,encKey1,plaintext1,ciphertext1);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!encKey1.empty());
    BOOST_REQUIRE(!ciphertext1.empty());

    // decrypt message
    BOOST_TEST_MESSAGE(fmt::format("Asymmetric decrypt with {}",asymAlgName));
    ByteArray plaintext2;
    auto dec=plugin->createADecryptor(privateKey1.get());
    BOOST_REQUIRE(dec);
    dec->setAlg(symAlg);
    ec=dec->runPack(encKey1,ciphertext1,plaintext2);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(plaintext1==plaintext2);

    // try to decrypt with wrong private key

    // load private key 2
    auto privateKey2=alg->createPrivateKey();
    BOOST_REQUIRE(privateKey2);
    ByteArray privateKeyData2;
    ec=privateKeyData2.loadFromFile(privateKeyPath2);
    BOOST_REQUIRE(!ec);
    ec=privateKey2->importFromBuf(privateKeyData2,ContainerFormat::PEM);
    BOOST_REQUIRE(!ec);

    BOOST_TEST_MESSAGE(fmt::format("Asymmetric decrypt with wrong key with {}",asymAlgName));
    ByteArray plaintext3;
    auto dec2=plugin->createADecryptor(privateKey2.get());
    BOOST_REQUIRE(dec);
    dec2->setAlg(symAlg);
    ec=dec2->runPack(encKey1,ciphertext1,plaintext3);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Expected decryption error: {}",ec.message()));
    }
    BOOST_CHECK(ec);

    // derive public key 2 from private key 2
    auto publicKey2=plugin->createPublicKey();
    BOOST_REQUIRE(publicKey2);
    ec=publicKey2->derive(*privateKey2);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Failed to derive public key {}",ec.message()));
    }
    BOOST_REQUIRE(!ec);

    // encrypt with multiple keys

    BOOST_TEST_MESSAGE(fmt::format("Asymmetric encrypt with multiple keys with {}",asymAlgName));
    std::vector<common::SharedPtr<crypt::PublicKey>> pubkeys{publicKey1,publicKey2};
    std::vector<common::ByteArray> encKeys2;
    ByteArray ciphertext2;
    ec=enc->runPack(pubkeys,encKeys2,plaintext1,ciphertext2);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(encKeys2.size(),2);
    BOOST_REQUIRE(!ciphertext2.empty());

    // decrypt message
    BOOST_TEST_MESSAGE(fmt::format("Asymmetric decrypt with pkey1 {}",asymAlgName));
    ByteArray plaintext4;
    ec=dec->runPack(encKeys2[0],ciphertext2,plaintext4);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(plaintext1==plaintext4);

    BOOST_TEST_MESSAGE(fmt::format("Asymmetric decrypt with pkey2 {}",asymAlgName));
    ByteArray plaintext5;
    ec=dec2->runPack(encKeys2[1],ciphertext2,plaintext5);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(plaintext1==plaintext4);
};

}

//#define PRINT_CRYPT_ENGINE_ALGS
BOOST_FIXTURE_TEST_SUITE(TestAsymmetric,CryptTestFixture)

static void checkAlg(
        const std::string& algType,
        const std::function<void (std::shared_ptr<CryptPlugin>&,const std::string&)>& handler
    )
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [algType,handler](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::AsymmetricEncryption))
            {
                const CryptAlgorithm* alg=nullptr;
                auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::AENCRYPTION,"_dummy_");
                BOOST_CHECK(ec);
                auto algs=plugin->listAsymmetricCiphers();
                HATN_REQUIRE(!algs.empty());                
                for (auto&& it:algs)
                {
                    const auto& algName=it;
#ifdef PRINT_CRYPT_ENGINE_ALGS
                    BOOST_TEST_MESSAGE(algName);
#endif
                    if (boost::algorithm::istarts_with(algName,algType))
                    {
                        handler(plugin,algName);
                    }
                }
            }
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckRSA)
{
    auto handler=[](std::shared_ptr<CryptPlugin>& plugin,const std::string&)
    {
        algHandler(plugin,"rsa3072","RSA/3072");
    };
    checkAlg("RSA",handler);
}

#if 0
BOOST_AUTO_TEST_CASE(CheckEC)
{
    //! @todo Implement ECIES
    auto handler=[](std::shared_ptr<CryptPlugin>& plugin,const std::string&)
    {
        const CryptAlgorithm* alg=nullptr;
        auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::AENCRYPTION,"EC/prime256v1");
        if (!ec)
        {
            algHandler(plugin,"ecprime256v1","EC/prime256v1");
        }
    };
    checkAlg("EC",handler);
}
#endif
BOOST_AUTO_TEST_SUITE_END()

}
}
