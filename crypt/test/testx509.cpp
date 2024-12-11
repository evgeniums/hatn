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
#include <hatn/common/fileutils.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/x509certificate.h>
#include <hatn/crypt/x509certificatestore.h>
#include <hatn/crypt/x509certificatechain.h>
#include <hatn/crypt/ciphersuite.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

//#define TEST_X509_RESULT_SAVE
//#define TEST_X509_PRINT_CONTENT

BOOST_FIXTURE_TEST_SUITE(TestX509,CryptTestFixture)

static void checkAlg(
        const std::function<void (std::shared_ptr<CryptPlugin>&,
                            const std::shared_ptr<CipherSuite>&,
                            const std::string&,
                            const std::string&)>& handler
    )
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [handler](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::X509)
                    &&
                plugin->isFeatureImplemented(Crypt::Feature::Signature)
               )
            {
                auto eachPath=[&plugin,handler](const std::string& path)
                {
                    auto eachLine=[&plugin,handler,&path](const std::string& algName)
                    {
                        auto algPathName=algName;
                        boost::algorithm::replace_all(algPathName,std::string("/"),std::string("-"));
                        std::string prefix=fmt::format("{}/x509/x509-{}",path,algPathName);

                        const CryptAlgorithm* alg=nullptr;
                        auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::SIGNATURE,algName);
                        if (!ec && alg)
                        {
                            std::string suiteFile=fmt::format("{}-suite.json",prefix);
                            auto suite=std::make_shared<CipherSuite>();
                            auto ec=suite->loadFromFile(suiteFile);
                            BOOST_CHECK(!ec);
                            CipherSuites::instance().addSuite(suite);
                            auto engine=std::make_shared<CryptEngine>(plugin.get());
                            CipherSuites::instance().setDefaultEngine(std::move(engine));

                            handler(plugin,suite,algName,prefix);

                            CipherSuites::instance().reset();
                        }
                    };
                    std::string fileName=fmt::format("{}/x509-algs.txt",path);
                    if (boost::filesystem::exists(fileName))
                    {
                        PluginList::eachLinefromFile(fileName,eachLine);
                    }
                };
                eachPath(PluginList::assetsPath("crypt"));
                eachPath(PluginList::assetsPath("crypt",plugin->info()->name));
            }
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckX509ExportImport)
{
    auto algHandler=[](std::shared_ptr<CryptPlugin>& plugin,const std::shared_ptr<CipherSuite>& suite,const std::string& algName,const std::string& pathPrefix)
    {
        BOOST_TEST_MESSAGE(fmt::format("Testing with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));

        Error ec;
        auto ca3Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca3Pem);
        std::string ca3PemFile=fmt::format("{}-ca3.pem",pathPrefix);
        ec=ca3Pem->loadFromFile(ca3PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);
        auto ca3Der=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca3Der);
        std::string ca3DerFile=fmt::format("{}-ca3.der",pathPrefix);
        ec=ca3Der->loadFromFile(ca3DerFile,ContainerFormat::DER);
        BOOST_CHECK(!ec);
        BOOST_CHECK(*ca3Pem==*ca3Der);

        auto ca1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca1Pem);
        std::string ca1PemFile=fmt::format("{}-ca1.pem",pathPrefix);
        ec=ca1Pem->loadFromFile(ca1PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);
        BOOST_CHECK(*ca1Pem!=*ca3Pem);

        HATN_REQUIRE(!ec);
        auto ca3Pem1=suite->createX509Certificate(ec);
        HATN_REQUIRE(ca3Pem1);
        auto tmpFile=fmt::format("{}/x509.pem",hatn::test::MultiThreadFixture::tmpPath());
        ec=ca3Pem->saveToFile(tmpFile,ContainerFormat::PEM);
        HATN_REQUIRE(!ec);
        ec=ca3Pem1->loadFromFile(tmpFile,ContainerFormat::PEM,true);
        BOOST_CHECK(!ec);
        BOOST_CHECK(*ca3Pem==*ca3Pem1);
        std::ignore=FileUtils::remove(tmpFile);

        auto ca3Der1=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca3Der1);
        tmpFile=fmt::format("{}/x509.der",hatn::test::MultiThreadFixture::tmpPath());
        ec=ca3Der->saveToFile(tmpFile,ContainerFormat::DER);
        HATN_REQUIRE(!ec);
        ec=ca3Der1->loadFromFile(tmpFile,ContainerFormat::DER,true);
        BOOST_CHECK(!ec);
        BOOST_CHECK(*ca3Der==*ca3Der1);
        std::ignore=FileUtils::remove(tmpFile);

        BOOST_TEST_MESSAGE(fmt::format("Done with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));
    };
    checkAlg(algHandler);
}

BOOST_AUTO_TEST_CASE(CheckX509Fields)
{
    auto algHandler=[](std::shared_ptr<CryptPlugin>& plugin,const std::shared_ptr<CipherSuite>& suite,const std::string& algName,const std::string& pathPrefix)
    {
        BOOST_TEST_MESSAGE(fmt::format("Testing with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));

        Error ec;
        auto client1_2_2Der=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(client1_2_2Der);
        std::string client1_2_2DerFile=fmt::format("{}-client1_2_2.der",pathPrefix);
        ec=client1_2_2Der->loadFromFile(client1_2_2DerFile,ContainerFormat::DER);
        HATN_REQUIRE(!ec);

        auto json1=client1_2_2Der->toString(true,true);
        std::string client1_2_2CheckFile1=fmt::format("{}-client1_2_2-pretty.json",pathPrefix);

#ifdef TEST_X509_PRINT_CONTENT
        BOOST_TEST_MESSAGE(fmt::format("============\n{}\n============",json1));
#endif
#ifdef TEST_X509_RESULT_SAVE
        ByteArray buf1(json1.c_str());
        ec=buf1.saveToFile(client1_2_2CheckFile1);
        HATN_REQUIRE(!ec);
#endif
        ByteArray check1;
        ec=check1.loadFromFile(client1_2_2CheckFile1);
        HATN_REQUIRE(!ec);
        std::string check1Str(check1.c_str());
        boost::algorithm::replace_all(check1Str,std::string("\r"),std::string(""));
#ifdef TEST_X509_PRINT_CONTENT
        BOOST_TEST_MESSAGE(fmt::format("============\n{}\n============",check1.c_str()));
#endif
        BOOST_CHECK_EQUAL(json1,check1Str);

        auto json2=client1_2_2Der->toString();
        std::string client1_2_2CheckFile2=fmt::format("{}-client1_2_2-compact.json",pathPrefix);
#ifdef TEST_X509_PRINT_CONTENT
        BOOST_TEST_MESSAGE(fmt::format("============\n{}\n============",json2));
#endif
#ifdef TEST_X509_RESULT_SAVE
        ByteArray buf2(json2.c_str());
        ec=buf2.saveToFile(client1_2_2CheckFile2);
        HATN_REQUIRE(!ec);
#endif
        ByteArray check2;
        ec=check2.loadFromFile(client1_2_2CheckFile2);
        HATN_REQUIRE(!ec);
        std::string check2Str(check2.c_str());
        boost::algorithm::replace_all(check2Str,std::string("\r"),std::string(""));
#ifdef TEST_X509_PRINT_CONTENT
        BOOST_TEST_MESSAGE(fmt::format("============\n{}\n============",check2Str));
#endif
        BOOST_CHECK_EQUAL(json2,check2Str);

        ec=client1_2_2Der->isDateValid();
        BOOST_CHECK(!ec);
        std::tm tm = {};
        std::stringstream ss("Jan 1 2010 10:20:30");
        ss >> std::get_time(&tm, "%b %d %Y %H:%M:%S");
        auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        ec=client1_2_2Der->isDateValid(tp);
        BOOST_CHECK(ec);
        std::stringstream ss1("Jan 1 2100 10:20:30");
        ss1 >> std::get_time(&tm, "%b %d %Y %H:%M:%S");
        tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        ec=client1_2_2Der->isDateValid(tp);
        BOOST_CHECK(ec);

        BOOST_TEST_MESSAGE(fmt::format("Done with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));
    };
    checkAlg(algHandler);
}

BOOST_AUTO_TEST_CASE(CheckX509VerifyCA)
{
    auto algHandler=[](std::shared_ptr<CryptPlugin>& plugin,const std::shared_ptr<CipherSuite>& suite,const std::string& algName,const std::string& pathPrefix)
    {
        BOOST_TEST_MESSAGE(fmt::format("Testing with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));

        Error ec;
        auto ca1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca1Pem);
        std::string ca1PemFile=fmt::format("{}-ca1.pem",pathPrefix);
        ec=ca1Pem->loadFromFile(ca1PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);

        auto im1_1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(im1_1Pem);
        std::string im1_1PemFile=fmt::format("{}-im1_1.pem",pathPrefix);
        ec=im1_1Pem->loadFromFile(im1_1PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);
        ec=im1_1Pem->verify(*ca1Pem);
        BOOST_CHECK(!ec);
        ec=im1_1Pem->checkIssuedBy(*ca1Pem);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_CHECK(!ec);

        auto im1_2Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(im1_2Pem);
        std::string im1_2PemFile=fmt::format("{}-im1_2.pem",pathPrefix);
        ec=im1_2Pem->loadFromFile(im1_2PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);
        ec=im1_2Pem->verify(*im1_1Pem);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_CHECK(!ec);
        ec=im1_2Pem->checkIssuedBy(*im1_1Pem);
        BOOST_CHECK(!ec);

        auto client1_2_2Der=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(client1_2_2Der);
        std::string client1_2_2DerFile=fmt::format("{}-client1_2_2.der",pathPrefix);
        ec=client1_2_2Der->loadFromFile(client1_2_2DerFile,ContainerFormat::DER);
        HATN_REQUIRE(!ec);
        ec=client1_2_2Der->verify(*im1_2Pem);
        BOOST_CHECK(!ec);
        ec=client1_2_2Der->checkIssuedBy(*im1_2Pem);
        BOOST_CHECK(!ec);

        ec=client1_2_2Der->checkIssuedBy(*ca1Pem);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected failure: {}",ec.message()));
        }
        BOOST_CHECK(ec);
        ec=client1_2_2Der->verify(*ca1Pem);
        BOOST_CHECK(ec);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected failure: {}",ec.message()));
        }
        BOOST_CHECK(ec.native());

//! @todo Fix native crypt error
#if 0
        auto verifyError=dynamic_cast<X509VerifyError*>(ec.native().get());
        HATN_REQUIRE(verifyError);
        HATN_REQUIRE(verifyError->certificate());
        BOOST_CHECK(*client1_2_2Der==*verifyError->certificate());
#endif
        ec=client1_2_2Der->checkIssuedBy(*im1_1Pem);
        BOOST_CHECK(ec);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected failure: {}",ec.message()));
        }
        ec=client1_2_2Der->verify(*im1_1Pem);
        BOOST_CHECK(ec);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected failure: {}",ec.message()));
        }

        BOOST_TEST_MESSAGE(fmt::format("Done with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));
    };
    checkAlg(algHandler);
}

BOOST_AUTO_TEST_CASE(CheckX509VerifyStore)
{
    auto algHandler=[](std::shared_ptr<CryptPlugin>& plugin,const std::shared_ptr<CipherSuite>& suite,const std::string& algName,const std::string& pathPrefix)
    {
        BOOST_TEST_MESSAGE(fmt::format("Testing with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));

        Error ec;

        // load certificates

        auto ca1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca1Pem);
        std::string ca1PemFile=fmt::format("{}-ca1.pem",pathPrefix);
        ec=ca1Pem->loadFromFile(ca1PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);

        auto ca2Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca2Pem);
        std::string ca2PemFile=fmt::format("{}-ca2.pem",pathPrefix);
        ec=ca2Pem->loadFromFile(ca2PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);
        BOOST_CHECK(*ca1Pem!=*ca2Pem);

        auto ca3Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca3Pem);
        std::string ca3PemFile=fmt::format("{}-ca3.pem",pathPrefix);
        ec=ca3Pem->loadFromFile(ca3PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);

        auto im1_1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(im1_1Pem);
        std::string im1_1PemFile=fmt::format("{}-im1_1.pem",pathPrefix);
        ec=im1_1Pem->loadFromFile(im1_1PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);
        ec=im1_1Pem->verify(*ca1Pem);
        BOOST_CHECK(!ec);
        ec=im1_1Pem->checkIssuedBy(*ca1Pem);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_CHECK(!ec);

        auto im1_2Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(im1_2Pem);
        std::string im1_2PemFile=fmt::format("{}-im1_2.pem",pathPrefix);
        ec=im1_2Pem->loadFromFile(im1_2PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);

        auto client1_2_1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(client1_2_1Pem);
        std::string client1_2_1PemFile=fmt::format("{}-client1_2_1.pem",pathPrefix);
        ec=client1_2_1Pem->loadFromFile(client1_2_1PemFile,ContainerFormat::PEM);
        HATN_REQUIRE(!ec);

        auto client2_1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(client2_1Pem);
        std::string client2_1PemFile=fmt::format("{}-client2_1.pem",pathPrefix);
        ec=client2_1Pem->loadFromFile(client2_1PemFile,ContainerFormat::PEM);
        HATN_REQUIRE(!ec);

        // test store with a single CA
        {
            auto store1=suite->createX509CertificateStore(ec);
            HATN_REQUIRE(!ec);
            HATN_REQUIRE(store1);
            ec=store1->addCertificate(*ca1Pem);
            BOOST_CHECK(!ec);
            ec=im1_1Pem->verify(*store1);
            BOOST_CHECK(!ec);
            ec=im1_2Pem->verify(*store1);
            BOOST_CHECK(ec);
        }

        // add certificate to the store from a file
        auto store2=suite->createX509CertificateStore(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(store2);
        ec=store2->addCertificate(ca1PemFile);
        BOOST_CHECK(!ec);
        ec=im1_1Pem->verify(*store2);
        BOOST_CHECK(!ec);
        ec=im1_2Pem->verify(*store2);
        BOOST_CHECK(ec);

        // multiple CA in the store
        ec=store2->addCertificate(*ca2Pem);
        BOOST_CHECK(!ec);
        ec=store2->addCertificate(*ca3Pem);
        BOOST_CHECK(!ec);
        ec=im1_1Pem->verify(*store2);
        BOOST_CHECK(!ec);
        ec=im1_2Pem->verify(*store2);
        BOOST_CHECK(ec);
        ec=client2_1Pem->verify(*store2);
        BOOST_CHECK(!ec);
        auto certs=store2->certificates(ec);
        BOOST_CHECK(!ec);
        BOOST_CHECK_EQUAL(certs.size(),3);
        size_t count=0;
        std::vector<SharedPtr<X509Certificate>> checkCerts{ca1Pem,ca2Pem,ca3Pem};
        for (size_t i=0;i<certs.size();i++)
        {
            for (size_t j=0;i<checkCerts.size();j++)
            {
                if (*certs[i]==*checkCerts[j])
                {
                    ++count;
                    break;
                }
            }
        }
        BOOST_CHECK_EQUAL(count,3);

        // test CA folder
        auto store3=suite->createX509CertificateStore(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(store3);
        std::string hashedFolder=fmt::format("{}-ca",pathPrefix);
        ec=store3->setCaFolder(hashedFolder);
        BOOST_CHECK(!ec);
        ec=im1_1Pem->verify(*store3);
        BOOST_CHECK(!ec);
        ec=im1_2Pem->verify(*store3);
        BOOST_CHECK(ec);
        ec=client2_1Pem->verify(*store3);
        BOOST_CHECK(!ec);

        BOOST_TEST_MESSAGE(fmt::format("Done with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));
    };
    checkAlg(algHandler);
}

BOOST_AUTO_TEST_CASE(CheckX509VerifyChain)
{
    auto algHandler=[](std::shared_ptr<CryptPlugin>& plugin,const std::shared_ptr<CipherSuite>& suite,const std::string& algName,const std::string& pathPrefix)
    {
        BOOST_TEST_MESSAGE(fmt::format("Testing with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));

        Error ec;

        // load certificates
        auto ca1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca1Pem);
        std::string ca1PemFile=fmt::format("{}-ca1.pem",pathPrefix);
        ec=ca1Pem->loadFromFile(ca1PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);

        auto ca2Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca2Pem);
        std::string ca2PemFile=fmt::format("{}-ca2.pem",pathPrefix);
        ec=ca2Pem->loadFromFile(ca2PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);
        BOOST_CHECK(*ca1Pem!=*ca2Pem);

        auto ca3Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca3Pem);
        std::string ca3PemFile=fmt::format("{}-ca3.pem",pathPrefix);
        ec=ca3Pem->loadFromFile(ca3PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);

        auto im1_1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(im1_1Pem);
        std::string im1_1PemFile=fmt::format("{}-im1_1.pem",pathPrefix);
        ec=im1_1Pem->loadFromFile(im1_1PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);
        ec=im1_1Pem->verify(*ca1Pem);
        BOOST_CHECK(!ec);
        ec=im1_1Pem->checkIssuedBy(*ca1Pem);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_CHECK(!ec);

        auto im1_2Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(im1_2Pem);
        std::string im1_2PemFile=fmt::format("{}-im1_2.pem",pathPrefix);
        ec=im1_2Pem->loadFromFile(im1_2PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);

        auto client1_2_1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(client1_2_1Pem);
        std::string client1_2_1PemFile=fmt::format("{}-client1_2_1.pem",pathPrefix);
        ec=client1_2_1Pem->loadFromFile(client1_2_1PemFile,ContainerFormat::PEM);
        HATN_REQUIRE(!ec);

        auto client1_2_2Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(client1_2_2Pem);
        std::string client1_2_2PemFile=fmt::format("{}-client1_2_2.pem",pathPrefix);
        ec=client1_2_2Pem->loadFromFile(client1_2_2PemFile,ContainerFormat::PEM);
        HATN_REQUIRE(!ec);

        auto client2_1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(client2_1Pem);
        std::string client2_1PemFile=fmt::format("{}-client2_1.pem",pathPrefix);
        ec=client2_1Pem->loadFromFile(client2_1PemFile,ContainerFormat::PEM);
        HATN_REQUIRE(!ec);

        // store with a single CA
        auto store1=suite->createX509CertificateStore(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(store1);
        ec=store1->addCertificate(*ca1Pem);

        // multiple CA in the store the store, some loaded from files
        auto store2=suite->createX509CertificateStore(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(store2);
        ec=store2->addCertificate(ca1PemFile);
        BOOST_CHECK(!ec);
        ec=store2->addCertificate(ca2PemFile);
        BOOST_CHECK(!ec);
        ec=store2->addCertificate(*ca3Pem);
        BOOST_CHECK(!ec);

        // chain with single certificate
        auto chain1=suite->createX509CertificateChain(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(chain1);
        ec=chain1->addCertificate(*im1_1Pem);
        BOOST_CHECK(!ec);
        ec=im1_2Pem->verify(*store1,chain1.get());
        BOOST_CHECK(!ec);
        ec=client1_2_1Pem->verify(*store1,chain1.get());
        BOOST_CHECK(ec);

        // second certificate in chain
        ec=chain1->addCertificate(*im1_2Pem);
        BOOST_CHECK(!ec);
        ec=im1_2Pem->verify(*store1,chain1.get());
        BOOST_CHECK(!ec);
        ec=client1_2_1Pem->verify(*store1,chain1.get());
        BOOST_CHECK(!ec);
        ec=client1_2_1Pem->verify(*store1);
        BOOST_CHECK(ec);
        ec=client1_2_2Pem->verify(*store1,chain1.get());
        BOOST_CHECK(!ec);
        ec=client2_1Pem->verify(*store1,chain1.get());
        BOOST_CHECK(ec);

        ec=client2_1Pem->verify(*store2,chain1.get());
        BOOST_CHECK(!ec);

        // check certificate list
        auto certs=chain1->certificates(ec);
        BOOST_CHECK(!ec);
        BOOST_CHECK_EQUAL(certs.size(),2);
        std::vector<SharedPtr<X509Certificate>> checkCerts{im1_1Pem,im1_2Pem};
        auto count=(std::min)(certs.size(),checkCerts.size());
        for (size_t i=0;i<count;i++)
        {
            BOOST_CHECK(*(certs[i])==*(checkCerts[i]));
            if (*(certs[i])!=*(checkCerts[i]))
            {
                break;
            }
        }

        // load list
        auto chain2=suite->createX509CertificateChain(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(chain2);
        ec=chain2->loadCerificates(checkCerts);
        BOOST_CHECK(!ec);
        ec=im1_2Pem->verify(*store1,chain2.get());
        BOOST_CHECK(!ec);
        ec=client1_2_1Pem->verify(*store1,chain2.get());
        BOOST_CHECK(!ec);
        ec=client1_2_1Pem->verify(*store1);
        BOOST_CHECK(ec);
        ec=client1_2_2Pem->verify(*store1,chain2.get());
        BOOST_CHECK(!ec);
        ec=client2_1Pem->verify(*store1,chain2.get());
        BOOST_CHECK(ec);

        // check import
        auto chain3=suite->createX509CertificateChain(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(chain3);
        std::string chainPemFile=fmt::format("{}-chain.pem",pathPrefix);
        ec=chain3->loadFromFile(chainPemFile);
        HATN_REQUIRE(!ec);
        ec=im1_2Pem->verify(*store1,chain3.get());
        BOOST_CHECK(!ec);
        ec=client1_2_1Pem->verify(*store1,chain3.get());
        BOOST_CHECK(!ec);
        ec=client1_2_1Pem->verify(*store1);
        BOOST_CHECK(ec);
        ec=client1_2_2Pem->verify(*store1,chain3.get());
        BOOST_CHECK(!ec);
        ec=client2_1Pem->verify(*store1,chain3.get());
        BOOST_CHECK(ec);

        // check export/import
        auto tmpFile=fmt::format("{}/x509_chain.pem",hatn::test::MultiThreadFixture::tmpPath());
        ec=chain3->saveToFile(tmpFile);
        HATN_REQUIRE(!ec);
        auto chain4=suite->createX509CertificateChain(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(chain4);
        ec=chain4->loadFromFile(tmpFile);
        HATN_REQUIRE(!ec);
        ec=im1_2Pem->verify(*store1,chain4.get());
        BOOST_CHECK(!ec);
        ec=client1_2_1Pem->verify(*store1,chain4.get());
        BOOST_CHECK(!ec);
        ec=client1_2_1Pem->verify(*store1);
        BOOST_CHECK(ec);
        ec=client1_2_2Pem->verify(*store1,chain4.get());
        BOOST_CHECK(!ec);
        ec=client2_1Pem->verify(*store1,chain4.get());
        BOOST_CHECK(ec);
        std::ignore=FileUtils::remove(tmpFile);

        BOOST_TEST_MESSAGE(fmt::format("Done with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));
    };
    checkAlg(algHandler);
}

BOOST_AUTO_TEST_CASE(CheckX509Signature)
{
    auto algHandler=[](std::shared_ptr<CryptPlugin>& plugin,const std::shared_ptr<CipherSuite>& suite,const std::string& algName,const std::string& pathPrefix)
    {
        BOOST_TEST_MESSAGE(fmt::format("Testing with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));

        Error ec;

        const CryptAlgorithm* alg=nullptr;
        ec=suite->signatureAlgorithm(alg);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(alg);

        // load private key
        auto pKey1=alg->createPrivateKey();
        BOOST_REQUIRE(pKey1);
        std::string pkey1File=fmt::format("{}-ca1_pkey.pem",pathPrefix);
        ec=pKey1->importFromFile(pkey1File,ContainerFormat::PEM);
        BOOST_REQUIRE(!ec);

        // sign message
        auto signProcessor=suite->createSignatureSign(ec);
        BOOST_REQUIRE(signProcessor);
        signProcessor->setKey(pKey1.get());
        std::string msgData="Hello from Dracosha!";
        ByteArray sig1;
        ec=signProcessor->sign(SpanBuffer(msgData),sig1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        HATN_REQUIRE(!ec);

        // load certificate
        auto ca1Pem=suite->createX509Certificate(ec);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(ca1Pem);
        std::string ca1PemFile=fmt::format("{}-ca1.pem",pathPrefix);
        ec=ca1Pem->loadFromFile(ca1PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);

        // verify message
        auto verifyProcessor=suite->createSignatureVerify(ec);
        BOOST_REQUIRE(verifyProcessor);
        ec=verifyProcessor->verifyX509(SpanBuffer(msgData),sig1,ca1Pem.get());
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_CHECK(!ec);

        BOOST_TEST_MESSAGE(fmt::format("Done with prefix {} for algorithm {} with plugin {}",pathPrefix,algName,plugin->info()->name));
    };
    checkAlg(algHandler);
}

BOOST_AUTO_TEST_SUITE_END()

}
}
