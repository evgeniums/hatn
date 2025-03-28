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

BOOST_FIXTURE_TEST_SUITE(TestStreamFile,CryptTestFixture)

static void checkAppend(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    // load suite from json
    auto cipherSuiteFile=fmt::format("{}/streamfile-ciphersuite1.json",path);
    auto keyFile=fmt::format("{}/streamfile-key.dat",path);
    auto assetsFilename1=fmt::format("{}/streamfile-crypt1.dat",path);

    if (!boost::filesystem::exists(cipherSuiteFile)
        ||
        !boost::filesystem::exists(keyFile)
#ifndef HATN_SAVE_TEST_FILES
        ||
        !boost::filesystem::exists(assetsFilename1)
#endif
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

    // check cipher algorithm
    const CryptAlgorithm* streamCipherAlg=nullptr;
    ec=suite->cipherAlgorithm(streamCipherAlg);
    if (ec)
    {
        return;
    }
    HATN_REQUIRE(streamCipherAlg);

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

    auto cryptFilename1=fmt::format("{}/streamfile-crypt1.dat",hatn::test::MultiThreadFixture::tmpPath());
    auto cryptFilename2=fmt::format("{}/streamfile-crypt2.dat",hatn::test::MultiThreadFixture::tmpPath());
    std::ignore=FileUtils::remove(cryptFilename1);
    std::ignore=FileUtils::remove(cryptFilename2);

    // init plain data
    ByteArray plaintext1;
    auto plainTextFile1=fmt::format("{}/streamfile-plaintext1.dat",path);
    ec=plaintext1.loadFromFile(plainTextFile1);
    BOOST_CHECK(!ec);
    ByteArray plaintext2;
    auto plainTextFile2=fmt::format("{}/streamfile-plaintext2.dat",path);
    ec=plaintext2.loadFromFile(plainTextFile2);
    BOOST_CHECK(!ec);

    // init crypt file    
    CryptFile cryptFile1(masterKey.get(),suite.get());
    BOOST_CHECK(!cryptFile1.isStreamingMode());
    cryptFile1.setStreamingMode(true);
    BOOST_CHECK(cryptFile1.isStreamingMode());

    // try to open in invalid mode
    ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::write);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Expected error: {}",ec.message()));
    }
    BOOST_REQUIRE(ec);

    // open in valid mode
    ec=cryptFile1.open(cryptFilename1,CryptFile::Mode::append);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);

    // write whole data
    BOOST_TEST_MESSAGE("Write whole data 1");
    auto written=cryptFile1.write(plaintext1.data(),plaintext1.size(),ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(written,plaintext1.size());

    // check written size
    BOOST_CHECK_EQUAL(cryptFile1.size(),plaintext1.size());
    cryptFile1.close(ec);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile1.size(),plaintext1.size());

    // read whole data
    BOOST_TEST_MESSAGE("Read whole data 1");
    CryptFile cryptFile2(masterKey.get(),suite.get());
    ec=cryptFile2.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(cryptFile2.isStreamingMode());
    ByteArray buf2;
    ec=cryptFile2.readAll(buf2);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(buf2.size(),plaintext1.size());
    BOOST_CHECK(buf2==plaintext1);
    cryptFile2.close(ec);
    BOOST_REQUIRE(!ec);

    // read from  second chunk
    BOOST_TEST_MESSAGE("Read from second chunk");
    CryptFile cryptFile2_(masterKey.get(),suite.get());
    ec=cryptFile2_.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(cryptFile2_.isStreamingMode());
    size_t offset2_=cryptFile2_.processor().firstChunkMaxSize()+100;
    size_t size2_=1000;
    ByteArray buf2_;
    ec=cryptFile2_.seek(offset2_);
    BOOST_REQUIRE(!ec);
    buf2_.resize(size2_);
    auto read2_=cryptFile2_.read(buf2_.data(),buf2_.size(),ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(read2_,buf2_.size());
    bool eq2_=lib::string_view{buf2_.data(),buf2_.size()}==lib::string_view{plaintext1.data()+offset2_,size2_};
    BOOST_CHECK(eq2_);
    cryptFile2_.close(ec);
    BOOST_REQUIRE(!ec);

    // open for append
    CryptFile cryptFile3(masterKey.get(),suite.get());
    BOOST_TEST_MESSAGE("Open for append 2");
    ec=cryptFile3.open(cryptFilename1,CryptFile::Mode::append);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(cryptFile3.isStreamingMode());
    size_t appended=0;
    BOOST_TEST_MESSAGE("Append data 2");
    while (appended<plaintext2.size())
    {
        auto diff=plaintext2.size()-appended;
        size_t writeSize=diff/2 + (diff&0x1);
        if (writeSize==0)
        {
            writeSize=1;
        }
        if ((writeSize+appended)>plaintext2.size())
        {
            writeSize=plaintext2.size()-appended;
        }
#if 0
        BOOST_TEST_MESSAGE(fmt::format("Append writeSize={} appended={} plaintext2.size()={}",writeSize,appended,plaintext2.size()));
#endif
        written=cryptFile3.write(plaintext2.data()+appended,writeSize,ec);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Failed to append: writeSize={} appended={} plaintext2.size()={} ec={}",writeSize,appended,plaintext2.size(),ec.message()));
        }
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(written,writeSize);
        appended+=written;
    }
    BOOST_CHECK_EQUAL(cryptFile3.size(),plaintext1.size()+plaintext2.size());
    cryptFile3.close(ec);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile3.size(),plaintext1.size()+plaintext2.size());

    // read whole data again
    CryptFile cryptFile4(masterKey.get(),suite.get());
    BOOST_TEST_MESSAGE("Open for read 2");
    ec=cryptFile4.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(cryptFile4.isStreamingMode());
    ByteArray buf4;
    BOOST_TEST_MESSAGE("Read whole 2");
    ec=cryptFile4.readAll(buf4);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(buf4.size(),plaintext1.size()+plaintext2.size());
    ByteArray sample4;
    sample4.append(plaintext1);
    sample4.append(plaintext2);
    BOOST_REQUIRE_EQUAL(buf4.size(),sample4.size());
    for (size_t i=0;i<buf4.size();i++)
    {
        if (buf4[i]!=sample4[i])
        {
            BOOST_FAIL(fmt::format("Data mismatch at position {}",i));
        }
    }
    BOOST_CHECK(buf4==sample4);
    cryptFile4.close(ec);
    BOOST_REQUIRE(!ec);

    // append by small pieces
    CryptFile cryptFile5(masterKey.get(),suite.get());
    BOOST_TEST_MESSAGE("Append by small pieces");
    ec=cryptFile5.open(cryptFilename1,CryptFile::Mode::append_existing);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(cryptFile5.isStreamingMode());
    appended=0;
    BOOST_TEST_MESSAGE("Append data by small piecies");
    size_t i=0;
    while (appended<plaintext2.size())
    {
        size_t writeSize=90+i%90;
        if ((writeSize+appended)>plaintext2.size())
        {
            writeSize=plaintext2.size()-appended;
        }
        i++;
#if 0
        BOOST_TEST_MESSAGE(fmt::format("Append writeSize={} appended={} plaintext2.size()={}",writeSize,appended,plaintext2.size()));
#endif
        written=cryptFile5.write(plaintext2.data()+appended,writeSize,ec);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Failed to append: writeSize={} appended={} plaintext2.size()={} ec={}",writeSize,appended,plaintext2.size(),ec.message()));
        }
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(written,writeSize);
        appended+=written;
        std::ignore=cryptFile5.flush(false);
    }
    BOOST_CHECK_EQUAL(cryptFile5.size(),plaintext1.size()+2*plaintext2.size());
    cryptFile5.close(ec);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile5.size(),plaintext1.size()+2*plaintext2.size());

    // read whole data again
    CryptFile cryptFile6(masterKey.get(),suite.get());
    BOOST_TEST_MESSAGE("Open for read 3");
    ec=cryptFile6.open(cryptFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(cryptFile6.isStreamingMode());
    ByteArray buf6;
    BOOST_TEST_MESSAGE("Read whole 3");
    ec=cryptFile6.readAll(buf6);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(buf6.size(),plaintext1.size()+2*plaintext2.size());
    ByteArray sample6;
    sample6.append(plaintext1);
    sample6.append(plaintext2);
    sample6.append(plaintext2);
    BOOST_REQUIRE_EQUAL(buf6.size(),sample6.size());
    BOOST_CHECK(buf6==sample6);
    cryptFile6.close(ec);
    BOOST_REQUIRE(!ec);

#ifdef HATN_SAVE_TEST_FILES
    FileUtils::copy(cryptFilename1,assetsFilename1);
#endif

    // decrypt and check file from assets
    CryptFile cryptFile7(masterKey.get(),suite.get());
    BOOST_TEST_MESSAGE("Open for read from assets");
    ec=cryptFile7.open(assetsFilename1,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(cryptFile7.isStreamingMode());
    ByteArray buf7;
    BOOST_TEST_MESSAGE("Read from assets");
    ec=cryptFile7.readAll(buf7);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(buf7==buf6);
    cryptFile7.close(ec);
    BOOST_REQUIRE(!ec);

    // write to first chunk only
    BOOST_TEST_MESSAGE("Open for write to first chunk");
    CryptFile cryptFile8(masterKey.get(),suite.get());
    cryptFile8.setStreamingMode(true);
    BOOST_CHECK(cryptFile8.isStreamingMode());
    ec=cryptFile8.open(cryptFilename2,CryptFile::Mode::append);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    ByteArray buf8;
    std::ignore=CipherSuitesGlobal::instance().defaultRandomGenerator()->randContainer(buf8,500,100);
    BOOST_TEST_MESSAGE(fmt::format("Write to first chunk bufSize={}",buf8.size()));
    written=0;
    for (size_t i=0;i<100;i++)
    {
        auto writeSize=i+i%10;
        if (writeSize>(buf8.size()-written))
        {
            writeSize=(buf8.size()-written);
        }
        written+=cryptFile8.write(buf8.data()+written,writeSize,ec);
        BOOST_REQUIRE(!ec);
        if (written==buf8.size())
        {
            break;
        }
    }
    BOOST_CHECK_EQUAL(cryptFile8.size(),buf8.size());
    cryptFile8.close(ec);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(cryptFile8.size(),buf8.size());

    CryptFile cryptFile9(masterKey.get(),suite.get());
    BOOST_TEST_MESSAGE("Open for read for check first chunk");
    ec=cryptFile9.open(cryptFilename2,CryptFile::Mode::scan);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(cryptFile9.isStreamingMode());
    ByteArray buf9;
    BOOST_TEST_MESSAGE("Read from first chunk");
    ec=cryptFile9.readAll(buf9);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);
    BOOST_CHECK(buf9==buf8);
    cryptFile9.close(ec);
    BOOST_REQUIRE(!ec);

    //! @todo Check reading with invalid key
    //! @todo Check writing/reading to exact chunk boundaries
}

BOOST_AUTO_TEST_CASE(CheckStreamFile)
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

BOOST_AUTO_TEST_SUITE_END()

}
}
