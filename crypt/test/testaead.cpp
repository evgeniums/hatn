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
#include <hatn/common/makeshared.h>
#include <hatn/common/elapsedtimer.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/aead.h>
#include <hatn/crypt/encryptmac.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

//#define PRINT_CRYPT_ENGINE_ALGS
//#define MEASURE_DECRYPT_PERFORMANCE
BOOST_FIXTURE_TEST_SUITE(TestAEAD,CryptTestFixture)

static void testVectors(std::shared_ptr<CryptPlugin>& plugin, const CryptAlgorithm* alg, const std::string& fileName)
{
    BOOST_TEST_MESSAGE("Checking test vectors");

    if (boost::filesystem::exists(fileName))
    {
        size_t runCount=0;
        auto handler=[&runCount,plugin,alg](std::string& line)
        {
            // reset tag size
            const_cast<CryptAlgorithm*>(alg)->setTagSize(0);

            BOOST_TEST_MESSAGE(fmt::format("Vector #{}",++runCount));
#ifdef MEASURE_DECRYPT_PERFORMANCE
            ElapsedTimer timer;
            size_t measureCount=1000000;
#endif
            std::vector<std::string> parts;
            Utils::trimSplit(parts,line,':');
            HATN_REQUIRE_EQUAL(parts.size(),6);
            const auto& keyHex=parts[0];
            const auto& ivHex=parts[1];
            const auto& plaintextHex=parts[2];
            const auto& authdataHex=parts[3];
            const auto& ciphertextHex=parts[4];
            const auto& tagHex=parts[5];

            ByteArray key,iv,plaintext,authdata,ciphertext,tag;
            ContainerUtils::hexToRaw(keyHex,key);
            ContainerUtils::hexToRaw(ivHex,iv);
            ContainerUtils::hexToRaw(plaintextHex,plaintext);
            ContainerUtils::hexToRaw(authdataHex,authdata);
            ContainerUtils::hexToRaw(ciphertextHex,ciphertext);
            ContainerUtils::hexToRaw(tagHex,tag);

            if (tag.size()!=16)
            {
                const_cast<CryptAlgorithm*>(alg)->setTagSize(tag.size());
            }

            auto keyObj=alg->createSymmetricKey();
            HATN_REQUIRE(keyObj);
            auto ec=keyObj->importFromBuf(key,ContainerFormat::RAW_PLAIN);
            HATN_REQUIRE(!ec)

            SharedPtr<AEADEncryptor> enc1;
            if (alg->isBackendAlgorithm())
            {
                enc1=plugin->createAeadEncryptor(keyObj.get());
            }
            else if (boost::algorithm::istarts_with(std::string(alg->name()),"encryptmac:"))

            {
                enc1=makeShared<EncryptMACEnc>(keyObj.get());
            }

            HATN_REQUIRE(enc1);

            // encrypt from single buffer
            ByteArray rCiphered1;
            ByteArray rTag1;
            ec=AEAD::encrypt(enc1.get(),SpanBuffer{iv},SpanBuffer{plaintext},SpanBuffer{authdata},rCiphered1,rTag1);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rCiphered1==ciphertext);
            BOOST_CHECK(rTag1==tag);

#if 0
            std::string rTagHex1;
            ContainerUtils::rawToHex(rTag1.stringView(),rTagHex1);
            BOOST_TEST_MESSAGE(rTagHex1);
            std::string rCipheredHex1;
            ContainerUtils::rawToHex(rCiphered1.stringView(),rCipheredHex1);
            BOOST_TEST_MESSAGE(rCipheredHex1);
#endif
            SharedPtr<AEADDecryptor> dec1;
            EncryptMACDec* dec1m=nullptr;
            if (alg->isBackendAlgorithm())
            {
                dec1=plugin->createAeadDecryptor(keyObj.get());
            }
            else if (boost::algorithm::istarts_with(std::string(alg->name()),"encryptmac:"))

            {
                auto dec=makeShared<EncryptMACDec>(keyObj.get());
                dec1=dec;
                dec1m=dec.get();
            }
            HATN_REQUIRE(dec1);

            // decrypt from single buffer
            ByteArray rPlain1;
            ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},SpanBuffer{ciphertext},SpanBuffer{authdata},SpanBuffer{tag},rPlain1);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rPlain1==plaintext);
            ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},SpanBuffer{rCiphered1},SpanBuffer{authdata},SpanBuffer{rTag1},rPlain1);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rPlain1==plaintext);

#ifdef MEASURE_DECRYPT_PERFORMANCE
            BOOST_TEST_MESSAGE("Measure good decrypt()");
            timer.reset();
            for (size_t i=0;i<measureCount;i++)
            {
                ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},SpanBuffer{rCiphered1},SpanBuffer{authdata},SpanBuffer{rTag1},rPlain1);
            }
            BOOST_TEST_MESSAGE(timer.toString());
#endif

            if (dec1m)
            {
                ByteArray rPlain1m=ciphertext;
                ec=dec1m->verifyDecrypt(SpanBuffer{iv},SpanBuffer{rCiphered1},SpanBuffer{authdata},SpanBuffer{tag},rPlain1m);
                BOOST_CHECK(!ec);
                BOOST_CHECK(rPlain1m==plaintext);

#ifdef MEASURE_DECRYPT_PERFORMANCE
                BOOST_TEST_MESSAGE("Measure good verifyDecrypt()");
                timer.reset();
                for (size_t i=0;i<measureCount;i++)
                {
                    ec=dec1m->verifyDecrypt(SpanBuffer{iv},SpanBuffer{rCiphered1},SpanBuffer{authdata},SpanBuffer{tag},rPlain1m);
                }
                BOOST_TEST_MESSAGE(timer.toString());
#endif
            }

            // encrypt from scattered buffers
            auto plainBuffers=CryptPluginTest::split(plaintext,4);
            auto authdataBuffers=CryptPluginTest::split(authdata,3);
            ByteArray rCiphered2;
            ByteArray rTag2;
            ec=AEAD::encrypt(enc1.get(),SpanBuffer{iv},plainBuffers,authdataBuffers,rCiphered2,rTag2);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rCiphered2==ciphertext);
            BOOST_CHECK(rTag2==tag);

            // decrypt from scattered buffer
            auto cipheredBuffers=CryptPluginTest::split(ciphertext,4);
            ByteArray rPlain2;
            ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},cipheredBuffers,authdataBuffers,SpanBuffer{tag},rPlain2);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rPlain2==plaintext);
            auto rCipheredBuffers=CryptPluginTest::split(rCiphered1,5);
            ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},rCipheredBuffers,authdataBuffers,SpanBuffer{rTag2},rPlain2);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rPlain2==plaintext);

#ifdef MEASURE_DECRYPT_PERFORMANCE
            BOOST_TEST_MESSAGE("Measure scattered good decrypt()");
            timer.reset();
            for (size_t i=0;i<measureCount;i++)
            {
                ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},rCipheredBuffers,authdataBuffers,SpanBuffer{rTag2},rPlain2);
            }
            BOOST_TEST_MESSAGE(timer.toString());
#endif

            if (dec1m)
            {
                ByteArray rPlain2m;
                ec=dec1m->verifyDecrypt(SpanBuffer{iv},rCipheredBuffers,authdataBuffers,SpanBuffer{tag},rPlain2m);
                BOOST_CHECK(!ec);
                BOOST_CHECK(rPlain2m==plaintext);

#ifdef MEASURE_DECRYPT_PERFORMANCE
                BOOST_TEST_MESSAGE("Measure scattered good verifyDecrypt()");
                timer.reset();
                for (size_t i=0;i<measureCount;i++)
                {
                    ec=dec1m->verifyDecrypt(SpanBuffer{iv},rCipheredBuffers,authdataBuffers,SpanBuffer{tag},rPlain2m);
                }
                BOOST_TEST_MESSAGE(timer.toString());


                BOOST_TEST_MESSAGE("Measure good scattered merge to single then decrypt()");
                timer.reset();
                for (size_t i=0;i<measureCount;i++)
                {
                    ByteArray tmp1,tmp2;
                    for (auto&& it:rCipheredBuffers)
                    {
                        tmp1.append(it.view());
                    }
                    for (auto&& it:authdataBuffers)
                    {
                        tmp2.append(it.view());
                    }
                    ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},tmp1,tmp2,SpanBuffer{rTag2},rPlain2);
                }
                BOOST_TEST_MESSAGE(timer.toString());
#endif
            }

            // encrypt and pack
            ByteArray rCiphered3;
            ec=AEAD::encryptPack(enc1.get(),SpanBuffer{plaintext},SpanBuffer{authdata},rCiphered3,SpanBuffer{iv});
            BOOST_CHECK(!ec);
            BOOST_CHECK_EQUAL(rCiphered3.size(),ciphertext.size()+enc1->ivSize()+enc1->getTagSize());
            BOOST_CHECK(ciphertext.isEqual(rCiphered3.data()+enc1->ivSize()+enc1->getTagSize(),ciphertext.size()));
            BOOST_CHECK(tag.isEqual(rCiphered3.data(),enc1->getTagSize()));
            BOOST_CHECK(iv.isEqual(rCiphered3.data()+enc1->getTagSize(),iv.size()));

            // unpack and decrypt
            ByteArray rPlain3;
            ec=AEAD::decryptPack(dec1.get(),SpanBuffer{rCiphered3},SpanBuffer{authdata},rPlain3);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rPlain3==plaintext);

#ifdef MEASURE_DECRYPT_PERFORMANCE
            BOOST_TEST_MESSAGE("Measure good decryptPack()");
            timer.reset();
            for (size_t i=0;i<measureCount;i++)
            {
                ec=AEAD::decryptPack(dec1.get(),SpanBuffer{rCiphered3},SpanBuffer{authdata},rPlain3);
            }
            BOOST_TEST_MESSAGE(timer.toString());
#endif

            if (dec1m)
            {
                ByteArray rPlain3m;
                ec=dec1m->verifyDecryptPack(SpanBuffer{rCiphered3},SpanBuffer{authdata},rPlain3m);
                BOOST_CHECK(!ec);
                BOOST_CHECK(rPlain3m==plaintext);

#ifdef MEASURE_DECRYPT_PERFORMANCE
                BOOST_TEST_MESSAGE("Measure good verifyDecryptPack()");
                timer.reset();
                for (size_t i=0;i<measureCount;i++)
                {
                    ec=dec1m->verifyDecryptPack(SpanBuffer{rCiphered3},SpanBuffer{authdata},rPlain3m);
                }
                BOOST_TEST_MESSAGE(timer.toString());
#endif
            }

            // encrypt and pack from scattered buffers
            ByteArray rCiphered4;
            ec=AEAD::encryptPack(enc1.get(),plainBuffers,authdataBuffers,rCiphered4,SpanBuffer{iv});
            BOOST_CHECK(!ec);
            BOOST_CHECK_EQUAL(rCiphered4.size(),ciphertext.size()+enc1->ivSize()+enc1->getTagSize());
            BOOST_CHECK(ciphertext.isEqual(rCiphered4.data()+enc1->ivSize()+enc1->getTagSize(),ciphertext.size()));
            BOOST_CHECK(tag.isEqual(rCiphered4.data(),enc1->getTagSize()));
            BOOST_CHECK(iv.isEqual(rCiphered4.data()+enc1->getTagSize(),iv.size()));
            BOOST_CHECK(rCiphered4==rCiphered3);

            // unpack and decrypt from scattered buffers
            ByteArray rPlain4;
            auto rCiphered4Buffers=CryptPluginTest::split(rCiphered3,4,enc1->ivSize()+enc1->getTagSize());
            ec=AEAD::decryptPack(dec1.get(),rCiphered4Buffers,authdataBuffers,rPlain4);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rPlain4==plaintext);

#ifdef MEASURE_DECRYPT_PERFORMANCE
            BOOST_TEST_MESSAGE("Measure scattered good decryptPack()");
            timer.reset();
            for (size_t i=0;i<measureCount;i++)
            {
                ec=AEAD::decryptPack(dec1.get(),rCiphered4Buffers,authdataBuffers,rPlain4);
            }
            BOOST_TEST_MESSAGE(timer.toString());
#endif

            if (dec1m)
            {
                ByteArray rPlain4m;
                ec=dec1m->verifyDecryptPack(rCiphered4Buffers,authdataBuffers,rPlain4m);
                BOOST_CHECK(!ec);
                BOOST_CHECK(rPlain4m==plaintext);
            }

            // spoil ciphered data and try to decrypt
            if (rCiphered3.size()>2)
            {
                if (tagHex!="000000000000000000000000") // exclude special case of poly1305 with zero key
                {
                    rCiphered3[rCiphered3.size()/2]=static_cast<char>(0xaa);
                    ByteArray rPlain5;
                    ec=AEAD::decryptPack(dec1.get(),SpanBuffer{rCiphered3},SpanBuffer{authdata},rPlain5);
                    BOOST_CHECK(ec);

                    BOOST_CHECK(ec);
#ifdef MEASURE_DECRYPT_PERFORMANCE
                    BOOST_TEST_MESSAGE("Measure bad decryptPack()");
                    timer.reset();
                    for (size_t i=0;i<measureCount;i++)
                    {
                        ec=AEAD::decryptPack(dec1.get(),SpanBuffer{rCiphered3},SpanBuffer{authdata},rPlain5);
                    }
                    BOOST_TEST_MESSAGE(timer.toString());
#endif

                    if (dec1m)
                    {
                        ByteArray rPlain5m;
                        ec=dec1m->verifyDecryptPack(rCiphered4Buffers,authdataBuffers,rPlain5m);
                        BOOST_CHECK(ec);
#ifdef MEASURE_DECRYPT_PERFORMANCE
                        BOOST_TEST_MESSAGE("Measure bad verifyDecryptPack()");
                        timer.reset();
                        for (size_t i=0;i<measureCount;i++)
                        {
                            ec=dec1m->verifyDecryptPack(rCiphered4Buffers,authdataBuffers,rPlain5m);
                        }
                        BOOST_TEST_MESSAGE(timer.toString());
#endif
                    }
                }
            }

            // check out of range
            if (!plaintext.isEmpty())
            {
                ec=AEAD::encrypt(enc1.get(),SpanBuffer{iv},SpanBuffer{plaintext,1000,10000},SpanBuffer{authdata},rCiphered1,rTag1);
                BOOST_CHECK(ec);
            }
            if (!authdata.isEmpty())
            {
                ec=AEAD::encrypt(enc1.get(),SpanBuffer{iv},SpanBuffer{plaintext},SpanBuffer{authdata,1000,10000},rCiphered1,rTag1);
                BOOST_CHECK(ec);
            }
            ec=AEAD::encrypt(enc1.get(),SpanBuffer{iv,100,300},SpanBuffer{plaintext},SpanBuffer{authdata},rCiphered1,rTag1);
            BOOST_CHECK(ec);
            if (!rCiphered2.isEmpty())
            {
                ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},SpanBuffer{rCiphered2,1000,10000},SpanBuffer{authdata},SpanBuffer{rTag2},rPlain1);
                BOOST_CHECK(ec);
            }
            if (!authdata.isEmpty())
            {
                ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},SpanBuffer{rCiphered2},SpanBuffer{authdata,1000,10000},SpanBuffer{rTag2},rPlain1);
                BOOST_CHECK(ec);
            }
            ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv,100,300},SpanBuffer{rCiphered2},SpanBuffer{authdata},SpanBuffer{rTag2},rPlain1);
            BOOST_CHECK(ec);
            ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv},SpanBuffer{rCiphered2},SpanBuffer{authdata},SpanBuffer{rTag2,100,300},rPlain1);
            BOOST_CHECK(ec);

            // check IV padding
            ByteArray rTag7;
            ByteArray rCiphered7;
            ByteArray rPlain7;
            ec=AEAD::encrypt(enc1.get(),SpanBuffer{iv.data(),7},SpanBuffer{plaintext},SpanBuffer{authdata},rCiphered7,rTag7);
            BOOST_CHECK(!ec);
            ec=AEAD::decrypt(dec1.get(),SpanBuffer{iv.data(),7},SpanBuffer{rCiphered7},SpanBuffer{authdata},SpanBuffer{rTag7},rPlain7);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rPlain7==plaintext);

            // create ciphers without key
            SharedPtr<AEADEncryptor> enc2;
            if (alg->isBackendAlgorithm())
            {
                enc2=plugin->createAeadEncryptor(keyObj.get());
            }
            else if (boost::algorithm::istarts_with(std::string(alg->name()),"encryptmac:"))
            {
                enc2=makeShared<EncryptMACEnc>(keyObj.get());
            }
            HATN_REQUIRE(enc1);
            HATN_REQUIRE(enc2);
            ByteArray rCiphered10;
            ByteArray rTag10;
            ec=AEAD::encrypt(enc2.get(),keyObj.get(),SpanBuffer{iv},SpanBuffer{plaintext},SpanBuffer{authdata},rCiphered10,rTag10);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rCiphered10==ciphertext);
            BOOST_CHECK(rTag10==tag);

            SharedPtr<AEADDecryptor> dec2;
            if (alg->isBackendAlgorithm())
            {
                dec2=plugin->createAeadDecryptor(keyObj.get());
            }
            else if (boost::algorithm::istarts_with(std::string(alg->name()),"encryptmac:"))

            {
                dec2=makeShared<EncryptMACDec>(keyObj.get());
            }
            HATN_REQUIRE(dec2);
            ByteArray rPlain10;
            ec=AEAD::decrypt(dec2.get(),keyObj.get(),SpanBuffer{iv},SpanBuffer{ciphertext},SpanBuffer{authdata},SpanBuffer{tag},rPlain10);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rPlain10==plaintext);
            ec=AEAD::decrypt(dec2.get(),keyObj.get(),SpanBuffer{iv},SpanBuffer{rCiphered10},SpanBuffer{authdata},SpanBuffer{rTag10},rPlain10);
            BOOST_CHECK(!ec);
            BOOST_CHECK(rPlain10==plaintext);
        };
        PluginList::eachLinefromFile(fileName,handler);
        BOOST_CHECK_GT(runCount,0u);
        BOOST_TEST_MESSAGE(fmt::format("Tested {} vectors",runCount).c_str());
    }
}

static void checkCipher(std::shared_ptr<CryptPlugin>& plugin, const std::string& algName, const std::string& fileName)
{
    if (!boost::filesystem::exists(fileName))
    {
        return;
    }

    std::string msg=fmt::format("Begin checking cipher {} with {}",algName,fileName);
    BOOST_TEST_MESSAGE(msg);

    // check alg
    const CryptAlgorithm* alg=nullptr;
    auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::AEAD,algName);
    HATN_REQUIRE(!ec);
    auto ivSize=alg->ivSize();
    BOOST_CHECK_GT(ivSize,static_cast<size_t>(0));
    ByteArray iv;
    ec=plugin->randContainer(iv,ivSize);
    HATN_REQUIRE(!ec);

    // check key creation
    auto key=alg->createSymmetricKey();
    HATN_REQUIRE(key);
    ec=key->generate();
    HATN_REQUIRE(!ec);

    // check encoder/decoder creation
    if (alg->isBackendAlgorithm())
    {
        auto enc=plugin->createAeadEncryptor(key.get());
        HATN_REQUIRE(enc);
        auto dec=plugin->createAeadDecryptor(key.get());
        HATN_REQUIRE(dec);
    }

    testVectors(plugin,alg,fileName);

    msg=fmt::format("End checking cipher {} with {}",algName,fileName);
    BOOST_TEST_MESSAGE(msg);
}

BOOST_AUTO_TEST_CASE(CheckAead)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::AEAD))
            {
                const CryptAlgorithm* alg=nullptr;
                auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::AEAD,"_dummy_");
                BOOST_CHECK(ec);

                auto ciphers=plugin->listAEADs();
                HATN_REQUIRE(!ciphers.empty());
                for (auto&& it:ciphers)
                {
#ifdef PRINT_CRYPT_ENGINE_ALGS
                    BOOST_TEST_MESSAGE(it);
#endif
                    std::string fileName=it;
                    boost::algorithm::replace_all(fileName,std::string("/"),std::string("-"));
                    boost::algorithm::replace_all(fileName,std::string(":"),std::string("-"));

                    std::string path=fmt::format("{}/aead-{}.txt",PluginList::assetsPath("crypt"),fileName);
                    checkCipher(plugin,it,path);
                    path=fmt::format("{}/aead-{}.txt",PluginList::assetsPath("crypt",plugin->info()->name),fileName);
                    checkCipher(plugin,it,path);
                }
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
