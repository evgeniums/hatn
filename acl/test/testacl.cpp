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

#include <boost/function.hpp>
#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include <hatn/test/multithreadfixture.h>

#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/db/schema.h>

#include <hatn/acl/acldbmodelsprovider.h>
#include <hatn/acl/acldbmodels.h>
#include <hatn/acl/aclcontroller.h>
#include <hatn/acl/ipp/aclcontroller.ipp>

#include <hatn/app/app.h>
#include <hatn/app/appenv.h>

HATN_APP_USING
HATN_ACL_USING
HATN_DB_USING
HATN_COMMON_USING
HATN_TEST_USING

struct ContextTraits
{
    using Context=AppEnvContext;

    static auto& contextDb(const SharedPtr<Context>& ctx)
    {
        return ctx->get<WithAppEnv>().env()->get<Db>();
    }

    static const pmr::AllocatorFactory* contextFactory(const SharedPtr<Context>& ctx)
    {
        return ctx->get<WithAppEnv>().env()->get<AllocatorFactory>().factory();
    }
};

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

    auto dbModels=std::make_shared<AclDbModels>();
    AclDbModelsProvider dbModelsProvider{dbModels};
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

    auto ctrl=std::make_shared<AclController<ContextTraits>>(dbModels);

    return std::make_pair(app,ctrl);
}

BOOST_AUTO_TEST_SUITE(TestAcl)

BOOST_FIXTURE_TEST_CASE(Simple,TestEnv)
{
    auto res=createApp("config.jsonc");
    auto app=res.first;
    auto ctrl=res.second;
    BOOST_REQUIRE(app);
    BOOST_REQUIRE(ctrl);

    auto cb=[](auto ctx, AclStatus status, const HATN_NAMESPACE::Error& ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Check access cb: status={}, ec={}",static_cast<int>(status),ec.message()));
    };

    auto ctx=makeAppEnvContext(app->env());
    ctrl->checkAccess(ctx,cb,"obj1","subj1","op1","topic1");

    exec(1);

    app->close();
}

BOOST_AUTO_TEST_SUITE_END()
