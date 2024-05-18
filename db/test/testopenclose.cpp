/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testopenclose.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/base/configtreeloader.h>
#include <hatn/test/multithreadfixture.h>

#include "hatn_test_config.h"
#include "initdbplugins.h"

HATN_USING
HATN_DB_USING
HATN_BASE_USING
HATN_TEST_USING

namespace {

void openEmptyConfig(std::shared_ptr<DbPlugin>& plugin)
{
    BOOST_TEST_MESSAGE(fmt::format("trying to open {} client with empty config",plugin->info()->name));

    auto client=plugin->makeClient();
    BOOST_REQUIRE(client);
    base::ConfigTree mainCfg;
    base::ConfigTree optCfg;
    ClientConfig cfg{
        mainCfg, optCfg, base::ConfigTreePath{}, base::ConfigTreePath{}
    };
    base::config_object::LogRecords logRecords;
    auto ec1=client->openDb(cfg,logRecords);
    BOOST_CHECK(ec1);
    BOOST_TEST_MESSAGE(ec1.message());
}

void createOpenCloseDestroy(std::shared_ptr<DbPlugin>& plugin)
{
    // make client
    auto client=plugin->makeClient();
    BOOST_REQUIRE(client);

    // load main config
    base::ConfigTree mainCfg;
    ConfigTreeLoader loader;
    loader.setPrefixSubstitution("$tmp",MultiThreadFixture::tmpPath());
    auto configFile=PluginList::assetsFilePath(DB_MODULE_NAME,"createopenclosedestroy.jsonc",plugin->info()->name);
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
    ClientConfig cfg{
        mainCfg, optCfg, cfgPath, cfgPath.copyAppend("options")
    };
    base::config_object::LogRecords logRecords;

    // create database
    BOOST_TEST_MESSAGE(fmt::format("creating database by {} client",plugin->info()->name));
    ec=client->createDb(cfg,logRecords);
    for (auto&& it:logRecords)
    {
        BOOST_TEST_MESSAGE(fmt::format("create DB configuration \"{}\": {}",it.name,it.value));
    }
    BOOST_CHECK(!ec);

    // open database
    BOOST_TEST_MESSAGE(fmt::format("opening database by {} client",plugin->info()->name));
    ec=client->openDb(cfg,logRecords);
    for (auto&& it:logRecords)
    {
        BOOST_TEST_MESSAGE(fmt::format("open DB configuration \"{}\": {}",it.name,it.value));
    }
    BOOST_CHECK(!ec);

    // close database
    BOOST_TEST_MESSAGE(fmt::format("closing database by {} client",plugin->info()->name));
    ec=client->closeDb();
    BOOST_CHECK(!ec);

    // destroy database
    BOOST_TEST_MESSAGE(fmt::format("destroying database by {} client",plugin->info()->name));
    ec=client->destroyDb(cfg,logRecords);
    for (auto&& it:logRecords)
    {
        BOOST_TEST_MESSAGE(fmt::format("destroy DB configuration \"{}\": {}",it.name,it.value));
    }
    BOOST_CHECK(!ec);
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(DbOpenClose, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(DbCreateOpenCloseDestroy)
{
    DbPluginTest::instance().eachPlugin<DbTestTraits>(
        [](std::shared_ptr<DbPlugin>& plugin)
        {
            createOpenCloseDestroy(plugin);
        }
    );
}

BOOST_AUTO_TEST_CASE(DbOpenEmptyConfig)
{
    DbPluginTest::instance().eachPlugin<DbTestTraits>(
        [](std::shared_ptr<DbPlugin>& plugin)
        {
            openEmptyConfig(plugin);
        }
        );
}

BOOST_AUTO_TEST_SUITE_END()
