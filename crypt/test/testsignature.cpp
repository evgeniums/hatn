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
#include <hatn/crypt/signature.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

namespace {

void algHandler(std::shared_ptr<CryptPlugin>& plugin,const std::string& algNameFromList,
                const std::string& fileNamePrefix,
                const std::string& sigAlgName, const std::string& digestAlgName)
{
    std::ignore=algNameFromList;

    std::string privateKeyPath=fmt::format("{}/{}-private-key.pem",PluginList::assetsPath("crypt"),fileNamePrefix);
    std::string publicKeyPath=fmt::format("{}/{}-public-key.pem",PluginList::assetsPath("crypt"),fileNamePrefix);
    std::string sigHexPath=fmt::format("{}/{}-{}-sig-hex.dat",PluginList::assetsPath("crypt"),fileNamePrefix,digestAlgName);
    std::string msgHexPath=fmt::format("{}/{}-sig-message-hex.txt",PluginList::assetsPath("crypt"),fileNamePrefix);

    if (!boost::filesystem::exists(privateKeyPath)
        ||
        !boost::filesystem::exists(publicKeyPath)
        ||
        !boost::filesystem::exists(sigHexPath)
        ||
        !boost::filesystem::exists(msgHexPath)
       )
    {
        privateKeyPath=fmt::format("{}/{}-private-key.pem",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix);
        publicKeyPath=fmt::format("{}/{}-public-key.pem",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix);
        sigHexPath=fmt::format("{}/{}-{}-sig-hex.dat",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix,digestAlgName);
        msgHexPath=fmt::format("{}/{}-sig-message-hex.txt",PluginList::assetsPath("crypt",plugin->info()->name),fileNamePrefix);
    }

    if (!boost::filesystem::exists(privateKeyPath)
        ||
        !boost::filesystem::exists(publicKeyPath)
        ||
        !boost::filesystem::exists(sigHexPath)
        ||
        !boost::filesystem::exists(msgHexPath)
       )
    {
        return;
    }

    if (digestAlgName.empty())
    {
        BOOST_TEST_MESSAGE(fmt::format("Testing {}, no digest",sigAlgName));
    }
    else
    {
        BOOST_TEST_MESSAGE(fmt::format("Testing {}, digest {}",sigAlgName,digestAlgName));
    }

    ByteArray msgData;
    ByteArray msgDataHex;
    auto ec=msgDataHex.loadFromFile(msgHexPath);
    BOOST_CHECK(!ec);
    ContainerUtils::hexToRaw(msgDataHex.stringView(),msgData);

    ByteArray sigSample;
    ByteArray sigHex;
    ec=sigHex.loadFromFile(sigHexPath);
    BOOST_CHECK(!ec);
    ContainerUtils::hexToRaw(sigHex.stringView(),sigSample);

    // find algorithms
    const CryptAlgorithm* alg=nullptr;
    ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::SIGNATURE,sigAlgName);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(alg);
    const CryptAlgorithm* digestAlg=nullptr;
    if (!digestAlgName.empty())
    {
        ec=plugin->findAlgorithm(digestAlg,CryptAlgorithm::Type::DIGEST,digestAlgName);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE(digestAlg);
    }

    // load private key
    auto privateKey1=alg->createPrivateKey();
    BOOST_REQUIRE(privateKey1);
    ByteArray privateKeyData;
    ec=privateKeyData.loadFromFile(privateKeyPath);
    BOOST_REQUIRE(!ec);
    ec=privateKey1->importFromBuf(privateKeyData,ContainerFormat::PEM);
    BOOST_REQUIRE(!ec);

    // sign message
    auto signProcessor=alg->createSignatureSign();
    BOOST_REQUIRE(signProcessor);
    signProcessor->setKey(privateKey1.get());
    signProcessor->setDigestAlg(digestAlg);
    ByteArray sig1;
    sig1.resize(100);
    sig1.fill(0);
    sig1.clear();
    ec=signProcessor->sign(SpanBuffer(msgData),sig1);
    auto bec=!ec.isNull();
    if (bec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_CHECK(!bec);

    // load public key
    auto publicKey1=plugin->createPublicKey();
    BOOST_REQUIRE(publicKey1);
    ByteArray publicKeyData1;
    ec=publicKeyData1.loadFromFile(publicKeyPath);
    BOOST_REQUIRE(!ec);
    ec=publicKey1->importFromBuf(publicKeyData1,ContainerFormat::PEM);
    BOOST_REQUIRE(!ec);

    // verify message
    auto verifyProcessor=alg->createSignatureVerify();
    BOOST_REQUIRE(verifyProcessor);
    verifyProcessor->setKey(publicKey1.get());
    verifyProcessor->setDigestAlg(digestAlg);
    ec=verifyProcessor->verify(SpanBuffer(msgData),sig1);
    BOOST_CHECK(!ec);
    ec=verifyProcessor->verify(SpanBuffer(msgData),sigSample);
    BOOST_CHECK(!ec);

    // sign and verify again
    ByteArray sig2;
    ec=signProcessor->sign(SpanBuffer(msgData),sig2);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_CHECK(!ec);
    ec=verifyProcessor->verify(SpanBuffer(msgData),sig2);
    BOOST_CHECK(!ec);

    // verify with mailformed signature
    if (sig2.size()>4)
    {
        sig2[sig2.size()/2]=static_cast<char>(0xAA);
        sig2[sig2.size()/2-1]=static_cast<char>(0xBB);
    }
    ec=verifyProcessor->verify(SpanBuffer(msgData),sig2);
    BOOST_CHECK(ec);

    // verify mailformed message
    ByteArray msgData2=msgData;
    if (msgData2.size()>4)
    {
        msgData2[msgData2.size()/2]=static_cast<char>('A');
        msgData2[msgData2.size()/2-1]=static_cast<char>('b');
    }
    ec=verifyProcessor->verify(SpanBuffer(msgData2),sigSample);
    BOOST_CHECK(ec);

    // derive public key and compare with sample public key
    auto publicKey2=plugin->createPublicKey();
    BOOST_REQUIRE(publicKey2);
    ec=publicKey2->derive(*privateKey1);
    BOOST_CHECK(!ec);
    ByteArray publicKeyData1_1;
    ec=publicKey1->exportToBuf(publicKeyData1_1,ContainerFormat::DER);
    BOOST_CHECK(!ec);
    ByteArray publicKeyData2;
    ec=publicKey2->exportToBuf(publicKeyData2,ContainerFormat::DER);
    BOOST_CHECK(!ec);
    BOOST_CHECK(publicKeyData1_1==publicKeyData2);

    // sign and verify with generated key
    auto privateKey2=alg->createPrivateKey();
    BOOST_REQUIRE(privateKey2);
    ec=privateKey2->generate();
    BOOST_REQUIRE(!ec);
    signProcessor->setKey(privateKey2.get());
    ByteArray sig3;
    ec=signProcessor->sign(SpanBuffer(msgData),sig3);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_CHECK(!ec);
    auto publicKey3=plugin->createPublicKey();
    BOOST_REQUIRE(publicKey3);
    ec=publicKey3->derive(*privateKey2);
    BOOST_CHECK(!ec);
    verifyProcessor->setKey(publicKey3.get());
    ec=verifyProcessor->verify(SpanBuffer(msgData),sig3);
    BOOST_CHECK(!ec);
    ec=verifyProcessor->verify(SpanBuffer(msgData),sigSample);
    BOOST_CHECK(ec);

    // check public key algorithm
    const CryptAlgorithm* pubkeyAlg=nullptr;
    ec=plugin->publicKeyAlgorithm(pubkeyAlg,publicKey1.get());
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Failed to get algorithm for pubkey: {}",ec.message()));
    }
    HATN_REQUIRE(!ec);
    BOOST_CHECK(pubkeyAlg==alg);

    // create private key from data
    SharedPtr<PrivateKey> privateKey4;
    ec=plugin->createAsymmetricPrivateKeyFromContent(privateKey4,privateKeyData,ContainerFormat::PEM);
    HATN_REQUIRE(!ec);
    HATN_REQUIRE(!privateKey4.isNull());
    BOOST_CHECK(privateKey4->alg());
    BOOST_CHECK(privateKey4->alg()==alg);
};

}

//#define PRINT_CRYPT_ENGINE_ALGS
BOOST_FIXTURE_TEST_SUITE(TestSignature,CryptTestFixture)

static void checkAlg(
        const std::string& algType,
        const std::function<void (std::shared_ptr<CryptPlugin>&,const std::string&)>& handler
    )
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [algType,handler](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::Signature))
            {
                const CryptAlgorithm* alg=nullptr;
                auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::SIGNATURE,"_dummy_");
                BOOST_CHECK(ec);
                auto algs=plugin->listSignatures();
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

BOOST_AUTO_TEST_CASE(CheckDSA)
{
    auto handler=[](std::shared_ptr<CryptPlugin>& plugin,const std::string& algNameFromList)
    {
        algHandler(plugin,algNameFromList,"dsa2048","DSA/2048","sha256");
    };
    checkAlg("DSA",handler);
}

BOOST_AUTO_TEST_CASE(CheckRSA)
{
    auto handler=[](std::shared_ptr<CryptPlugin>& plugin,const std::string& algNameFromList)
    {
        algHandler(plugin,algNameFromList,"rsa3072","RSA/3072","sha256");
    };
    checkAlg("RSA",handler);
}

BOOST_AUTO_TEST_CASE(CheckEC)
{
    auto handler=[](std::shared_ptr<CryptPlugin>& plugin,const std::string& algNameFromList)
    {
        const CryptAlgorithm* alg=nullptr;
        auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::SIGNATURE,"EC/prime256v1");
        if (!ec)
        {
            const CryptAlgorithm* digest=nullptr;
            ec=plugin->findAlgorithm(digest,CryptAlgorithm::Type::DIGEST,"sha512");
            if (!ec)
            {
                algHandler(plugin,algNameFromList,"ecprime256v1","EC/prime256v1","sha512");
            }
        }
    };
    checkAlg("EC",handler);
}

BOOST_AUTO_TEST_CASE(CheckEd25519)
{
    auto handler=[](std::shared_ptr<CryptPlugin>& plugin,const std::string& algNameFromList)
    {
        const CryptAlgorithm* alg=nullptr;
        auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::SIGNATURE,"ED/25519");
        if (!ec)
        {
            algHandler(plugin,algNameFromList,"ed25519","ED/25519","");
            algHandler(plugin,algNameFromList,"ed25519","ED/25519","sha512");
        }
    };
    checkAlg("ED/25519",handler);
}

BOOST_AUTO_TEST_CASE(CheckEd448)
{
    auto handler=[](std::shared_ptr<CryptPlugin>& plugin,const std::string& algNameFromList)
    {
        const CryptAlgorithm* alg=nullptr;
        auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::SIGNATURE,"ED/448");
        if (!ec)
        {
            algHandler(plugin,algNameFromList,"ed448","ED/448","");
            algHandler(plugin,algNameFromList,"ed448","ED/448","sha256");
        }
    };
    checkAlg("ED/448",handler);
}

BOOST_AUTO_TEST_SUITE_END()

}
}
