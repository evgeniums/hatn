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
#include <hatn/common/fileutils.h>
#include <hatn/common/plainfile.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/cryptfile.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

// #define HATN_SAVE_TEST_FILES

BOOST_FIXTURE_TEST_SUITE(TestCryptFile,CryptTestFixture)

static void prepareContainer(CryptContainer& container,
                            bool defaultChunkSizes,
                            bool attachCipherSuite,
                            size_t maxChunkSize,
                            size_t maxFirstChunkSize,
                            container_descriptor::KdfType kdfType,
                            const std::string& salt
                            )
{
    BOOST_CHECK_EQUAL(container.chunkMaxSize(),0x40000u);
    BOOST_CHECK_EQUAL(container.firstChunkMaxSize(),0x1000u);
    if (!defaultChunkSizes)
    {
        container.setChunkMaxSize(static_cast<uint32_t>(maxChunkSize));
        BOOST_CHECK_EQUAL(container.chunkMaxSize(),static_cast<uint32_t>(maxChunkSize));
        container.setFirstChunkMaxSize(static_cast<uint32_t>(maxFirstChunkSize));
        BOOST_CHECK_EQUAL(container.firstChunkMaxSize(),maxFirstChunkSize);
    }
    auto containerSalt=container.salt();
    std::string containerSaltStr(containerSalt.data(),containerSalt.size());
    BOOST_CHECK_EQUAL(containerSaltStr,std::string());
    container.setSalt(salt);
    containerSalt=container.salt();
    containerSaltStr=std::string(containerSalt.data(),containerSalt.size());
    BOOST_CHECK_EQUAL(containerSaltStr,salt);
    BOOST_CHECK_EQUAL(container.isAttachCipherSuiteEnabled(),false);
    container.setAttachCipherSuiteEnabled(attachCipherSuite);
    BOOST_CHECK_EQUAL(container.isAttachCipherSuiteEnabled(),attachCipherSuite);
    BOOST_CHECK_EQUAL(static_cast<int>(container.kdfType()),static_cast<int>(container_descriptor::KdfType::PbkdfThenHkdf));
    container.setKdfType(kdfType);
    BOOST_CHECK_EQUAL(static_cast<int>(container.kdfType()),static_cast<int>(kdfType));
}

#define HATN_TEST_FILE_MESSAGE(x) BOOST_TEST_MESSAGE(x)
// #define HATN_TEST_FILE_MESSAGE(x)

static void writeExistingFile(
        const std::string& masterCryptFilePath,
        std::shared_ptr<CryptPlugin>& plugin,
        const ByteArray& plaintext,
        const SymmetricKey* masterKey
    )
{
    ByteArray tmpByteArray;
    auto ec=plugin->randContainer(tmpByteArray,0x400000);
    HATN_REQUIRE(!ec);

    for (size_t i=0;i<8;i++)
    {
        HATN_TEST_FILE_MESSAGE("***begin read write***");

        auto plainFilePath=fmt::format("{}/plainfile.dat",hatn::test::MultiThreadFixture::tmpPath());
        std::ignore=FileUtils::remove(plainFilePath);
        ec=plaintext.saveToFile(plainFilePath);
        HATN_REQUIRE(!ec);

        auto tmpFile=fmt::format("{}/cryptfile.dat",hatn::test::MultiThreadFixture::tmpPath());
        std::ignore=FileUtils::remove(tmpFile);
        ec=FileUtils::copy(masterCryptFilePath,tmpFile);
        HATN_REQUIRE(!ec);

        bool flush=static_cast<bool>(i%2);
        bool sync=i==3;
        bool fsync=i==5;
        bool readAfterWrite=i==0 || i==1 || i==4 || i==5;
        bool doubleWrite=i>4;

        // codechecker_false_positive [all]
        if (flush)
        {
            HATN_TEST_FILE_MESSAGE("Check write/read with immediate flush");
        }
        else
        {
            HATN_TEST_FILE_MESSAGE("Check write/read with delayed flush");
        }
        if (sync)
        {
            HATN_TEST_FILE_MESSAGE("Check write/read with sync");
        }
        if (fsync)
        {
            HATN_TEST_FILE_MESSAGE("Check write/read with fsync");
        }
        // codechecker_false_positive [all]
        if (readAfterWrite)
        {
            HATN_TEST_FILE_MESSAGE("Check write read with read after write");
        }
        else
        {
            HATN_TEST_FILE_MESSAGE("Check write read without read after write");
        }

        // open files
        PlainFile plainFile;
        ec=plainFile.open(plainFilePath,File::Mode::write_existing);
        HATN_REQUIRE(!ec);
        CryptFile cryptFile;
        cryptFile.processor().setMasterKey(masterKey);
        ec=cryptFile.open(tmpFile,File::Mode::write_existing);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        HATN_REQUIRE(!ec);

        // handler for checking current size
        auto currentSize=plaintext.size();
        auto checkSize=[&plainFile,&cryptFile,&currentSize]()
        {
            BOOST_CHECK_EQUAL(currentSize,plainFile.size());
            BOOST_CHECK_EQUAL(currentSize,cryptFile.size());
            currentSize=static_cast<size_t>(plainFile.size());
        };

        // handler for writing and reading
        auto writeData=[&plainFile,&cryptFile,&tmpByteArray,flush,sync,fsync,readAfterWrite,doubleWrite](size_t fileOffset, size_t size, size_t dataOffset)
        {
            auto ec=plainFile.seek(fileOffset);
            HATN_REQUIRE(!ec);
            auto writtenPlain=plainFile.write(tmpByteArray.data()+dataOffset,size,ec);
            HATN_REQUIRE(!ec);
            BOOST_CHECK_EQUAL(writtenPlain,size);
            ec=cryptFile.seek(fileOffset);
            HATN_REQUIRE(!ec);
            auto writtenCrypted=cryptFile.write(tmpByteArray.data()+dataOffset,size,ec);
            HATN_REQUIRE(!ec);
            BOOST_CHECK_EQUAL(writtenCrypted,size);
            if (doubleWrite)
            {
                writtenPlain=plainFile.write(tmpByteArray.data()+dataOffset+size,size,ec);
                HATN_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(writtenPlain,size);
                writtenCrypted=cryptFile.write(tmpByteArray.data()+dataOffset+size,size,ec);
                HATN_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(writtenCrypted,size);
            }

            if (flush)
            {
                ec=plainFile.flush();
                HATN_REQUIRE(!ec);
                ec=cryptFile.flush();
                HATN_REQUIRE(!ec);
            }

            if (sync)
            {
                ec=plainFile.sync();
                HATN_REQUIRE(!ec);
                ec=cryptFile.sync();
                HATN_REQUIRE(!ec);
            }

            if (fsync)
            {
                ec=plainFile.fsync();
                HATN_REQUIRE(!ec);
                ec=cryptFile.fsync();
                HATN_REQUIRE(!ec);
            }

            if (readAfterWrite)
            {
                ByteArray plainReadData, cryptReadData;
                plainReadData.resize(size);
                cryptReadData.resize(size);
                ec=plainFile.seek(fileOffset);
                HATN_REQUIRE(!ec);
                auto readPlain=plainFile.read(plainReadData.data(),size,ec);
                HATN_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(readPlain,size);
                ec=cryptFile.seek(fileOffset);
                HATN_REQUIRE(!ec);
                auto readCrypted=cryptFile.read(cryptReadData.data(),size,ec);
                HATN_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(readCrypted,size);

                BOOST_CHECK(plainReadData==cryptReadData);

                if (doubleWrite)
                {
                    readPlain=plainFile.read(plainReadData.data(),size,ec);
                    HATN_REQUIRE(!ec);
                    BOOST_CHECK_EQUAL(readPlain,size);
                    readCrypted=cryptFile.read(cryptReadData.data(),size,ec);
                    HATN_REQUIRE(!ec);
                    BOOST_CHECK_EQUAL(readCrypted,size);

                    BOOST_CHECK(plainReadData==cryptReadData);
                }
            }
        };

        size_t ratio=1;
        if (doubleWrite)
        {
            ratio=2;
        }

        // end
        HATN_TEST_FILE_MESSAGE("write to end");
        writeData(currentSize,0x500,0x700);
        currentSize+=0x500*ratio;
        checkSize();

        // begin
        HATN_TEST_FILE_MESSAGE("write to begin");
        writeData(0,0x100,0);
        checkSize();

        // middle
        HATN_TEST_FILE_MESSAGE("write to middle");
        decltype(currentSize) offset1=currentSize/2;
        writeData(offset1,offset1-50,0x10000);
        if (doubleWrite)
        {
            if ((currentSize%2)==0)
            {
                currentSize+=offset1-100;
            }
            else
            {
                currentSize+=offset1-101;
            }
        }
        checkSize();

        // begin with offset
        HATN_TEST_FILE_MESSAGE("write to begin with offset");
        writeData(0x50,0x300,0x7700);
        checkSize();

        // end + append
        HATN_TEST_FILE_MESSAGE("write to end with append");
        writeData(currentSize-100,300,0x10000);
        currentSize+=200;
        if (doubleWrite)
        {
            currentSize+=300;
        }
        checkSize();

        // middle + append
        HATN_TEST_FILE_MESSAGE("write to middle with append");
        decltype(currentSize) offset=currentSize/2;
        writeData(offset,offset+1000,0x50000);
        currentSize=2*offset+1000;
        if (doubleWrite)
        {
            currentSize+=offset+1000;
        }
        checkSize();

        // the same chunk, less than chunk size
        HATN_TEST_FILE_MESSAGE("write to the same chunk");
        writeData(currentSize-200,100,0x34000);
        checkSize();

        // the same chunk, up to the chunk size
        HATN_TEST_FILE_MESSAGE("the same chunk, up to the chunk size");
        writeData(currentSize-100,100,0x79000);
        if (doubleWrite)
        {
            currentSize+=100;
        }
        checkSize();

        // the same chunk + append
        HATN_TEST_FILE_MESSAGE("the same chunk + append");
        writeData(currentSize-80,500,0x21100);
        currentSize+=500-80;
        if (doubleWrite)
        {
            currentSize+=500;
        }
        checkSize();

        // middle
        HATN_TEST_FILE_MESSAGE("write to middle quarter");
        writeData(currentSize/2-1000,currentSize/4,0x1000);
        checkSize();

        // first chunk
        HATN_TEST_FILE_MESSAGE("write to first chunk");
        writeData(0,cryptFile.processor().firstChunkMaxSize(),0x3900);
        if (currentSize<(cryptFile.processor().firstChunkMaxSize())*ratio)
        {
            currentSize=cryptFile.processor().firstChunkMaxSize()*ratio;
        }
        checkSize();

        // third chunk
        HATN_TEST_FILE_MESSAGE("write to third chunk");
        if (currentSize>=(cryptFile.processor().firstChunkMaxSize()+cryptFile.processor().chunkMaxSize()))
        {
            writeData(cryptFile.processor().firstChunkMaxSize()+cryptFile.processor().chunkMaxSize(),
                      cryptFile.processor().chunkMaxSize(),0x7500);
            if (!doubleWrite)
            {
                if (currentSize<
                        (
                            cryptFile.processor().firstChunkMaxSize()
                            +
                            cryptFile.processor().chunkMaxSize()
                            +
                            cryptFile.processor().chunkMaxSize()
                         )
                   )
                {
                    currentSize=(cryptFile.processor().firstChunkMaxSize()
                            +
                            cryptFile.processor().chunkMaxSize()
                            +
                            cryptFile.processor().chunkMaxSize());
                }
                checkSize();
            }
            else
            {
                currentSize=static_cast<size_t>(plainFile.size());
            }
        }

        // last chunk
        HATN_TEST_FILE_MESSAGE("write to last chunk");
        if (currentSize>=cryptFile.processor().chunkMaxSize())
        {
            writeData(currentSize-cryptFile.processor().chunkMaxSize(),
                      cryptFile.processor().chunkMaxSize(),0x1200);
            if (doubleWrite)
            {
                currentSize+=cryptFile.processor().chunkMaxSize();
            }
            checkSize();
        }

        // first chunk plus
        HATN_TEST_FILE_MESSAGE("write to first chunk plus");
        writeData(0,cryptFile.processor().firstChunkMaxSize()+100,0x55000);
        if (currentSize<(cryptFile.processor().firstChunkMaxSize()+100)*ratio)
        {
            currentSize=(cryptFile.processor().firstChunkMaxSize()+100)*ratio;
        }
        checkSize();

        // third chunk plus
        HATN_TEST_FILE_MESSAGE("write to third chunk plus");
        if (currentSize>=(cryptFile.processor().firstChunkMaxSize()+cryptFile.processor().chunkMaxSize()))
        {
            writeData(cryptFile.processor().firstChunkMaxSize()+cryptFile.processor().chunkMaxSize(),
                      cryptFile.processor().chunkMaxSize()+800,0x23000);
            if (currentSize<
                    (
                        cryptFile.processor().firstChunkMaxSize()
                        +
                        cryptFile.processor().chunkMaxSize()
                    )
                        +
                    (cryptFile.processor().chunkMaxSize()+800)*ratio
               )
            {
                // auto prevSize=currentSize;
                currentSize=(cryptFile.processor().firstChunkMaxSize()
                        +
                        cryptFile.processor().chunkMaxSize()
                        +
                        cryptFile.processor().chunkMaxSize()+800)*ratio;
            }
            checkSize();
        }

        // last chunk plus
        HATN_TEST_FILE_MESSAGE("write to last chunk plus");
        if (currentSize>=cryptFile.processor().chunkMaxSize())
        {
            writeData(currentSize-cryptFile.processor().chunkMaxSize(),
                      cryptFile.processor().chunkMaxSize()+121,0x9800);
            currentSize+=121;
            if (doubleWrite)
            {
                currentSize+=cryptFile.processor().chunkMaxSize()+121;
            }
            checkSize();
        }

        HATN_TEST_FILE_MESSAGE("close file");

        // close file
        plainFile.close(ec);
        BOOST_CHECK(!ec);
        cryptFile.close(ec);
        BOOST_CHECK(!ec);

        // compare contents
        ByteArray plainData1;
        ec=plainData1.loadFromFile(plainFilePath);
        BOOST_CHECK(!ec);
        BOOST_CHECK_EQUAL(currentSize,plainData1.size());

        ByteArray plainData2;
        PlainFile plainFile2;
        ec=plainFile2.readAll(plainFilePath,plainData2);
        BOOST_CHECK(!ec);
        BOOST_CHECK(plainData1==plainData2);

        ByteArray cryptData2;
        CryptFile cryptFile2(masterKey);
        ec=cryptFile2.readAll(tmpFile,cryptData2);
        BOOST_CHECK(!ec);
        BOOST_CHECK(plainData1==cryptData2);

        // remove plainfile
        ec=FileUtils::remove(plainFilePath);
        BOOST_CHECK(!ec);

        // remove tmp crypt file
        ec=FileUtils::remove(tmpFile);
        BOOST_CHECK(!ec);

        HATN_TEST_FILE_MESSAGE("***done read write***");
    }
}

static void checkReadWrite(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    BOOST_TEST_MESSAGE(fmt::format("Checking test vectors in {}",path));
    size_t runCount=0;

    // up to 16 test bundles
    for (size_t i=1u;i<=16u;i++)
    {
        std::string cipherSuiteFile=fmt::format("{}/cryptcontainer-ciphersuite{}.json",path,i);
        std::string plainTextFile=fmt::format("{}/cryptfile-plaintext{}.dat",path,i);
        std::string cipherTextFile=fmt::format("{}/cryptfile-ciphertext{}.dat",path,i);
        std::string configFile=fmt::format("{}/cryptfile-config{}.dat",path,i);

        if (!boost::filesystem::exists(cipherSuiteFile) && !boost::filesystem::exists(configFile) && i>8)
        {
            size_t ii=i%8;
            if (i==16)
            {
                ii=8;
            }
            cipherSuiteFile=fmt::format("{}/cryptcontainer-ciphersuite{}.json",path,ii);
            configFile=fmt::format("{}/cryptfile-config{}.dat",path,ii);
        }

        if (boost::filesystem::exists(cipherSuiteFile)
            &&
            boost::filesystem::exists(plainTextFile)
            &&
            boost::filesystem::exists(cipherTextFile)
            &&
            boost::filesystem::exists(configFile)
           )
        {
            BOOST_TEST_MESSAGE(fmt::format("Testing vector #{}",i));

            // parse config
            auto configStr=PluginList::linefromFile(configFile);
            std::vector<std::string> parts;
            Utils::trimSplit(parts,configStr,':');
            HATN_REQUIRE_GE(parts.size(),7u);
            const auto& suiteID=parts[0];
            const auto& masterKeyStr=parts[1];
            const auto& kdfTypeStr=parts[2];
            auto kdfTypeInt=std::stoul(kdfTypeStr);
            HATN_REQUIRE(kdfTypeInt==0 || kdfTypeInt==1 || kdfTypeInt==2);
            auto kdfType=static_cast<container_descriptor::KdfType>(kdfTypeInt);
            const auto& salt=parts[3];
            const auto& maxFirstChunkSizeStr=parts[4];
            auto maxFirstChunkSize=std::stoul(maxFirstChunkSizeStr);
            const auto& maxChunkSizeStr=parts[5];
            auto maxChunkSize=std::stoul(maxChunkSizeStr);
            const auto& attachCipherSuiteStr=parts[6];
            auto attachCipherSuite=static_cast<bool>(std::stoul(attachCipherSuiteStr));
            bool defaultChunkSizes=false;
            if (parts.size()>7)
            {
                const auto& defaultChunkSizesStr=parts[7];
                defaultChunkSizes=static_cast<bool>(std::stoul(defaultChunkSizesStr));
            }

            // load suite from json
            ByteArray cipherSuiteJson;
            auto ec=cipherSuiteJson.loadFromFile(cipherSuiteFile);
            HATN_REQUIRE(!ec);
            auto createSuite=std::make_shared<CipherSuite>();
            ec=createSuite->loadFromJSON(cipherSuiteJson);
            HATN_REQUIRE(!ec);
            BOOST_CHECK_EQUAL(suiteID,std::string(createSuite->id()));
            // check json
            std::string json;
            bool jsonParsed=createSuite->toJSON(json,true);
            BOOST_CHECK(jsonParsed);

            // check serializing/deserializing of the suite
            ByteArray suiteData;
            ec=createSuite->store(suiteData);
            BOOST_CHECK(!ec);
            auto checkSuite=std::make_shared<CipherSuite>();
            ec=checkSuite->load(suiteData);
            BOOST_CHECK(!ec);
            std::string json1;
            jsonParsed=createSuite->toJSON(json1,true);
            BOOST_CHECK(jsonParsed);
            BOOST_CHECK_EQUAL(json,json1);
            checkSuite.reset();

            // add suite to table of suites
            CipherSuitesGlobal::instance().addSuite(createSuite);

            // set engine
            auto engine=std::make_shared<CryptEngine>(plugin.get());
            CipherSuitesGlobal::instance().setDefaultEngine(std::move(engine));

            // check suite
            auto suite=CipherSuitesGlobal::instance().suite(suiteID.c_str());
            HATN_REQUIRE(suite);

            // check AEAD algorithm
            const CryptAlgorithm* aeadAlg=nullptr;
            ec=suite->aeadAlgorithm(aeadAlg);
            if (ec)
            {
                continue;
            }
            HATN_REQUIRE(aeadAlg);

            // check KDF algorithm and setup master key
            common::SharedPtr<SymmetricKey> masterKey;
            const CryptAlgorithm* kdfAlg=nullptr;
            if (kdfType==container_descriptor::KdfType::PBKDF || kdfType==container_descriptor::KdfType::PbkdfThenHkdf)
            {
                ec=suite->pbkdfAlgorithm(kdfAlg);
                if (ec)
                {
                    continue;
                }
                masterKey=plugin->createPassphraseKey();
                HATN_REQUIRE(masterKey);
                ec=masterKey->importFromBuf(masterKeyStr,ContainerFormat::RAW_PLAIN);
                HATN_REQUIRE(!ec)
            }
            else
            {
                ec=suite->hkdfAlgorithm(kdfAlg);
                if (ec)
                {
                    continue;
                }
                masterKey=aeadAlg->createSymmetricKey();

                HATN_REQUIRE(masterKey);
                ByteArray masterKeyData;
                ContainerUtils::hexToRaw(masterKeyStr,masterKeyData);
                ec=masterKey->importFromBuf(masterKeyData,ContainerFormat::RAW_PLAIN);
                HATN_REQUIRE(!ec)
            }
            HATN_REQUIRE(kdfAlg);

            ByteArray plaintext1;
#ifdef HATN_SAVE_TEST_FILES
            ec=plugin->randContainer(plaintext1,0x80000u,1);
            HATN_REQUIRE(!ec);
            ec=plaintext1.saveToFile(plainTextFile);
            HATN_REQUIRE(!ec);
            std::ignore=FileUtils::remove(cipherTextFile);
#else
            ec=plaintext1.loadFromFile(plainTextFile);
            HATN_REQUIRE(!ec);
#endif

            // prepare crypt file
            CryptFile cryptFile1(masterKey.get(),suite);
            auto& container1=cryptFile1.processor();
            prepareContainer(container1,
                            defaultChunkSizes,
                            attachCipherSuite,
                            maxChunkSize,
                            maxFirstChunkSize,
                            kdfType,
                            salt
                            );

            // check single shot writing
#ifdef HATN_SAVE_TEST_FILES
            auto tmpFile=cipherTextFile;
#else
            std::string tmpFile;
            tmpFile=fmt::format("{}/cryptfile.dat",hatn::test::MultiThreadFixture::tmpPath());
#endif
            for (size_t i=0;i<3;i++)
            {
                auto wrMode=File::Mode::write;
                if (i==0)
                {
                    wrMode=File::Mode::write;
                    BOOST_TEST_MESSAGE(fmt::format("Write mode"));
                }
                else
                {
                    ec=common::FileUtils::remove(tmpFile);
                    BOOST_REQUIRE(!ec);
                    if (i==1)
                    {
                        wrMode=File::Mode::append;
                        BOOST_TEST_MESSAGE(fmt::format("Append after remove"));
                    }
                    else
                    {
                        wrMode=File::Mode::write_new;
                        BOOST_TEST_MESSAGE(fmt::format("Write new after remove"));
                    }
                }

                auto isOpen=cryptFile1.isOpen();
                BOOST_CHECK(!isOpen);
                ec=cryptFile1.open(tmpFile,wrMode);
                if (ec)
                {
                    BOOST_TEST_MESSAGE(ec.message());
                }
                HATN_REQUIRE(!ec);
                isOpen=cryptFile1.isOpen();
                BOOST_CHECK(isOpen);
                // check size
                auto size=cryptFile1.size(ec);
                BOOST_CHECK(!ec);
                BOOST_CHECK_EQUAL(size,0ul);
                // check pos
                auto pos=cryptFile1.pos(ec);
                BOOST_CHECK(!ec);
                BOOST_CHECK_EQUAL(pos,0ul);
                // write to file
                auto written=cryptFile1.write(plaintext1.data(),plaintext1.size(),ec);
                HATN_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(written,plaintext1.size());
                // check size
                size=cryptFile1.size(ec);
                BOOST_CHECK_EQUAL(size,plaintext1.size());
                // check pos
                pos=cryptFile1.pos(ec);
                BOOST_CHECK(!ec);
                BOOST_CHECK_EQUAL(pos,plaintext1.size());
                // close file
                cryptFile1.close(ec);
                BOOST_CHECK(!ec);
                isOpen=cryptFile1.isOpen();
                BOOST_CHECK(!isOpen);
                // check size of closed file
                size=cryptFile1.size(ec);
                BOOST_CHECK(!ec);
                BOOST_CHECK_EQUAL(size,plaintext1.size());
                PlainFile plainFile1;
                plainFile1.setFilename(tmpFile);
                auto rawSize1=plainFile1.size(ec);
                BOOST_CHECK(!ec);
                auto rawSize2=cryptFile1.storageSize(ec);
                BOOST_CHECK(!ec);
                BOOST_CHECK_EQUAL(rawSize1,rawSize2);
                auto rawSize3=cryptFile1.usedSize(ec);
                BOOST_CHECK(!ec);
                BOOST_CHECK_EQUAL(rawSize1,rawSize3);

                // check single shot reading
                for (size_t j=0;j<2;j++)
                {
                    auto rdMode=File::Mode::read;
                    if (j==1)
                    {
                        rdMode=File::Mode::scan;
                    }
                    for (size_t k=0;k<2;k++)
                    {
                        auto rdFile=tmpFile;
                        if (k==1)
                        {
                            rdFile=cipherTextFile;
                        }
                        CryptFile cryptFile2(masterKey.get(),suite);
                        isOpen=cryptFile2.isOpen();
                        BOOST_CHECK(!isOpen);
                        // open file for read
                        ec=cryptFile2.open(tmpFile,rdMode);
                        HATN_REQUIRE(!ec);
                        isOpen=cryptFile2.isOpen();
                        BOOST_CHECK(isOpen);
                        // check size
                        size=cryptFile2.size(ec);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK_EQUAL(size,plaintext1.size());

                        rawSize2=cryptFile2.storageSize(ec);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK_EQUAL(rawSize1,rawSize2);
                        rawSize3=cryptFile2.usedSize(ec);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK_EQUAL(rawSize1,rawSize3);

                        // check pos
                        pos=cryptFile2.pos(ec);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK_EQUAL(pos,0ul);
                        // read from file
                        ByteArray plainTextRd;
                        plainTextRd.resize(plaintext1.size());
                        auto read=cryptFile2.read(plainTextRd.data(),plainTextRd.size(),ec);
                        HATN_REQUIRE(!ec);
                        BOOST_CHECK_EQUAL(read,plaintext1.size());
                        BOOST_CHECK(plainTextRd==plaintext1);
                        // check size
                        size=cryptFile2.size(ec);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK_EQUAL(size,plaintext1.size());
                        // check pos
                        pos=cryptFile2.pos(ec);
                        BOOST_CHECK(!ec);
                        BOOST_CHECK_EQUAL(pos,plaintext1.size());
                        // close file
                        cryptFile2.close(ec);
                        BOOST_CHECK(!ec);
                        isOpen=cryptFile2.isOpen();
                        BOOST_CHECK(!isOpen);
                    }
                }

                ByteArray cryptData2;
                CryptFile cryptFile2(masterKey.get());
                ec=cryptFile2.readAll(tmpFile,cryptData2);
                BOOST_CHECK(!ec);
                BOOST_CHECK(plaintext1==cryptData2);
            }
            if (tmpFile!=cipherTextFile)
            {
                ec=FileUtils::remove(tmpFile);
                BOOST_CHECK(!ec);
                writeExistingFile(cipherTextFile,plugin,plaintext1,masterKey.get());

                auto plainFilePath=fmt::format("{}/plainfile.dat",hatn::test::MultiThreadFixture::tmpPath());
                std::ignore=FileUtils::remove(tmpFile);
                std::ignore=FileUtils::remove(plainFilePath);
            }

            // reset suites
            CipherSuitesGlobal::instance().reset();

            BOOST_TEST_MESSAGE(fmt::format("Done vector #{}",i));
            ++runCount;
        }
    }
    BOOST_TEST_MESSAGE(fmt::format("Tested {} vectors",runCount));
}

static void checkFileStamp(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    // load suite from json
    auto cipherSuiteFile=fmt::format("{}/cryptcontainer-ciphersuite1.json",path);
    auto keyFile=fmt::format("{}/cryptfile-stamp-key.dat",path);

    if (!boost::filesystem::exists(cipherSuiteFile)
            ||
        !boost::filesystem::exists(keyFile)
        )
    {
        return;
    }

    ByteArray cipherSuiteJson;
    auto ec=cipherSuiteJson.loadFromFile(cipherSuiteFile);
    HATN_REQUIRE(!ec);
    auto suite=std::make_shared<CipherSuite>();
    ec=suite->loadFromJSON(cipherSuiteJson);
    HATN_REQUIRE(!ec);

    // add suite to table of suites
    CipherSuitesGlobal::instance().addSuite(suite);

    // set engine
    auto engine=std::make_shared<CryptEngine>(plugin.get());
    CipherSuitesGlobal::instance().setDefaultEngine(std::move(engine));

    // check AEAD algorithm
    const CryptAlgorithm* aeadAlg=nullptr;
    ec=suite->aeadAlgorithm(aeadAlg);
    if (ec)
    {
        return;
    }
    HATN_REQUIRE(aeadAlg);

    // check pbkdf algorithm
    const CryptAlgorithm* kdfAlg=nullptr;
    ec=suite->pbkdfAlgorithm(kdfAlg);
    if (ec)
    {
        return;
    }

    // load master key
    common::SharedPtr<SymmetricKey> masterKey;
    masterKey=plugin->createPassphraseKey();
    HATN_REQUIRE(masterKey);
    ec=masterKey->importFromFile(keyFile,ContainerFormat::RAW_PLAIN);
    HATN_REQUIRE(!ec)

#ifdef HATN_SAVE_TEST_FILES

    auto tmpFileStampSave=fmt::format("{}/cryptfile-stamp.dat",hatn::test::MultiThreadFixture::tmpPath());
    auto tmpFileNoStampSave=fmt::format("{}/cryptfile-no-stamp.dat",hatn::test::MultiThreadFixture::tmpPath());

    ByteArray plaintext1;
    auto plainTextFile=fmt::format("{}/cryptfile-plaintext10.dat",path);
    ec=plaintext1.loadFromFile(plainTextFile);
    BOOST_CHECK(!ec);

    CryptFile cryptFileSample1(masterKey.get(),suite.get());
    ec=cryptFileSample1.open(tmpFileStampSave,CryptFile::Mode::write);
    BOOST_CHECK(!ec);
    if (!ec)
    {
        cryptFileSample1.write(plaintext1.data(),plaintext1.size());
        cryptFileSample1.close();
    }
    else
    {
        BOOST_TEST_MESSAGE(ec.message());
    }

    CryptFile cryptFileSample2(masterKey.get(),suite.get());
    ec=cryptFileSample2.open(tmpFileNoStampSave,CryptFile::Mode::write);
    BOOST_CHECK(!ec);
    if (!ec)
    {
        cryptFileSample2.write(plaintext1.data(),plaintext1.size());
        cryptFileSample2.close();
    }
    else
    {
        BOOST_TEST_MESSAGE(ec.message());
    }

#endif

    // check operations
    std::vector<std::string> types{"digest","mac","digest-mac"};
    for (auto&& type:types)
    {
        BOOST_TEST_MESSAGE(fmt::format("Testing stamp for {}",type));

        auto tmpFileStamp=fmt::format("{}/cryptfile-stamp-{}.dat",hatn::test::MultiThreadFixture::tmpPath(),type);
        auto tmpFileNoStamp=fmt::format("{}/cryptfile-no-stamp-{}.dat",hatn::test::MultiThreadFixture::tmpPath(),type);

        std::ignore=FileUtils::remove(tmpFileStamp);
        std::ignore=FileUtils::remove(tmpFileNoStamp);

        std::string cipherTextFile=fmt::format("{}/cryptfile-stamp-{}.dat",path,type);
        std::string cipherTextFileNoStamp=fmt::format("{}/cryptfile-no-stamp.dat",path);
        if (!boost::filesystem::exists(cipherTextFileNoStamp)
                ||
            !boost::filesystem::exists(cipherTextFile)
            )
        {
            continue;
        }

#ifndef HATN_SAVE_TEST_FILES
        ec=FileUtils::copy(cipherTextFile,tmpFileStamp);
        HATN_REQUIRE(!ec);
        ec=FileUtils::copy(cipherTextFileNoStamp,tmpFileNoStamp);
        HATN_REQUIRE(!ec);
#else
        ec=FileUtils::copy(tmpFileStampSave,tmpFileStamp);
        HATN_REQUIRE(!ec);
        ec=FileUtils::copy(tmpFileNoStampSave,tmpFileNoStamp);
        HATN_REQUIRE(!ec);
#endif

        // check size of open file
        CryptFile cryptFile1;
        std::ignore=cryptFile1.open(tmpFileNoStamp,common::File::Mode::append_existing,ec);
        HATN_REQUIRE(!ec);
        auto size1=cryptFile1.size();
        CryptFile cryptFile2;
        cryptFile2.setFilename(tmpFileNoStamp);
        auto size2=cryptFile2.size(ec);
        BOOST_REQUIRE(!ec);
        BOOST_TEST_MESSAGE(fmt::format("Comparing sizes of open and not open files: {}={}",size1,size2));
        BOOST_CHECK_EQUAL(size2,size1);
        cryptFile1.close(ec);
        BOOST_REQUIRE(!ec);

        // init files
        CryptFile cryptFileNoStamp(masterKey.get());
        cryptFileNoStamp.setFilename(tmpFileNoStamp);

        CryptFile cryptFileStamp(masterKey.get());
        cryptFileStamp.setFilename(tmpFileStamp);

        // check digest in file without stamp
        if (type=="digest" || type=="digest-mac")
        {
            ec=cryptFileNoStamp.checkStampDigest();
            BOOST_CHECK(ec);
        }
        // check mac in file without stamp
        if (type=="mac" || type=="digest-mac")
        {
            ec=cryptFileNoStamp.verifyStampMAC();
            BOOST_CHECK(ec);
        }

        // add digest to file without stamp
        if (type=="digest" || type=="digest-mac")
        {
            ec=cryptFileNoStamp.stampDigest();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
#ifdef HATN_SAVE_TEST_FILES
            ec=cryptFileStamp.stampDigest();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
#endif
        }
        // add mac to file without stamp
        if (type=="mac" || type=="digest-mac")
        {
            ec=cryptFileNoStamp.stampMAC();            
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
#ifdef HATN_SAVE_TEST_FILES
            ec=cryptFileStamp.stampMAC();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
#endif
        }

        // check digest
        if (type=="digest" || type=="digest-mac")
        {
            ec=cryptFileNoStamp.checkStampDigest();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
            ec=cryptFileStamp.checkStampDigest();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
        }
        // check mac
        if (type=="mac" || type=="digest-mac")
        {
            ec=cryptFileNoStamp.verifyStampMAC();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
            ec=cryptFileStamp.verifyStampMAC();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
        }

#ifndef HATN_SAVE_TEST_FILES
        // add digest to file with stamp
        if (type=="digest" || type=="digest-mac")
        {
            ec=cryptFileStamp.stampDigest();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
        }
        // add mac to file with stamp
        if (type=="mac" || type=="digest-mac")
        {
            ec=cryptFileStamp.stampMAC();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
        }

        // check digest
        if (type=="digest" || type=="digest-mac")
        {
            ec=cryptFileStamp.checkStampDigest();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
        }
        // check mac
        if (type=="mac" || type=="digest-mac")
        {
            ec=cryptFileStamp.verifyStampMAC();
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_CHECK(!ec);
        }

        // check open file
        ec=cryptFileStamp.open(CryptFile::Mode::scan);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_CHECK(!ec);

        // check digest
        if (type=="digest" || type=="digest-mac")
        {
            // failed on open file
            ec=cryptFileStamp.checkStampDigest();
            BOOST_CHECK(ec);
        }
        // check mac
        if (type=="mac" || type=="digest-mac")
        {
            // failed on open file
            ec=cryptFileStamp.verifyStampMAC();
            BOOST_CHECK(ec);
        }
        // close file
        cryptFileStamp.close(ec);
        BOOST_CHECK(!ec);

        // invalid key
        auto masterKey2=plugin->createPassphraseKey();
        HATN_REQUIRE(masterKey2);
        auto gen=plugin->createPasswordGenerator();
        HATN_REQUIRE(gen);
        ec=masterKey2->generatePassword(gen.get());
        BOOST_CHECK(!ec);
        CryptFile cryptFileStamp1(masterKey2.get());
        cryptFileStamp1.setFilename(tmpFileStamp);
        // check digest
        if (type=="digest" || type=="digest-mac")
        {
            ec=cryptFileStamp1.checkStampDigest();
            BOOST_CHECK(!ec);
        }
        // check mac
        if (type=="mac" || type=="digest-mac")
        {
            ec=cryptFileStamp1.verifyStampMAC();
            BOOST_CHECK(ec);
        }

        // change content
        CryptFile cryptFileStamp2(masterKey.get());
        ec=cryptFileStamp2.open(tmpFileStamp,CryptFile::Mode::write_existing);
        BOOST_CHECK(!ec);
        auto size=cryptFileStamp2.size(ec);
        BOOST_CHECK(!ec);
        ec=cryptFileStamp2.seek(size/2);
        BOOST_CHECK(!ec);
        uint32_t data=0xaabbccdd;
        auto written=cryptFileStamp2.write(reinterpret_cast<char*>(&data),sizeof(data),ec);
        BOOST_CHECK(!ec);
        BOOST_CHECK_EQUAL(written,sizeof(data));
        // check digest
        if (type=="digest" || type=="digest-mac")
        {
            // failed on open file
            ec=cryptFileStamp2.checkStampDigest();
            BOOST_CHECK(ec);
        }
        // check mac
        if (type=="mac" || type=="digest-mac")
        {
            // failed on open file
            ec=cryptFileStamp2.verifyStampMAC();
            BOOST_CHECK(ec);
        }
        // close file
        cryptFileStamp2.close(ec);
        BOOST_CHECK(!ec);

        // check digest
        if (type=="digest" || type=="digest-mac")
        {
            // failed on changed file
            ec=cryptFileStamp2.checkStampDigest();
            BOOST_CHECK(ec);
        }
        // check mac
        if (type=="mac" || type=="digest-mac")
        {
            // failed on changed file
            ec=cryptFileStamp2.verifyStampMAC();
            BOOST_CHECK(ec);
        }

        // add digest to changed file
        if (type=="digest" || type=="digest-mac")
        {
            ec=cryptFileStamp2.stampDigest();
            BOOST_CHECK(!ec);
        }
        // add mac to changed file
        if (type=="mac" || type=="digest-mac")
        {
            ec=cryptFileStamp2.stampMAC();
            BOOST_CHECK(!ec);
        }
        // check digest
        if (type=="digest" || type=="digest-mac")
        {
            ec=cryptFileStamp2.checkStampDigest();
            BOOST_CHECK(!ec);
        }
        // check mac
        if (type=="mac" || type=="digest-mac")
        {
            ec=cryptFileStamp2.verifyStampMAC();
            BOOST_CHECK(!ec);
        }
#endif
#ifndef HATN_SAVE_TEST_FILES
        // remove tmp files
        std::ignore=FileUtils::remove(tmpFileStamp);
        std::ignore=FileUtils::remove(tmpFileNoStamp);
#endif
        BOOST_TEST_MESSAGE(fmt::format("Done stamp for {}",type));
    }

    // reset suites
    CipherSuitesGlobal::instance().reset();
}

BOOST_AUTO_TEST_CASE(CheckReadWrite)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            CipherSuitesGlobal::instance().reset();
            checkReadWrite(plugin,PluginList::assetsPath("crypt"));
            CipherSuitesGlobal::instance().reset();
            checkReadWrite(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            CipherSuitesGlobal::instance().reset();
        }
    );
}

static void checkAppend(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    // load suite from json
    auto cipherSuiteFile=fmt::format("{}/cryptcontainer-ciphersuite1.json",path);
    auto keyFile=fmt::format("{}/cryptfile-stamp-key.dat",path);

    if (!boost::filesystem::exists(cipherSuiteFile)
        ||
        !boost::filesystem::exists(keyFile)
        )
    {
        return;
    }

    ByteArray cipherSuiteJson;
    auto ec=cipherSuiteJson.loadFromFile(cipherSuiteFile);
    HATN_REQUIRE(!ec);
    auto suite=std::make_shared<CipherSuite>();
    ec=suite->loadFromJSON(cipherSuiteJson);
    HATN_REQUIRE(!ec);

    // add suite to table of suites
    CipherSuitesGlobal::instance().addSuite(suite);

    // set engine
    auto engine=std::make_shared<CryptEngine>(plugin.get());
    CipherSuitesGlobal::instance().setDefaultEngine(std::move(engine));

    // check AEAD algorithm
    const CryptAlgorithm* aeadAlg=nullptr;
    ec=suite->aeadAlgorithm(aeadAlg);
    if (ec)
    {
        return;
    }
    HATN_REQUIRE(aeadAlg);

    // check pbkdf algorithm
    const CryptAlgorithm* kdfAlg=nullptr;
    ec=suite->pbkdfAlgorithm(kdfAlg);
    if (ec)
    {
        return;
    }

    // load master key
    common::SharedPtr<SymmetricKey> masterKey;
    masterKey=plugin->createPassphraseKey();
    HATN_REQUIRE(masterKey);
    ec=masterKey->importFromFile(keyFile,ContainerFormat::RAW_PLAIN);
    HATN_REQUIRE(!ec)

    // init files
    auto plainTextFile0=fmt::format("{}/cryptfile-plaintext8.dat",path);
    ByteArray plaintext0;
    ec=plaintext0.loadFromFile(plainTextFile0);
    BOOST_REQUIRE(!ec);
    auto plainTextFile=fmt::format("{}/cryptfile-plaintext10.dat",path);
    auto cryptFilename1=fmt::format("{}/cryptfile-append1.dat",hatn::test::MultiThreadFixture::tmpPath());
    auto plainFilename1=fmt::format("{}/plainfile-append1.dat",hatn::test::MultiThreadFixture::tmpPath());

    auto run=[&](common::File::Mode mode)
    {
        // init plain file
        ec=FileUtils::remove(plainFilename1);
        BOOST_REQUIRE(!ec);
        ByteArray plaintext1;
        ec=plaintext1.loadFromFile(plainTextFile);
        BOOST_REQUIRE(!ec);
        PlainFile plainFile1;
        ec=plainFile1.open(plainFilename1,PlainFile::Mode::append);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);
        auto written=plainFile1.write(plaintext1.data(),plaintext1.size(),ec);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(written,plaintext1.size());

        // init crypt file
        ec=FileUtils::remove(cryptFilename1);
        BOOST_REQUIRE(!ec);
        CryptFile cryptFile1(masterKey.get(),suite.get());
        ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::append);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);
        written=cryptFile1.write(plaintext1.data(),plaintext1.size(),ec);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(written,plaintext1.size());
        BOOST_CHECK_EQUAL(cryptFile1.size(),plainFile1.size());

        plainFile1.close(ec);
        BOOST_REQUIRE(!ec);
        cryptFile1.close(ec);
        BOOST_REQUIRE(!ec);

        // append data
        PlainFile plainFile2;
        ec=plainFile2.open(plainFilename1,mode);
        BOOST_REQUIRE(!ec);
        written=plainFile2.write(plaintext0,ec);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(written,plaintext0.size());
        plainFile2.close();

        CryptFile cryptFile2(masterKey.get(),suite.get());
        ec=cryptFile2.open(cryptFilename1,mode);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);
        written=cryptFile2.write(plaintext0,ec);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(written,plaintext0.size());
        cryptFile2.close();

        // read and compare data
        ByteArray plainBuf1;
        ec=plainFile1.readAll(plainBuf1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);
        ByteArray cryptBuf1;
        ec=cryptFile1.readAll(cryptBuf1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(cryptBuf1.size(),plaintext1.size()+plaintext0.size());
        BOOST_CHECK(cryptBuf1==plainBuf1);

        ByteArray checkBuf1{plaintext1};
        checkBuf1.append(plaintext0);
        BOOST_CHECK(cryptBuf1==checkBuf1);
    };

    run(CryptFile::Mode::append);
    run(CryptFile::Mode::append_existing);

    // check append_existing with non existing file
    ec=FileUtils::remove(plainFilename1);
    BOOST_REQUIRE(!ec);
    PlainFile plainFile1;
    ec=plainFile1.open(plainFilename1,PlainFile::Mode::append_existing);
    BOOST_CHECK(ec);
    ec=FileUtils::remove(cryptFilename1);
    BOOST_REQUIRE(!ec);
    CryptFile cryptFile1(masterKey.get(),suite.get());
    ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::append_existing);
    BOOST_CHECK(ec);
}

BOOST_AUTO_TEST_CASE(CheckAppend)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            CipherSuitesGlobal::instance().reset();
            checkAppend(plugin,PluginList::assetsPath("crypt"));
            CipherSuitesGlobal::instance().reset();
            checkAppend(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            CipherSuitesGlobal::instance().reset();
        }
        );
}

BOOST_AUTO_TEST_CASE(CheckFileStamp)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            CipherSuitesGlobal::instance().reset();
            checkFileStamp(plugin,PluginList::assetsPath("crypt"));
            CipherSuitesGlobal::instance().reset();
            checkFileStamp(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            CipherSuitesGlobal::instance().reset();
        }
    );
}

static void checkTruncate(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    // load suite from json
    auto cipherSuiteFile=fmt::format("{}/cryptcontainer-ciphersuite1.json",path);
    auto keyFile=fmt::format("{}/cryptfile-stamp-key.dat",path);

    if (!boost::filesystem::exists(cipherSuiteFile)
        ||
        !boost::filesystem::exists(keyFile)
        )
    {
        return;
    }

    ByteArray cipherSuiteJson;
    auto ec=cipherSuiteJson.loadFromFile(cipherSuiteFile);
    HATN_REQUIRE(!ec);
    auto suite=std::make_shared<CipherSuite>();
    ec=suite->loadFromJSON(cipherSuiteJson);
    HATN_REQUIRE(!ec);

    // add suite to table of suites
    CipherSuitesGlobal::instance().addSuite(suite);

    // set engine
    auto engine=std::make_shared<CryptEngine>(plugin.get());
    CipherSuitesGlobal::instance().setDefaultEngine(std::move(engine));

    // check AEAD algorithm
    const CryptAlgorithm* aeadAlg=nullptr;
    ec=suite->aeadAlgorithm(aeadAlg);
    if (ec)
    {
        return;
    }
    HATN_REQUIRE(aeadAlg);

    // check pbkdf algorithm
    const CryptAlgorithm* kdfAlg=nullptr;
    ec=suite->pbkdfAlgorithm(kdfAlg);
    if (ec)
    {
        return;
    }

    // load master key
    common::SharedPtr<SymmetricKey> masterKey;
    masterKey=plugin->createPassphraseKey();
    HATN_REQUIRE(masterKey);
    ec=masterKey->importFromFile(keyFile,ContainerFormat::RAW_PLAIN);
    HATN_REQUIRE(!ec)

    // init crypt file
    auto cryptFilename1=fmt::format("{}/cryptfile-truncate1.dat",hatn::test::MultiThreadFixture::tmpPath());
    auto cryptFilename0=fmt::format("{}/cryptfile-truncate0.dat",hatn::test::MultiThreadFixture::tmpPath());
    auto plainFilename1=fmt::format("{}/plainfile-truncate1.dat",hatn::test::MultiThreadFixture::tmpPath());
    auto plainFilename2=fmt::format("{}/plainfile-truncate2.dat",hatn::test::MultiThreadFixture::tmpPath());
    ByteArray plaintext1;
    auto plainTextFile=fmt::format("{}/cryptfile-plaintext10.dat",path);
    ec=plaintext1.loadFromFile(plainTextFile);
    BOOST_CHECK(!ec);
    PlainFile plainFile1;
    ec=plainFile1.open(plainFilename1,PlainFile::Mode::write);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    plainFile1.write(plaintext1.data(),plaintext1.size(),ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);

    // check seek beyond eof in plain file
    BOOST_TEST_MESSAGE("Check seek beyond eof in plain file");
    PlainFile plainFile2;
    ec=plainFile2.open(plainFilename2,PlainFile::Mode::write);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    plainFile2.write(plaintext1.data(),plaintext1.size(),ec);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(plainFile2.size(),plaintext1.size());
    ec=plainFile2.seek(plaintext1.size()+10);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(plainFile2.size(),plaintext1.size());
    ByteArray barr0;
    barr0.resize(8);
    auto read0=plainFile2.read(barr0.data(),barr0.size(),ec);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(read0,0);
    std::array<char,8> arr0{10,11,12,13,14,15,16,17};
    auto written0=plainFile2.write(arr0.data(),arr0.size(),ec);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(written0,arr0.size());
    BOOST_CHECK_EQUAL(plainFile2.size(),plaintext1.size()+10+arr0.size());
    ec=plainFile2.seek(plaintext1.size());
    BOOST_REQUIRE(!ec);
    ByteArray arr1;
    arr1.resize(10+arr0.size());
    auto read1=plainFile2.read(arr1.data(),arr1.size(),ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(read1,arr1.size());
    for (size_t i=0;i<arr1.size();i++)
    {
        if (i>=10)
        {
            BOOST_CHECK_EQUAL(int(arr1[i]),int(arr0[i-10]));
        }
        else
        {
            BOOST_CHECK_EQUAL(int(arr1[i]),0);
        }
    }

    // check seek beyond eof in crypt file
    BOOST_TEST_MESSAGE("Check seek beyond eof in crypt file");
    CryptFile cryptFile0(masterKey.get(),suite.get());
    ec=cryptFile0.open(cryptFilename0,CryptFile::Mode::write);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    cryptFile0.write(plaintext1.data(),plaintext1.size(),ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile0.size(),plaintext1.size());
    // seek beyond eof
    ec=cryptFile0.seek(plaintext1.size()+10);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile0.size(),plaintext1.size());
    barr0.clear();
    barr0.resize(8);
    // try to read beyond wof
    read0=cryptFile0.read(barr0.data(),barr0.size(),ec);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(read0,0);
    // write beyond eof
    written0=cryptFile0.write(arr0.data(),arr0.size(),ec);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(written0,arr0.size());
    // new content is prev content + zeroed gap + written arr0
    BOOST_CHECK_EQUAL(cryptFile0.size(),plaintext1.size()+10+arr0.size());
    ec=cryptFile0.seek(plaintext1.size());
    BOOST_REQUIRE(!ec);
    arr1.clear();
    arr1.resize(10+arr0.size());
    read1=cryptFile0.read(arr1.data(),arr1.size(),ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(read1,arr1.size());
    for (size_t i=0;i<arr1.size();i++)
    {
        if (i>=10)
        {
            BOOST_CHECK_EQUAL(int(arr1[i]),int(arr0[i-10]));
        }
        else
        {
            BOOST_CHECK_EQUAL(int(arr1[i]),0);
        }
    }

#if 1
    CryptFile cryptFile1(masterKey.get(),suite.get());
    auto& processor=cryptFile1.processor();
    processor.setChunkMaxSize(4096);
    processor.setFirstChunkMaxSize(4096);
    ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::write);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    cryptFile1.write(plaintext1.data(),plaintext1.size(),ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile1.size(),plainFile1.size());

    // truncate same size
    auto size=cryptFile1.size();
    BOOST_REQUIRE_EQUAL(size,plainFile1.size());
    ec=plainFile1.truncate(size);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    ec=cryptFile1.truncate(size);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size,cryptFile1.size());
    BOOST_TEST_MESSAGE(fmt::format("fileSize={}, chunkMaxSize={}, firstChunkMaxSize={}",cryptFile1.size(),processor.chunkMaxSize(),processor.firstChunkMaxSize()));

    // try to truncate last chunk with expected failure
    BOOST_TEST_MESSAGE("Try to truncate last chunk with expected failure");
    auto pos1=cryptFile1.pos();
    auto newSize1=size-10;
    ec=cryptFile1.truncateImpl(newSize1,true,true);
    BOOST_CHECK(ec);
    BOOST_CHECK_EQUAL(cryptFile1.size(),plainFile1.size());
    ByteArray tmpBuf1;
    ec=cryptFile1.readAll(tmpBuf1);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(tmpBuf1==plaintext1);
    ec=cryptFile1.seek(pos1);
    BOOST_REQUIRE(!ec);

    // truncate from the last chunk
    BOOST_TEST_MESSAGE("Truncate from the last chunk");
    ec=plainFile1.truncate(newSize1);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize1);
    ec=cryptFile1.truncate(newSize1);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),newSize1);
    cryptFile1.close(ec);
    BOOST_REQUIRE(!ec);

    // reopen file and check content
    CryptFile cryptFile2(masterKey.get(),suite.get());
    ec=cryptFile2.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile2.size(),plainFile1.size());
    ByteArray plainBuf2;
    ec=plainFile1.readAll(plainBuf2);
    BOOST_REQUIRE(!ec);
    ByteArray decryptBuf2;
    ec=cryptFile2.readAll(decryptBuf2);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(decryptBuf2==plainBuf2);
    cryptFile2.close(ec);
    BOOST_REQUIRE(!ec);

    // reopen file in write_existing mode
    CryptFile cryptFile3(masterKey.get(),suite.get());
    ec=cryptFile3.open(cryptFilename1,CryptFile::Mode::write_existing);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    auto size2=cryptFile3.size();
    cryptFile3.close(ec);
    BOOST_REQUIRE(!ec);

    // truncate from the first chunk
    BOOST_TEST_MESSAGE("Truncate from intermediate chunk");
    auto newSize2=size2-3*processor.chunkMaxSize()-10;
    ec=plainFile1.truncate(newSize2);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize2);
    ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::write_existing);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    ec=cryptFile1.truncate(newSize2);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),newSize2);
    cryptFile1.close(ec);
    BOOST_REQUIRE(!ec);

    // reopen file and check content
    ec=cryptFile2.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile2.size(),plainFile1.size());
    ec=plainFile1.readAll(plainBuf2);
    BOOST_REQUIRE(!ec);
    ec=cryptFile2.readAll(decryptBuf2);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(decryptBuf2==plainBuf2);
    cryptFile2.close(ec);
    BOOST_REQUIRE(!ec);

    // truncate from a chunk boundary
    BOOST_TEST_MESSAGE("Truncate from chunk boundary");
    auto pos3=8*processor.chunkMaxSize()+100;
    auto newSize3=10*processor.chunkMaxSize();
    ec=plainFile1.seek(pos3);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(plainFile1.pos(),pos3);
    ec=plainFile1.truncate(newSize3);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize3);
    ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::write_existing);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    ec=cryptFile1.seek(pos3);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(cryptFile1.pos(),pos3);
    ec=cryptFile1.truncate(newSize3);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),newSize3);
    BOOST_CHECK_EQUAL(cryptFile1.pos(),pos3);
    cryptFile1.close(ec);
    BOOST_REQUIRE(!ec);

    // reopen file and check content
    ec=cryptFile2.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile3.size(),plainFile1.size());
    ec=plainFile1.readAll(plainBuf2);
    BOOST_REQUIRE(!ec);
    ec=cryptFile2.readAll(decryptBuf2);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(decryptBuf2==plainBuf2);
    cryptFile2.close(ec);
    BOOST_REQUIRE(!ec);

    // truncate from the first chunk
    BOOST_TEST_MESSAGE("Truncate from the first chunk");
    auto newSize4=processor.chunkMaxSize()-100;
    ec=plainFile1.truncate(newSize4);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize4);
    ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::write_existing);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    ec=cryptFile1.truncate(newSize4);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),newSize4);
    cryptFile1.close(ec);
    BOOST_REQUIRE(!ec);

    // reopen file and check content
    ec=cryptFile2.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile3.size(),plainFile1.size());
    ec=plainFile1.readAll(plainBuf2);
    BOOST_REQUIRE(!ec);
    ec=cryptFile2.readAll(decryptBuf2);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(decryptBuf2==plainBuf2);
    cryptFile2.close(ec);
    BOOST_REQUIRE(!ec);

    // truncate to zero
    BOOST_TEST_MESSAGE("Truncate to zero");
    auto newSize5=0;
    auto oldSize5=plainFile1.size();
    auto pos5=plainFile1.pos();
    BOOST_TEST_MESSAGE(fmt::format("Before truncating pos={}",pos5));
    ec=plainFile1.truncate(newSize5);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(plainFile1.pos(),pos5);
    // write to truncated file
    uint32_t ab=1516;
    auto writtenSize=plainFile1.write(reinterpret_cast<const char*>(&ab),sizeof(ab),ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),oldSize5+sizeof(ab));
    BOOST_REQUIRE_EQUAL(plainFile1.pos(),pos5+sizeof(ab));
    BOOST_CHECK_EQUAL(writtenSize,sizeof(ab));
    BOOST_REQUIRE(!ec);
    // truncate again
    ec=plainFile1.truncate(newSize5);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(plainFile1.pos(),pos5+sizeof(ab));
    // read from truncated file
    ec=plainFile1.seek(pos5);
    BOOST_REQUIRE(!ec);
    ec=plainFile1.truncate(newSize5);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(plainFile1.pos(),pos5);
    uint32_t ab5=2829;
    auto readSize=plainFile1.read(reinterpret_cast<char*>(&ab5),sizeof(ab5),ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(plainFile1.pos(),pos5);
    BOOST_REQUIRE_EQUAL(readSize,0);
    BOOST_CHECK_EQUAL(uint32_t(ab5),uint32_t(2829));
    uint64_t abab5=28293031;
    readSize=plainFile1.read(reinterpret_cast<char*>(&abab5),sizeof(abab5),ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(plainFile1.pos(),pos5);
    BOOST_REQUIRE_EQUAL(readSize,0);

    // truncate to zero cryptfile
    ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::write_existing);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    ec=cryptFile1.seek(pos5);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.pos(),pos5);
    oldSize5=cryptFile1.size();
    ec=cryptFile1.truncate(newSize5);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(cryptFile1.pos(),pos5);
    // write to truncated file
    ab=1516;
    writtenSize=cryptFile1.write(reinterpret_cast<const char*>(&ab),sizeof(ab),ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),oldSize5+sizeof(ab));
    BOOST_REQUIRE_EQUAL(cryptFile1.pos(),pos5+sizeof(ab));
    BOOST_CHECK_EQUAL(writtenSize,sizeof(ab));
    BOOST_REQUIRE(!ec);
    // truncate again
    ec=cryptFile1.truncate(newSize5);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(cryptFile1.pos(),pos5+sizeof(ab));
    // read from truncated file
    ec=cryptFile1.seek(pos5);
    BOOST_REQUIRE(!ec);
    ec=cryptFile1.truncate(newSize5);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(cryptFile1.pos(),pos5);
    ab5=2829;
    readSize=cryptFile1.read(reinterpret_cast<char*>(&ab5),sizeof(ab5),ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(cryptFile1.pos(),pos5);
    BOOST_REQUIRE_EQUAL(readSize,0);
    BOOST_CHECK_EQUAL(uint32_t(ab5),uint32_t(2829));
    abab5=28293031;
    readSize=cryptFile1.read(reinterpret_cast<char*>(&abab5),sizeof(abab5),ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),newSize5);
    BOOST_REQUIRE_EQUAL(cryptFile1.pos(),pos5);
    BOOST_REQUIRE_EQUAL(readSize,0);
    cryptFile1.close(ec);
    BOOST_REQUIRE(!ec);

    // reopen file and check size
    ec=cryptFile2.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile2.size(),0);
    cryptFile2.close(ec);
    BOOST_REQUIRE(!ec);

    // refill files
    ec=plainFile1.seek(0);
    BOOST_REQUIRE(!ec);
    plainFile1.write(plaintext1.data(),plaintext1.size(),ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),plaintext1.size());
    ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::write);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    cryptFile1.write(plaintext1.data(),plaintext1.size(),ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile1.size(),plainFile1.size());

    // increase size
    auto pos6=cryptFile1.pos();
    BOOST_CHECK_EQUAL(pos6,plainFile1.size());
    BOOST_CHECK_EQUAL(pos6,plainFile1.pos());
    auto newSize6=cryptFile1.size()+cryptFile1.maxProcessingSize()*2+100;
    ec=plainFile1.truncate(newSize6);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(plainFile1.size(),newSize6);
    BOOST_CHECK_EQUAL(plainFile1.pos(),pos6);
    ec=cryptFile1.truncate(newSize6);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(cryptFile1.size(),plainFile1.size());
    BOOST_CHECK_EQUAL(cryptFile1.pos(),pos6);
    cryptFile1.close(ec);
    BOOST_REQUIRE(!ec);

    // reopen file and check content
    ec=cryptFile2.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile3.size(),plainFile1.size());
    ec=plainFile1.readAll(plainBuf2);
    BOOST_REQUIRE(!ec);
    ec=cryptFile2.readAll(decryptBuf2);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(decryptBuf2==plainBuf2);
    cryptFile2.close(ec);
    BOOST_REQUIRE(!ec);
#endif
}

BOOST_AUTO_TEST_CASE(CheckTruncate)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            CipherSuitesGlobal::instance().reset();
            checkTruncate(plugin,PluginList::assetsPath("crypt"));
            CipherSuitesGlobal::instance().reset();
            checkTruncate(plugin,PluginList::assetsPath("crypt",plugin->info()->name));
            CipherSuitesGlobal::instance().reset();
        }
    );
}

//! @todo Add fuzzy tests of cryptfile

BOOST_AUTO_TEST_SUITE_END()

}
}
