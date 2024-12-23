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

    std::string privateKeyPath=fmt::format("{}/{}-private-key.pem",PluginList::assetsPath("crypt"),fileNamePrefix);
    std::string publicKeyPath=fmt::format("{}/{}-public-key.pem",PluginList::assetsPath("crypt"),fileNamePrefix);
    std::string plaintextPath=fmt::format("{}/{}-plaintext1.dat",PluginList::assetsPath("crypt"),fileNamePrefix);

    if (!boost::filesystem::exists(privateKeyPath)
        ||
        !boost::filesystem::exists(publicKeyPath)
        ||
        !boost::filesystem::exists(plaintextPath)
       )
    {
        privateKeyPath=fmt::format("{}/{}-private-key.pem",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix);
        publicKeyPath=fmt::format("{}/{}-public-key.pem",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix);
        plaintextPath=fmt::format("{}/{}-plaintext1.dat",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix);
    }

    if (!boost::filesystem::exists(privateKeyPath)
        ||
        !boost::filesystem::exists(publicKeyPath)
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

    // load public key
    auto publicKey1=plugin->createPublicKey();
    BOOST_REQUIRE(publicKey1);
    ByteArray publicKeyData1;
    ec=publicKeyData1.loadFromFile(publicKeyPath);
    BOOST_REQUIRE(!ec);
    ec=publicKey1->importFromBuf(publicKeyData1,ContainerFormat::PEM);
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

    // load private key
    auto privateKey1=alg->createPrivateKey();
    BOOST_REQUIRE(privateKey1);
    ByteArray privateKeyData;
    ec=privateKeyData.loadFromFile(privateKeyPath);
    BOOST_REQUIRE(!ec);
    ec=privateKey1->importFromBuf(privateKeyData,ContainerFormat::PEM);
    BOOST_REQUIRE(!ec);

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

    //! @todo Try to decrypt with wrong key
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

BOOST_AUTO_TEST_SUITE_END()

}
}
