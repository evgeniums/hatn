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

#define PRINT_CRYPT_ENGINE_ALGS

BOOST_FIXTURE_TEST_SUITE(TestPbkdf,CryptTestFixture)

void checkPBKDF(std::shared_ptr<CryptPlugin>& plugin, const std::string& fileName)
{
    BOOST_TEST_MESSAGE("Checking test vectors");

    if (boost::filesystem::exists(fileName))
    {
        size_t runCount=0;
        auto handler=[&runCount,plugin](std::string& line)
        {
            BOOST_TEST_MESSAGE(fmt::format("Vector #{}",++runCount));
#ifdef MEASURE_DECRYPT_PERFORMANCE
            ElapsedTimer timer;
            size_t measureCount=1000000;
#endif
            std::vector<std::string> parts;
            Utils::trimSplit(parts,line,':');
            HATN_REQUIRE_EQUAL(parts.size(),6);

            auto kdfName=parts[0];
            auto cipherName=parts[1];
            auto password=parts[2];
            auto salt=parts[3];
            auto result=parts[4];
            bool saltRequired=static_cast<bool>(std::stoi(parts[5]));

            const CryptAlgorithm* cipherAlg=nullptr;

            BOOST_TEST_MESSAGE(fmt::format("Cipher alg {}",cipherName));

            auto ec=plugin->findAlgorithm(cipherAlg,CryptAlgorithm::Type::SENCRYPTION,cipherName);
            if (ec)
            {
                BOOST_TEST_MESSAGE(fmt::format("Cipher alg {} is not supported by plugin {}",cipherName,plugin->info()->name));
                return;
            }

            const CryptAlgorithm* kdfAlg=nullptr;
            BOOST_TEST_MESSAGE(fmt::format("KDF alg {}",kdfName));

            ec=plugin->findAlgorithm(kdfAlg,CryptAlgorithm::Type::PBKDF,kdfName);
            if (ec)
            {
                BOOST_TEST_MESSAGE(fmt::format("KDF alg {} is not supported by plugin {}",kdfName,plugin->info()->name));
                return;
            }

            auto pbkdf=plugin->createPBKDF(cipherAlg,kdfAlg);
            HATN_REQUIRE(!pbkdf.isNull());

            // basic check
            common::SharedPtr<SymmetricKey> derivedKey1;
            ec=pbkdf->derive(password.c_str(),derivedKey1,salt);
            BOOST_CHECK(!ec);
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            HATN_REQUIRE(!derivedKey1.isNull());
            BOOST_CHECK_EQUAL(derivedKey1->content().size(),cipherAlg->keySize());

            std::string hex1;
            common::ContainerUtils::rawToHex(derivedKey1->content().stringView(),hex1,true);
    #if 0
            BOOST_TEST_MESSAGE(fmt::format("Derived key: {}",hex1));
            BOOST_TEST_MESSAGE(fmt::format("Compare key with sample (starts_with): {}",result));
    #endif
            size_t checkSize=(std::min)(result.size(),hex1.size());
            BOOST_CHECK_GT(checkSize,0u);
            std::string checkSample=result.substr(0,checkSize);
            std::string checkDerived=hex1.substr(0,checkSize);
            BOOST_CHECK_EQUAL(checkSample,checkDerived);

            // check different interfaces
            common::SharedPtr<SymmetricKey> derivedKey2;
            ec=pbkdf->derive(password.c_str(),derivedKey2);
            if (!saltRequired)
            {
                BOOST_CHECK(!ec);
                if (ec)
                {
                    BOOST_TEST_MESSAGE(ec.message());
                }
                HATN_REQUIRE(!derivedKey2.isNull());
                BOOST_CHECK_EQUAL(derivedKey2->content().size(),cipherAlg->keySize());
            }
            else
            {
                BOOST_CHECK(ec);
            }
            common::SharedPtr<SymmetricKey> derivedKey4;
            ec=pbkdf->derive(password.data(),password.size(),derivedKey4);
            if (!saltRequired)
            {
                BOOST_CHECK(!ec);
                if (ec)
                {
                    BOOST_TEST_MESSAGE(ec.message());
                }
                HATN_REQUIRE(!derivedKey4.isNull());
                BOOST_CHECK_EQUAL(derivedKey4->content().size(),cipherAlg->keySize());
                BOOST_CHECK(derivedKey2->content()==derivedKey4->content());
            }
            else
            {
                BOOST_CHECK(ec);
            }

            common::SharedPtr<SymmetricKey> derivedKey3;
            ec=pbkdf->derive(password.data(),password.size(),derivedKey3,salt);
            BOOST_CHECK(!ec);
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            HATN_REQUIRE(!derivedKey3.isNull());
            BOOST_CHECK(derivedKey1->content()==derivedKey3->content());

            // check passphrasekey
            auto passPhraseKey1=plugin->createPassphraseKey(cipherAlg,kdfAlg);
            HATN_REQUIRE(!passPhraseKey1.isNull());
            ec=passPhraseKey1->importFromBuf(password);
            BOOST_CHECK(!ec);
            ec=pbkdf->derive(passPhraseKey1.get(),salt);
            BOOST_CHECK(!ec);
            BOOST_REQUIRE(passPhraseKey1->derivedKey()!=nullptr);
            BOOST_CHECK(derivedKey1->content()==passPhraseKey1->derivedKey()->content());

            auto passPhraseKey2=plugin->createPassphraseKey(cipherAlg,kdfAlg);
            HATN_REQUIRE(!passPhraseKey2.isNull());
            ec=passPhraseKey2->importFromBuf(password);
            BOOST_CHECK(!ec);
            passPhraseKey2->setSalt(salt);
            BOOST_REQUIRE(passPhraseKey2->derivedKey()!=nullptr);
            BOOST_CHECK(derivedKey1->content()==passPhraseKey2->derivedKey()->content());

            // check arbitrary key
            auto masterKey=std::make_shared<SecureKey>(password);
            masterKey->setFormat(ContainerFormat::RAW_PLAIN);
            common::SharedPtr<SymmetricKey> derivedKey5;
            ec=pbkdf->derive(masterKey.get(),derivedKey5,salt);
            BOOST_CHECK(!ec);
            BOOST_REQUIRE(!derivedKey5.isNull());
            BOOST_CHECK(derivedKey1->content()==derivedKey5->content());
        };

        PluginList::eachLinefromFile(fileName,handler);
        BOOST_CHECK_GT(runCount,0u);
        BOOST_TEST_MESSAGE(fmt::format("Tested {} vectors",runCount).c_str());
    }
}

BOOST_AUTO_TEST_CASE(CheckPbkdf)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::PBKDF))
            {
                BOOST_TEST_MESSAGE("PBKDF algorithms sypported by plugin:");

#ifdef PRINT_CRYPT_ENGINE_ALGS
                auto algs=plugin->listPBKDFs();
                for (auto&& it:algs)
                {
                    BOOST_TEST_MESSAGE(it);
                }
#endif
                std::string path=fmt::format("{}/pbkdf-vectors.txt",PluginList::assetsPath("crypt"));
                checkPBKDF(plugin,path);
                path=fmt::format("{}/pbkdf-vectors.txt",PluginList::assetsPath("crypt",plugin->info()->name));
                checkPBKDF(plugin,path);
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
