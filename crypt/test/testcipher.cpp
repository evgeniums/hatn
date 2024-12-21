/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

#include <hatn_test_config.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/format.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/cipher.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

#define PRINT_CRYPT_ENGINE_ALGS
BOOST_FIXTURE_TEST_SUITE(TestCipher,CryptTestFixture)

static void singleEncryptDecrypt(std::shared_ptr<CryptPlugin>& plugin, const CryptAlgorithm* alg)
{
    std::ignore=plugin;

    BOOST_TEST_MESSAGE("Checking single encrypt/decrypt");

    static ByteArray PlainTextData("Hello world from Dracosha! Let's encrypt this phrase with symmetric algorithm.");

    auto key=alg->createSymmetricKey();
    HATN_REQUIRE(key);
    auto ec=key->generate();
    HATN_REQUIRE(!ec);

    ByteArray ciphertext;
    ec=Cipher::encrypt(key.get(),PlainTextData,ciphertext);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!ciphertext.isEmpty());
    if (!alg->isNone())
    {
        BOOST_CHECK(ciphertext!=PlainTextData);
    }
    else
    {
        BOOST_CHECK(ciphertext==PlainTextData);
    }

    ByteArray plaintext;
    ec=Cipher::decrypt(key.get(),ciphertext,plaintext);
    BOOST_CHECK(!ec);
    BOOST_CHECK(!plaintext.isEmpty());
    BOOST_CHECK_EQUAL(plaintext.size(),PlainTextData.size());
    BOOST_CHECK_EQUAL(plaintext.c_str(),PlainTextData.c_str());

    ec=Cipher::encrypt(key.get(),ByteArray(),ciphertext);
    BOOST_CHECK(!ec);
    BOOST_CHECK(ciphertext.isEmpty());

    ec=Cipher::decrypt(key.get(),ByteArray(),plaintext);
    BOOST_CHECK(!ec);
    BOOST_CHECK(plaintext.isEmpty());
}

static void testVectors(std::shared_ptr<CryptPlugin>& plugin, const CryptAlgorithm* alg, const std::string& fileName)
{
    std::ignore=plugin;
    BOOST_TEST_MESSAGE("Checking test vectors");

    if (boost::filesystem::exists(fileName+"-tv.key"))
    {
        auto tvKey=PluginList::linefromFile(fileName+"-tv.key");
        if (boost::filesystem::exists(fileName+"-tv.plain"))
        {
            auto tvPlain=PluginList::linefromFile(fileName+"-tv.plain");
            if (boost::filesystem::exists(fileName+"-tv.ciphered"))
            {
                auto tvCiphered=PluginList::linefromFile(fileName+"-tv.ciphered");
                if (boost::filesystem::exists(fileName+"-tv.iv"))
                {
                    auto tvIV=PluginList::linefromFile(fileName+"-tv.iv");

                    ByteArray keyBuf;
                    boost::algorithm::unhex(tvKey.begin(),tvKey.end(),std::back_inserter(keyBuf));
                    ByteArray plaintext;
                    boost::algorithm::unhex(tvPlain.begin(),tvPlain.end(),std::back_inserter(plaintext));
                    ByteArray ciphertext;
                    boost::algorithm::unhex(tvCiphered.begin(),tvCiphered.end(),std::back_inserter(ciphertext));
                    ByteArray iv;
                    boost::algorithm::unhex(tvIV.begin(),tvIV.end(),std::back_inserter(iv));

                    auto key=alg->createSymmetricKey();
                    HATN_REQUIRE(key);
                    auto ec=key->importFromBuf(keyBuf,ContainerFormat::RAW_PLAIN);
                    HATN_REQUIRE(!ec);

                    // enable/disable padding by placing cipher-<algorithm>.pad file in assets folder
                    // e.g., in OpenSSL the padding is enabled by default
                    // but AES test vectors are without padding
                    const_cast<CryptAlgorithm*>(alg)->setEnablePadding(boost::filesystem::exists(fileName+".pad"));

                    // check single step encryption
                    ByteArray mCiphertext;
                    auto enc=key->alg()->engine()->plugin()->createSEncryptor(key.get());
                    HATN_REQUIRE(enc);
                    ec=enc->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=enc->processAndFinalize(plaintext,mCiphertext);
                    HATN_REQUIRE(!ec);
                    BOOST_CHECK(mCiphertext==ciphertext);

                    // check single step decryption
                    ByteArray mPlaintext;
                    auto dec=key->alg()->engine()->plugin()->createSDecryptor(key.get());
                    HATN_REQUIRE(dec);
                    ec=dec->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=dec->processAndFinalize(ciphertext,mPlaintext);
                    HATN_REQUIRE(!ec);
                    BOOST_CHECK(mPlaintext==plaintext);

                    // check multiple steps encryption
                    mCiphertext.clear();
                    size_t outOffset=0;
                    enc->reset();
                    ec=enc->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=enc->process(plaintext,mCiphertext,outOffset,8);
                    HATN_REQUIRE(!ec);
                    ec=enc->process(plaintext,mCiphertext,outOffset,plaintext.size()-8,8,outOffset);
                    HATN_REQUIRE(!ec);
                    ec=enc->finalize(mCiphertext,outOffset,outOffset);
                    HATN_REQUIRE(!ec);
                    BOOST_CHECK(mCiphertext==ciphertext);

                    // check multiple steps decryption
                    mPlaintext.clear();
                    outOffset=0;
                    dec->reset();
                    ec=dec->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=dec->process(ciphertext,mPlaintext,outOffset,8);
                    HATN_REQUIRE(!ec);
                    ec=dec->process(ciphertext,mPlaintext,outOffset,plaintext.size()-8,8,outOffset);
                    HATN_REQUIRE(!ec);
                    ec=dec->finalize(mPlaintext,outOffset,outOffset);
                    HATN_REQUIRE(!ec);
                    BOOST_CHECK(mPlaintext==plaintext);

                    // check boundary cases
                    enc->reset();
                    ec=enc->init(ByteArray());
                    BOOST_CHECK(!ec);
                    ec=enc->processAndFinalize(plaintext,mCiphertext);
                    BOOST_CHECK(!ec);
                    if (!alg->isNone())
                    {
                        BOOST_CHECK(ciphertext!=mCiphertext);
                    }
                    else
                    {
                        BOOST_CHECK(ciphertext==mCiphertext);
                    }
                    dec->reset();
                    ec=dec->init(ByteArray());
                    BOOST_CHECK(!ec);
                    ec=dec->processAndFinalize(mCiphertext,mPlaintext);
                    BOOST_CHECK(!ec);
                    BOOST_CHECK(plaintext==mPlaintext);

                    mCiphertext.clear();
                    ec=enc->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=enc->processAndFinalize(ByteArray(),mCiphertext);
                    BOOST_CHECK(!ec);
                    BOOST_CHECK(mCiphertext.isEmpty());

                    mCiphertext.clear();
                    ec=enc->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=enc->processAndFinalize(plaintext,mCiphertext,1000,0,100);
                    BOOST_CHECK(ec);

                    mPlaintext.clear();
                    ec=dec->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=dec->processAndFinalize(ByteArray(),mCiphertext);
                    BOOST_CHECK(!ec);
                    BOOST_CHECK(mCiphertext.isEmpty());

                    mCiphertext.clear();
                    ec=dec->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=dec->processAndFinalize(ciphertext,mCiphertext,1000,0,100);
                    BOOST_CHECK(ec);

                    // check offsets
                    ByteArray pTextOffset("some offset");
                    mPlaintext=pTextOffset+plaintext;
                    ByteArray cTextOffset("12345678");
                    mCiphertext=cTextOffset;
                    ec=enc->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=enc->processAndFinalize(mPlaintext,mCiphertext,pTextOffset.size(),0,cTextOffset.size());
                    BOOST_CHECK(!ec);
                    BOOST_CHECK(ciphertext.isEqual(mCiphertext.data()+cTextOffset.size(),ciphertext.size()));
                    BOOST_CHECK(cTextOffset.isEqual(mCiphertext.data(),cTextOffset.size()));

                    pTextOffset="some other offset";
                    mPlaintext.copy(pTextOffset);
                    ec=dec->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=dec->processAndFinalize(mCiphertext,mPlaintext,cTextOffset.size(),0,pTextOffset.size());
                    BOOST_CHECK(!ec);
                    BOOST_CHECK(plaintext.isEqual(mPlaintext.data()+pTextOffset.size(),plaintext.size()));
                    BOOST_CHECK(pTextOffset.isEqual(mPlaintext.data(),pTextOffset.size()));

                    // check multiple finalize without re-init
                    mCiphertext.clear();
                    ec=enc->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=enc->processAndFinalize(plaintext,mCiphertext);
                    BOOST_CHECK(!ec);
                    ec=enc->processAndFinalize(plaintext,mCiphertext);
                    BOOST_CHECK(ec);

                    mPlaintext.clear();
                    ec=dec->init(iv);
                    HATN_REQUIRE(!ec);
                    ec=dec->processAndFinalize(ciphertext,mPlaintext);
                    BOOST_CHECK(!ec);
                    ec=dec->processAndFinalize(ciphertext,mPlaintext);
                    BOOST_CHECK(ec);

                    // check invalid key length
                    if (!alg->isNone())
                    {
                        auto key1=alg->createSymmetricKey();
                        HATN_REQUIRE(key1);
                        ByteArray keyBuf1("blablabla");
                        ec=key1->importFromBuf(keyBuf1,ContainerFormat::RAW_PLAIN);
                        HATN_REQUIRE(!ec);
                        enc->setKey(key1.get());
                        ec=enc->init(iv);
                        BOOST_CHECK(ec);
                        dec->setKey(key1.get());
                        ec=dec->init(iv);
                        BOOST_CHECK(ec);
                    }
                }
            }
        }
    }
}

static void checkCipher(std::shared_ptr<CryptPlugin>& plugin, const std::string& algName, const std::string& fileName)
{
    if (!boost::filesystem::exists(fileName+".check"))
    {
        return;
    }

    std::string msg=fmt::format("Begin checking cipher {} with {}",algName,fileName);
    BOOST_TEST_MESSAGE(msg);

    // check alg
    const CryptAlgorithm* alg=nullptr;
    auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::SENCRYPTION,algName);
    HATN_REQUIRE(!ec);
    if (algName=="-"
        ||
        boost::algorithm::iequals(algName,"none")
        )
    {
        BOOST_CHECK(alg->isNone());
    }
    else
    {
        BOOST_CHECK(!alg->isNone());
    }
    if (!alg->isNone())
    {
        auto ivSize=alg->ivSize();
        BOOST_CHECK_GT(ivSize,static_cast<size_t>(0));
        ByteArray iv;
        ec=plugin->randContainer(iv,ivSize);
        HATN_REQUIRE(!ec);
    }

    // check key creation
    auto key=alg->createSymmetricKey();
    HATN_REQUIRE(key);
    ec=key->generate();
    HATN_REQUIRE(!ec);

    // check encoder/decoder creation
    auto enc=plugin->createSEncryptor(key.get());
    HATN_REQUIRE(enc);
    auto dec=plugin->createSDecryptor(key.get());
    HATN_REQUIRE(dec);

    // run tests of the cipher
    singleEncryptDecrypt(plugin,alg);
    testVectors(plugin,alg,fileName);

    msg=fmt::format("End checking cipher {} with {}",algName,fileName);
    BOOST_TEST_MESSAGE(msg);
}

BOOST_AUTO_TEST_CASE(CheckCiphers)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::SymmetricEncryption))
            {
                const CryptAlgorithm* alg=nullptr;
                auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::SENCRYPTION,"_dummy_");
                BOOST_CHECK(ec);

                auto ciphers=plugin->listCiphers();
                HATN_REQUIRE(!ciphers.empty());
                for (auto&& it:ciphers)
                {
#ifdef PRINT_CRYPT_ENGINE_ALGS
                    BOOST_TEST_MESSAGE(it);
#endif
                    std::string path=fmt::format("{}/cipher-{}",PluginList::assetsPath("crypt"),it);
                    checkCipher(plugin,it,path);
                    path=fmt::format("{}/cipher-{}",PluginList::assetsPath("crypt",plugin->info()->name),it);
                    checkCipher(plugin,it,path);
                }
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
