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

    auto addRole=[&res,&topic,&role1](auto&& listRoles)
    {
        BOOST_TEST_MESSAGE("Adding role");

        auto cb=[listRoles,&role1](auto ctx, const Error& ec, const HATN_DATAUNIT_NAMESPACE::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_TEST_MESSAGE(fmt::format("Role added name={}, id={}",role1,oid.string()));
            listRoles(std::move(ctx));
        };

        auto role=makeShared<acl_role::managed>();
        role->setFieldValue(acl_role::name,role1);
        role->setFieldValue(acl_role::description,"description of role1");
        auto ctx1=makeAppEnvContext(res.app->env());
        res.ctrl->addRole(
            ctx1,
            cb,
            role,
            topic
        );
    };

    auto listRoles=[&res,&topic,&role1](auto ctx)
    {
        BOOST_TEST_MESSAGE("Listing roles");

        auto cb=[&role1](auto ctx, common::Result<common::pmr::vector<DbObject>> result) mutable
        {
            HATN_TEST_RESULT(result)
            BOOST_REQUIRE(!result);
            BOOST_TEST_MESSAGE(fmt::format("Roles listed, found {}",result->size()));
            BOOST_REQUIRE_EQUAL(1,result->size());
            const auto* role=result->at(0).as<acl::acl_role::type>();
            BOOST_TEST_MESSAGE(fmt::format("Found role name {}, id {}",role->fieldValue(acl::acl_role::name),role->fieldValue(db::object::_id).string()));
        };

        auto queryBuilder=[&topic]()
        {
            return db::makeQuery(aclRoleNameIdx(),db::query::where(acl::acl_role::name,db::query::gte,db::query::First),topic);
        };
        res.ctrl->listRoles(
            ctx,
            cb,
            wrapQueryBuilder(queryBuilder,topic),
            topic
        );
    };

    auto chain=hatn::chain(
            addRole,
            listRoles
        );
    chain();

    exec(3);

    res.app->close();
}

BOOST_AUTO_TEST_SUITE_END()
