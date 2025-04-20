/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/test/testapp.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include <hatn/test/multithreadfixture.h>

#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/db/schema.h>

#include <hatn/acl/acldbmodelsprovider.h>
#include <hatn/acl/acldbmodels.h>

#include <hatn/app/app.h>

HATN_APP_USING
HATN_ACL_USING
HATN_DB_USING
HATN_TEST_USING

struct TestEnv : public MultiThreadFixture
{
    TestEnv()
    {
    }

    ~TestEnv()
    {
    }

    TestEnv(const TestEnv&)=delete;
    TestEnv(TestEnv&&) =delete;
    TestEnv& operator=(const TestEnv&)=delete;
    TestEnv& operator=(TestEnv&&) =delete;
};


auto createApp(std::string configFileName)
{
    AppName appName{"testacl","Test Acl"};
    auto app=std::make_shared<App>(appName);
    auto configFile=MultiThreadFixture::assetsFilePath("acl",configFileName);
    app->setAppDataFolder(MultiThreadFixture::tmpPath());
    auto ec=app->loadConfigFile(configFile);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);
    ec=app->init();
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    ec=app->openDb();
    if (ec)
    {
        app->close();
    }
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    AclDbModelsProvider dbModelsProvider{std::make_shared<AclDbModels>()};
    dbModelsProvider.registerRocksdbModels();

    auto dbSchema=std::make_shared<Schema>();
    dbSchema->addModels(&dbModelsProvider);
    app->registerDbSchema(dbSchema);
    ec=app->database().setSchema(dbSchema);
    HATN_TEST_EC(ec)
    if (ec)
    {
        app->close();
    }
    BOOST_REQUIRE(!ec);

    return app;
}

BOOST_AUTO_TEST_SUITE(TestAcl)

BOOST_FIXTURE_TEST_CASE(Simple,TestEnv)
{
    auto app=createApp("config.jsonc");
    BOOST_REQUIRE(app);

    exec(1);

    app->close();
}

BOOST_AUTO_TEST_SUITE_END()
