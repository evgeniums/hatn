/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/preparedb.сpp
  *
  *  Prepare database and run test.
  *
*/

#include <boost/test/unit_test.hpp>

#include <hatn/base/configtreeloader.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/crypt/keyprotector.h>

#include <hatn/db/client.h>

#include "initdbplugins.h"
#include "preparedb.h"

#ifndef HATN_TEST_DB_ENCRYPTED_PLUGIN
#define HATN_TEST_DB_ENCRYPTED_PLUGIN "hatnrocksdb"
#endif

#ifndef HATN_TEST_DB_ENCRYPTED_CRYPT_PLUGIN
#define HATN_TEST_DB_ENCRYPTED_CRYPT_PLUGIN "hatnopenssl"
#endif

#define HATN_TEST_DB_PLAIN_ONLY 1
#define HATN_TEST_DB_PLAIN_AND_ENCRYPTED 2

#define HATN_TEST_DB_COUNT HATN_TEST_DB_PLAIN_AND_ENCRYPTED
// #define HATN_TEST_DB_COUNT HATN_TEST_DB_PLAIN_ONLY

// #define HATN_TEST_DB_SAVE_CRYPT_FILES

HATN_TEST_NAMESPACE_BEGIN

db::ClientConfig* PrepareDbAndRun::currentCfg=nullptr;

void PrepareDbAndRun::eachPlugin(const TestFn& fn, const std::string& testConfigFile, const std::vector<PartitionRange>& partitions)
{
    DbPluginTest::instance().eachPlugin<DbTestTraits>(
        [&](std::shared_ptr<db::DbPlugin>& plugin)
        {
            if (testConfigFile.empty())
            {
                // invoke test without client
                fn(plugin,std::shared_ptr<db::Client>{});
                return;
            }

            for (size_t i=0;i<HATN_TEST_DB_COUNT;i++)
            {
                std::shared_ptr<crypt::CryptPlugin> cryptPlugin;
                std::shared_ptr<db::EncryptionManager> encryptionManager;
                if (i==1)
                {
                    if (!prepareEncryption(plugin,cryptPlugin,encryptionManager))
                    {
                        continue;
                    }
                    BOOST_TEST_MESSAGE("Testing encrypted rocksdb");
                }
                else
                {
                    BOOST_TEST_MESSAGE("Testing not encrypted rocksdb");
                }

                // make client
                auto client=plugin->makeClient();
                BOOST_REQUIRE(client);

                auto cfg=prepareConfig(plugin,testConfigFile,encryptionManager);
                currentCfg=cfg.get();

                // destroy existing database
                BOOST_TEST_MESSAGE(fmt::format("destroying database by {} client",plugin->info()->name));
                base::config_object::LogRecords logRecords;
                auto ec=client->destroyDb(*cfg,logRecords);
                for (auto&& it:logRecords)
                {
                    BOOST_TEST_MESSAGE(fmt::format("destroy DB configuration \"{}\": {}",it.name,it.value));
                }
                BOOST_REQUIRE(!ec);

                // create database
                BOOST_TEST_MESSAGE(fmt::format("creating database by {} client",plugin->info()->name));
                ec=client->createDb(*cfg,logRecords);
                for (auto&& it:logRecords)
                {
                    BOOST_TEST_MESSAGE(fmt::format("create DB configuration \"{}\": {}",it.name,it.value));
                }
                if (ec)
                {
                    BOOST_TEST_MESSAGE(ec.message());
                }
                BOOST_REQUIRE(!ec);

                // open database
                BOOST_TEST_MESSAGE(fmt::format("opening database by {} client",plugin->info()->name));
                ec=client->openDb(*cfg,logRecords);
                for (auto&& it:logRecords)
                {
                    BOOST_TEST_MESSAGE(fmt::format("open DB configuration \"{}\": {}",it.name,it.value));
                }
                if (ec)
                {
                    BOOST_FAIL(ec.message());
                }

                // add partitions
                for (auto&& partitionRange:partitions)
                {
                    ec=client->addDatePartitions(partitionRange.models,partitionRange.to,partitionRange.from);
                    BOOST_REQUIRE(!ec);
                }

                // invoke test
                fn(plugin,client);

                // close db
                ec=client->closeDb();
                if (ec)
                {
                    BOOST_TEST_MESSAGE(fmt::format("failed to close database: {}",ec.message()));
                }

                // cleanup
                currentCfg=nullptr;
            }
            crypt::CipherSuites::instance().reset();
        }
    );
}

std::shared_ptr<db::ClientConfig> PrepareDbAndRun::prepareConfig(
        const std::shared_ptr<db::DbPlugin>& plugin,
        const std::string &path,
        std::shared_ptr<db::EncryptionManager> encryptionManager
    )
{
    // load main config
    auto mainCfg=std::make_shared<base::ConfigTree>();
    base::ConfigTreeLoader loader;
    loader.setPrefixSubstitution("$tmp",MultiThreadFixture::tmpPath());
    auto configFile=PluginList::assetsFilePath(db::DB_MODULE_NAME,path,plugin->info()->name);
    auto ec=loader.loadFromFile(*mainCfg,configFile);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);

    // load options
    auto optCfg=std::make_shared<base::ConfigTree>();
    ec=loader.loadFromFile(*optCfg,configFile);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(!ec);

    // prepare config
    base::ConfigTreePath cfgPath{plugin->info()->name};
    std::shared_ptr<db::ClientEnvironment> environment;
    auto cfg=std::make_shared<db::ClientConfig>(
        std::move(mainCfg),
        std::move(optCfg),
        cfgPath,
        cfgPath.copyAppend("options"),
        std::move(encryptionManager),
        std::move(environment)
    );
    return cfg;
}

bool PrepareDbAndRun::prepareEncryption(
        const std::shared_ptr<db::DbPlugin>& plugin,
        std::shared_ptr<crypt::CryptPlugin> &cryptPlugin,
        std::shared_ptr<db::EncryptionManager> &encryptionManager
    )
{
    if (plugin->info()->name!=HATN_TEST_DB_ENCRYPTED_PLUGIN)
    {
        return false;
    }

    // prepare encryption manager

    // load crypt plugin
    cryptPlugin=common::PluginLoader::instance().loadPlugin<crypt::CryptPlugin>(
        HATN_TEST_DB_ENCRYPTED_CRYPT_PLUGIN
        );
    if (!cryptPlugin)
    {
        return false;
    }

    auto path=PluginList::assetsPath("db");
    auto cipherSuiteFile=fmt::format("{}/crypt-ciphersuite1.json",path);
    auto passphraseFile=fmt::format("{}/crypt-passphrase1.dat",path);
    auto keyFile=fmt::format("{}/crypt-key1.dat",path);

#ifdef HATN_TEST_DB_SAVE_CRYPT_FILES
    if (!lib::filesystem::exists(cipherSuiteFile))
    {
        return false;
    }
#else
    if (!lib::filesystem::exists(cipherSuiteFile)
        ||
        !lib::filesystem::exists(keyFile)
        ||
        !lib::filesystem::exists(passphraseFile)
        )
    {
        return false;
    }
#endif
    common::ByteArray cipherSuiteJson;
    auto ec=cipherSuiteJson.loadFromFile(cipherSuiteFile);
    BOOST_REQUIRE(!ec);
    auto suite=std::make_shared<crypt::CipherSuite>();
    ec=suite->loadFromJSON(cipherSuiteJson);
    BOOST_REQUIRE(!ec);
    // add suite to table of suites
    crypt::CipherSuites::instance().addSuite(suite);
    // set engine
    auto engine=std::make_shared<crypt::CryptEngine>(cryptPlugin.get());
    crypt::CipherSuites::instance().setDefaultEngine(std::move(engine));
    // load passphrase
    auto passphrase=suite->createPassphraseKey(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(passphrase);

#ifdef HATN_TEST_DB_SAVE_CRYPT_FILES
    auto passwordGen=cryptPlugin->createPasswordGenerator();
    BOOST_REQUIRE(passwordGen);
    ec=passphrase->generatePassword(passwordGen.get());
    BOOST_REQUIRE(!ec);
    ec=passphrase->exportToFile(passphraseFile,crypt::ContainerFormat::RAW_PLAIN);
    BOOST_REQUIRE(!ec);
#endif
    ec=passphrase->importFromFile(passphraseFile,crypt::ContainerFormat::RAW_PLAIN);
    BOOST_REQUIRE(!ec);
    // find aead algorithm
    const crypt::CryptAlgorithm* aeadAlg=nullptr;
    ec=suite->aeadAlgorithm(aeadAlg);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(aeadAlg!=nullptr);
    // create and load master key
    auto keyProtector=common::makeShared<crypt::KeyProtector>(passphrase,suite.get());
    auto masterKey=aeadAlg->createSymmetricKey();
    BOOST_REQUIRE(masterKey);
    masterKey->setProtector(keyProtector.get());
#ifdef HATN_TEST_DB_SAVE_CRYPT_FILES
    ec=masterKey->generate();
    BOOST_REQUIRE(!ec);
    ec=masterKey->exportToFile(keyFile,crypt::ContainerFormat::RAW_ENCRYPTED);
    BOOST_REQUIRE(!ec);
#endif
    ec=masterKey->importFromFile(keyFile,crypt::ContainerFormat::RAW_ENCRYPTED);
    BOOST_REQUIRE(!ec);
    masterKey->setProtector(nullptr);

    encryptionManager=std::make_shared<db::EncryptionManager>();
    encryptionManager->setSuite(suite);
    encryptionManager->setDefaultKey(masterKey);

    return true;
}

HATN_TEST_NAMESPACE_END
