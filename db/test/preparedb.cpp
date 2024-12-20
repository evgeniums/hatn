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

#define HATN_TEST_DB_ECRYPTED_ONLY 1
#define HATN_TEST_DB_PLAIN_AND_ENCRYPTED 2

#define HATN_TEST_DB_COUNT HATN_TEST_DB_PLAIN_AND_ENCRYPTED
// #define HATN_TEST_DB_COUNT HATN_TEST_DB_ECRYPTED_ONLY

// #define HATN_TEST_DB_SAVE_CRYPT_FILES

HATN_TEST_NAMESPACE_BEGIN

db::ClientConfig* PrepareDbAndRun::currentCfg=nullptr;

void PrepareDbAndRun::eachPlugin(const TestFn& fn, const std::string& testConfigFile, const std::vector<PartitionRange>& partitions)
{
    DbPluginTest::instance().eachPlugin<DbTestTraits>(
        [&](std::shared_ptr<db::DbPlugin>& plugin)
        {
        for (size_t i=0;i<HATN_TEST_DB_COUNT;i++)
        {
            std::shared_ptr<crypt::CryptPlugin> cryptPlugin;
            std::shared_ptr<db::EncryptionManager> encryptionManager;
            if (i==1)
            {
                if (plugin->info()->name!=HATN_TEST_DB_ENCRYPTED_PLUGIN)
                {
                    continue;
                }

                // prepare encryption manager

                // load crypt plugin
                cryptPlugin=common::PluginLoader::instance().loadPlugin<crypt::CryptPlugin>(
                        HATN_TEST_DB_ENCRYPTED_CRYPT_PLUGIN
                    );
                if (!cryptPlugin)
                {
                    continue;
                }

                auto path=PluginList::assetsPath("db");
                auto cipherSuiteFile=fmt::format("{}/crypt-ciphersuite1.json",path);
                auto passphraseFile=fmt::format("{}/crypt-passphrase1.dat",path);
                auto keyFile=fmt::format("{}/crypt-key1.dat",path);

#ifdef HATN_TEST_DB_SAVE_CRYPT_FILES
                if (!lib::filesystem::exists(cipherSuiteFile))
                {
                    continue;
                }
#else
                if (!lib::filesystem::exists(cipherSuiteFile)
                    ||
                    !lib::filesystem::exists(keyFile)
                    ||
                    !lib::filesystem::exists(passphraseFile)
                    )
                {
                    continue;
                }
#endif
                BOOST_TEST_MESSAGE("Testing encrypted rocksdb");
                common::ByteArray cipherSuiteJson;
                auto ec=cipherSuiteJson.loadFromFile(cipherSuiteFile);
                HATN_REQUIRE(!ec);
                auto suite=std::make_shared<crypt::CipherSuite>();
                ec=suite->loadFromJSON(cipherSuiteJson);
                HATN_REQUIRE(!ec);
                // add suite to table of suites
                crypt::CipherSuites::instance().addSuite(suite);
                // set engine
                auto engine=std::make_shared<crypt::CryptEngine>(cryptPlugin.get());
                crypt::CipherSuites::instance().setDefaultEngine(std::move(engine));
                // load passphrase
                auto passphrase=suite->createPassphraseKey(ec);
                HATN_REQUIRE(!ec);
                HATN_REQUIRE(passphrase);

#ifdef HATN_TEST_DB_SAVE_CRYPT_FILES
                auto passwordGen=cryptPlugin->createPasswordGenerator();
                HATN_REQUIRE(passwordGen);
                ec=passphrase->generatePassword(passwordGen.get());
                HATN_REQUIRE(!ec);
                ec=passphrase->exportToFile(passphraseFile,crypt::ContainerFormat::RAW_PLAIN);
                HATN_REQUIRE(!ec)
#endif
                ec=passphrase->importFromFile(passphraseFile,crypt::ContainerFormat::RAW_PLAIN);
                HATN_REQUIRE(!ec)
                // find aead algorithm
                const crypt::CryptAlgorithm* aeadAlg=nullptr;
                ec=suite->aeadAlgorithm(aeadAlg);
                HATN_REQUIRE(!ec);
                HATN_REQUIRE(aeadAlg!=nullptr);
                // create and load master key
                auto keyProtector=common::makeShared<crypt::KeyProtector>(passphrase,suite.get());
                auto masterKey=aeadAlg->createSymmetricKey();
                HATN_REQUIRE(masterKey);
                masterKey->setProtector(keyProtector.get());
#ifdef HATN_TEST_DB_SAVE_CRYPT_FILES
                ec=masterKey->generate();
                HATN_REQUIRE(!ec);
                ec=masterKey->exportToFile(keyFile,crypt::ContainerFormat::RAW_ENCRYPTED);
                HATN_REQUIRE(!ec);
#endif
                ec=masterKey->importFromFile(keyFile,crypt::ContainerFormat::RAW_ENCRYPTED);
                HATN_REQUIRE(!ec)
                masterKey->setProtector(nullptr);

                encryptionManager=std::make_shared<db::EncryptionManager>();
                encryptionManager->setSuite(suite);
                encryptionManager->setDefaultKey(masterKey);
            }
            else
            {
                BOOST_TEST_MESSAGE("Testing not encrypted rocksdb");
            }

            // make client
            auto client=plugin->makeClient();
            BOOST_REQUIRE(client);

            // load main config
            base::ConfigTree mainCfg;
            base::ConfigTreeLoader loader;
            loader.setPrefixSubstitution("$tmp",MultiThreadFixture::tmpPath());
            auto configFile=PluginList::assetsFilePath(db::DB_MODULE_NAME,testConfigFile,plugin->info()->name);
            auto ec=loader.loadFromFile(mainCfg,configFile);
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_REQUIRE(!ec);

            // load options
            base::ConfigTree optCfg;
            ec=loader.loadFromFile(optCfg,configFile);
            if (ec)
            {
                BOOST_TEST_MESSAGE(ec.message());
            }
            BOOST_REQUIRE(!ec);

            // prepare config
            base::ConfigTreePath cfgPath{plugin->info()->name};            
            std::shared_ptr<db::ClientEnvironment> environment;
            db::ClientConfig cfg{
                mainCfg, optCfg, cfgPath, cfgPath.copyAppend("options"),
                encryptionManager,
                environment
            };
            currentCfg=&cfg;
            base::config_object::LogRecords logRecords;

            // destroy existing database
            BOOST_TEST_MESSAGE(fmt::format("destroying database by {} client",plugin->info()->name));
            ec=client->destroyDb(cfg,logRecords);
            for (auto&& it:logRecords)
            {
                BOOST_TEST_MESSAGE(fmt::format("destroy DB configuration \"{}\": {}",it.name,it.value));
            }
            BOOST_REQUIRE(!ec);

            // create database
            BOOST_TEST_MESSAGE(fmt::format("creating database by {} client",plugin->info()->name));
            ec=client->createDb(cfg,logRecords);
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
            ec=client->openDb(cfg,logRecords);
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

HATN_TEST_NAMESPACE_END
