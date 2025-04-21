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

#include <hatn/common/meta/chain.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/db/schema.h>

#include <hatn/acl/acldbmodelsprovider.h>
#include <hatn/acl/acldbmodels.h>
#include <hatn/acl/accesschecker.h>
#include <hatn/acl/ipp/accesschecker.ipp>
#include <hatn/acl/localaclcontroller.h>
#include <hatn/acl/ipp/localaclcontroller.ipp>

#include <hatn/app/app.h>
#include <hatn/app/appenv.h>

HATN_APP_USING
HATN_ACL_USING
HATN_DB_USING
HATN_COMMON_USING
HATN_TEST_USING
HATN_USING

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

struct AppCtx
{
    std::shared_ptr<App> app;
    std::shared_ptr<AccessChecker<ContextTraits>> checker;
    std::shared_ptr<LocalAclController<ContextTraits>> ctrl;
};

auto createApp(std::string configFileName)
{
    AppName appName{"testacl","Test Acl"};
    auto app=std::make_shared<App>(appName);
    auto configFile=MultiThreadFixture::assetsFilePath("acl",configFileName);
    app->setAppDataFolder(MultiThreadFixture::tmpPath());
    BOOST_TEST_MESSAGE(fmt::format("Data dir {}",MultiThreadFixture::tmpPath()));
    auto ec=app->loadConfigFile(configFile);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);
    ec=app->init();
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    ec=app->destroyDb();
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
    dbModelsProvider.unregisterRocksdbModels();
    dbModelsProvider.registerRocksdbModels();

    auto dbSchema=std::make_shared<Schema>();
    dbSchema->addModels(&dbModelsProvider);
    app->unregisterDbSchema(dbSchema->name());
    app->registerDbSchema(dbSchema);
    ec=app->database().setSchema(dbSchema);
    HATN_TEST_EC(ec)
    if (ec)
    {
        app->close();
    }
    BOOST_REQUIRE(!ec);

    auto checker=std::make_shared<AccessChecker<ContextTraits>>(dbModels);

    auto ctrl=std::make_shared<LocalAclController<ContextTraits>>(dbModels);

    return AppCtx{app,checker,ctrl};
}

BOOST_AUTO_TEST_SUITE(TestAcl)

BOOST_FIXTURE_TEST_CASE(CheckEmptyAcl,TestEnv)
{
    auto res=createApp("config.jsonc");
    BOOST_REQUIRE(res.app);
    BOOST_REQUIRE(res.checker);

    auto cb=[](auto ctx, AclStatus status, const HATN_NAMESPACE::Error& ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("Check access cb: status={}, ec={}",static_cast<int>(status),ec.message()));
    };
    auto ctx=makeAppEnvContext(res.app->env());
    res.checker->checkAccess(ctx,cb,"obj1","subj1","op1","topic1");

    exec(1);

    res.app->close();
}

BOOST_FIXTURE_TEST_CASE(AclControllerOps,TestEnv)
{
    auto res=createApp("config.jsonc");
    BOOST_REQUIRE(res.app);
    BOOST_REQUIRE(res.checker);
    std::string topic{"topic1"};
    std::string role1{"role1"};

    auto addRole=[&res,&topic,role1](auto&& listRoles)
    {
        BOOST_TEST_MESSAGE("Adding role");

        auto addRoleCb=[listRoles](auto ctx, const HATN_NAMESPACE::Error& ec) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_TEST_MESSAGE("Role added");
            listRoles(std::move(ctx));
        };

        auto role=makeShared<acl_role::managed>();
        role->setFieldValue(acl_role::name,role1);
        role->setFieldValue(acl_role::description,"description of role1");
        auto ctx1=makeAppEnvContext(res.app->env());
        res.ctrl->addRole(
            ctx1,
            addRoleCb,
            role,
            topic
        );
    };

    auto listRoles=[&res](auto ctx)
    {
        BOOST_TEST_MESSAGE("Listing roles");
    };

    auto chain=hatn::chain(
            addRole,
            listRoles
        );
    chain();

    exec(1);

    res.app->close();
}

BOOST_AUTO_TEST_SUITE_END()
