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

#include <hatn/db/client.h>

#include "initdbplugins.h"
#include "preparedb.h"

HATN_TEST_NAMESPACE_BEGIN

void PrepareDbAndRun::eachPlugin(const TestFn& fn, const std::string& testConfigFile, const std::vector<PartitionRange>& partitions)
{
    DbPluginTest::instance().eachPlugin<DbTestTraits>(
        [&](std::shared_ptr<db::DbPlugin>& plugin)
        {
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
            db::ClientConfig cfg{
                mainCfg, optCfg, cfgPath, cfgPath.copyAppend("options")
            };
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
            BOOST_REQUIRE(!ec);

            // open database
            BOOST_TEST_MESSAGE(fmt::format("opening database by {} client",plugin->info()->name));
            ec=client->openDb(cfg,logRecords);
            for (auto&& it:logRecords)
            {
                BOOST_TEST_MESSAGE(fmt::format("open DB configuration \"{}\": {}",it.name,it.value));
            }
            BOOST_REQUIRE(!ec);

            // add partitions
            for (auto&& partitionRange:partitions)
            {
                ec=client->addDatePartitions(partitionRange.models,partitionRange.to,partitionRange.from);
                BOOST_REQUIRE(!ec);
            }

            // invoke test
            fn(plugin,client);
        }
    );
}

HATN_TEST_NAMESPACE_END
