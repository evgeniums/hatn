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
#include <hatn/common/makeshared.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/ecdh.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

BOOST_FIXTURE_TEST_SUITE(TestECDH,CryptTestFixture)

static void checkEcdh(std::shared_ptr<CryptPlugin>& plugin, const std::string& curveName, const std::string& fileName)
{
    if (!boost::filesystem::exists(fileName+".check"))
    {
        return;
    }

    std::string msg=fmt::format("Begin checking curve {} with {}",curveName,fileName);
    BOOST_TEST_MESSAGE(msg);

    // check alg
    const CryptAlgorithm* alg=nullptr;
    auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::ECDH,curveName);
    HATN_REQUIRE(!ec);

    // generate first pair
    auto ecdh1=plugin->createECDH(alg);
    HATN_REQUIRE(ecdh1);
    SharedPtr<PublicKey> pubKey1;
    ec=ecdh1->generateKey(pubKey1);
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!pubKey1.isNull());
    BOOST_CHECK(!pubKey1->isNull());

    // generate second pair
    auto ecdh2=plugin->createECDH(alg);
    HATN_REQUIRE(ecdh2);
    SharedPtr<PublicKey> pubKey2;
    ec=ecdh2->generateKey(pubKey2);
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!pubKey2.isNull());
    BOOST_CHECK(!pubKey2->isNull());

    // compute shared secret
    SharedPtr<DHSecret> genSecret1;
    SharedPtr<DHSecret> genSecret2;
    ec=ecdh1->computeSecret(pubKey2,genSecret1);
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!genSecret1.isNull());
    BOOST_CHECK(!genSecret1->isNull());
    ec=ecdh2->computeSecret(pubKey1,genSecret2);
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!genSecret2.isNull());
    BOOST_CHECK(genSecret1->content()==genSecret2->content());

    // export state
    SharedPtr<PrivateKey> privKey1_2;
    SharedPtr<PublicKey> pubKeyExp1_2;
    ec=ecdh1->exportState(privKey1_2,pubKeyExp1_2);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!pubKeyExp1_2.isNull());
    BOOST_CHECK(!privKey1_2.isNull());
    ByteArray b1_1,b1_2;
    ec=pubKey1->exportToBuf(b1_1);
    BOOST_CHECK(!ec);
    ec=pubKeyExp1_2->exportToBuf(b1_2);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!b1_1.isEmpty());
    BOOST_CHECK_EQUAL(b1_1.c_str(),b1_2.c_str());
    MemoryLockedArray b1_3;
    ec=privKey1_2->exportToBuf(b1_3,ContainerFormat::PEM,true);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!b1_3.isEmpty());

    // import state
    auto ecdh3=plugin->createECDH(alg);
    HATN_REQUIRE(ecdh3);
    ec=ecdh3->importState(privKey1_2);
    BOOST_CHECK(!ec);
    SharedPtr<DHSecret> genSecret3;
    ec=ecdh3->computeSecret(pubKey2,genSecret3);
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!genSecret3.isNull());
    BOOST_CHECK(!genSecret3->isNull());
    BOOST_CHECK(genSecret1->content()==genSecret3->content());

    // import state using new key's obects with imported data
    auto ecdh4=plugin->createECDH(alg);
    HATN_REQUIRE(ecdh4);
    auto privKey4=alg->createPrivateKey();
    HATN_REQUIRE(!privKey4.isNull());
    ec=privKey4->importFromBuf(b1_3,ContainerFormat::PEM);
    BOOST_CHECK(!ec);
    ec=ecdh4->importState(privKey4);
    BOOST_CHECK(!ec);
    ByteArray pb2;
    ec=pubKey2->exportToBuf(pb2);
    BOOST_CHECK(!ec);
    auto pubKey4=plugin->createPublicKey(alg);
    HATN_REQUIRE(!pubKey4.isNull());
    ec=pubKey4->importFromBuf(pb2);
    BOOST_CHECK(!ec);
    SharedPtr<DHSecret> genSecret4;
    ec=ecdh4->computeSecret(pubKey4,genSecret4);
    BOOST_CHECK(!ec);
    HATN_REQUIRE(!genSecret4.isNull());
    BOOST_CHECK(!genSecret4->isNull());
    BOOST_CHECK(genSecret1->content()==genSecret4->content());

    // check errors
    SharedPtr<PrivateKey> eprivKey1=makeShared<PrivateKey>();
    ec=ecdh4->importState(eprivKey1);
    BOOST_CHECK(ec);
    SharedPtr<PublicKey> epubKey2=makeShared<PublicKey>();
    ec=ecdh4->computeSecret(epubKey2,genSecret4);
    BOOST_CHECK(ec);
    SharedPtr<PublicKey> epubKey3;
    ec=ecdh4->computeSecret(epubKey3,genSecret4);
    BOOST_CHECK(ec);
    ec=ecdh4->generateKey(epubKey2);
    BOOST_CHECK(ec);
    const CryptAlgorithm* ealg=nullptr;
    ec=plugin->findAlgorithm(ealg,CryptAlgorithm::Type::ECDH,"unknown_curve");
    BOOST_CHECK(ec);

    msg=fmt::format("End checking curve {} with {}",curveName,fileName);
    BOOST_TEST_MESSAGE(msg);
}

BOOST_AUTO_TEST_CASE(CheckECDH)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::ECDH))
            {
                auto curves=plugin->listEllipticCurves();
                HATN_REQUIRE(!curves.empty());
                for (auto&& it:curves)
                {
#ifdef PRINT_CRYPT_ENGINE_ALGS
                    BOOST_TEST_MESSAGE(it);
#endif
                    std::string path=fmt::format("{}/ec-{}",PluginList::assetsPath("crypt"),it);
                    checkEcdh(plugin,it,path);
                    path=fmt::format("{}/ec-{}",PluginList::assetsPath("crypt",plugin->info()->name),it);
                    checkEcdh(plugin,it,path);
                }
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
