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

#include <hatn/utility/acldbmodelsprovider.h>
#include <hatn/utility/acldbmodels.h>
#include <hatn/utility/accesschecker.h>
#include <hatn/utility/ipp/accesschecker.ipp>
#include <hatn/utility/localaclcontroller.h>
#include <hatn/utility/ipp/localaclcontroller.ipp>

#include <hatn/app/app.h>
#include <hatn/app/appenv.h>

HATN_APP_USING
HATN_UTILITY_USING
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

    static auto subject(const SharedPtr<Context>& ctx)
    {
        return ObjectWrapperRef{};
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
    auto configFile=MultiThreadFixture::assetsFilePath("utility",configFileName);
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
    // res.checker->checkAccess(ctx,cb,"obj1","subj1",&AclOperations::addRole(),"topic1");
    res.checker->checkAccess(ctx,cb,&AclOperations::addRole(),"topic1");

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
    std::string description1{"description1"};
    std::string opGrant{"op_grant"};
    std::string opDeny{"op_deny"};
    std::string subj1{"subj1"};
    std::string obj1{"obj1"};
    du::ObjectId roleId;;
    du::ObjectId roleOpId;
    du::ObjectId relationId;

    auto addRole=[&res,&topic,&role1,&description1,&roleId](auto&& listRoles)
    {
        BOOST_TEST_MESSAGE("Adding role");

        auto cb=[listRoles,&role1,&roleId](auto ctx, const Error& ec, const HATN_DATAUNIT_NAMESPACE::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_TEST_MESSAGE(fmt::format("Role added name={}, id={}",role1,oid.string()));

            roleId=oid;
            listRoles(std::move(ctx));
        };

        auto role=makeShared<acl_role::managed>();
        role->setFieldValue(acl_role::name,role1);
        role->setFieldValue(acl_role::description,description1);
        auto ctx1=makeAppEnvContext(res.app->env());
        res.ctrl->addRole(
            ctx1,
            cb,
            role,
            topic
        );
    };

    auto listRoles=[&res,&topic,&role1,&description1](auto&& addRoleOperation, auto ctx)
    {
        BOOST_TEST_MESSAGE("Listing roles");

        auto cb=[addRoleOperation{std::move(addRoleOperation)},&role1,&description1](auto ctx, common::Result<common::pmr::vector<DbObject>> result) mutable
        {
            HATN_TEST_RESULT(result)
            BOOST_REQUIRE(!result);
            BOOST_TEST_MESSAGE(fmt::format("Roles listed, found {}",result->size()));
            BOOST_REQUIRE_EQUAL(1,result->size());
            const auto* role=result->at(0).as<HATN_UTILITY_NAMESPACE::acl_role::type>();
            BOOST_TEST_MESSAGE(fmt::format("Found role name {}, description {}, id {}",
                                           role->fieldValue(HATN_UTILITY_NAMESPACE::acl_role::name),
                                           role->fieldValue(HATN_UTILITY_NAMESPACE::acl_role::description),
                                           role->fieldValue(db::object::_id).string()));
            BOOST_CHECK_EQUAL(role1,std::string(role->fieldValue(HATN_UTILITY_NAMESPACE::acl_role::name)));
            BOOST_CHECK_EQUAL(description1,std::string(role->fieldValue(HATN_UTILITY_NAMESPACE::acl_role::description)));

            addRoleOperation(std::move(ctx),role->fieldValue(db::object::_id));
        };

        auto queryBuilder=[&topic]()
        {
            return db::makeQuery(aclRoleNameIdx(),db::query::where(HATN_UTILITY_NAMESPACE::acl_role::name,db::query::gte,db::query::First),topic);
        };
        res.ctrl->listRoles(
            std::move(ctx),
            std::move(cb),
            wrapQueryBuilder(queryBuilder,topic),
            topic
        );
    };

    auto addRoleOperation=[&res,&topic,&opGrant,&roleOpId](auto&& listRoleOperations, auto ctx, ObjectId roleOid)
    {
        BOOST_TEST_MESSAGE("Adding role operation");

        auto cb=[listRoleOperations=std::move(listRoleOperations),roleOid,&roleOpId](auto ctx, const Error& ec, const HATN_DATAUNIT_NAMESPACE::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_TEST_MESSAGE(fmt::format("Added grant operation to role, id={}",oid.string()));

            roleOpId=oid;
            listRoleOperations(std::move(ctx),roleOid);
        };

        auto roleOp=makeShared<acl_role_operation::managed>();
        roleOp->setFieldValue(acl_role_operation::role,roleOid);
        roleOp->setFieldValue(acl_role_operation::operation,opGrant);
        roleOp->setFieldValue(acl_role_operation::grant,true);
        res.ctrl->addRoleOperation(
            std::move(ctx),
            std::move(cb),
            roleOp,
            topic
        );
    };

    auto listRoleOperations=[&res,&topic,&opGrant](auto&& addRelation, auto ctx, ObjectId roleOid)
    {
        BOOST_TEST_MESSAGE("Listing role operations");

        auto cb=[addRelation=std::move(addRelation),&opGrant,roleOid](auto ctx, common::Result<common::pmr::vector<DbObject>> result) mutable
        {
            HATN_TEST_RESULT(result)
            BOOST_REQUIRE(!result);
            BOOST_TEST_MESSAGE(fmt::format("Role operations listed, found {}",result->size()));
            BOOST_REQUIRE_EQUAL(1,result->size());
            const auto* roleOp=result->at(0).as<HATN_UTILITY_NAMESPACE::acl_role_operation::type>();
            BOOST_TEST_MESSAGE(fmt::format("Found role operation role_id {}, operation {}, access {}, id {}",
                                           roleOp->fieldValue(HATN_UTILITY_NAMESPACE::acl_role_operation::role).string(),
                                           roleOp->fieldValue(HATN_UTILITY_NAMESPACE::acl_role_operation::operation),
                                           roleOp->fieldValue(HATN_UTILITY_NAMESPACE::acl_role_operation::grant),
                                           roleOp->fieldValue(db::object::_id).string()));
            BOOST_CHECK_EQUAL(roleOid.string(),roleOp->fieldValue(HATN_UTILITY_NAMESPACE::acl_role_operation::role).string());
            BOOST_CHECK_EQUAL(opGrant,roleOp->fieldValue(HATN_UTILITY_NAMESPACE::acl_role_operation::operation));
            BOOST_CHECK(roleOp->fieldValue(HATN_UTILITY_NAMESPACE::acl_role_operation::grant));

            addRelation(std::move(ctx),roleOid);
        };

        auto queryBuilder=[&topic]()
        {
            return makeQuery(oidIdx(),query::where(Oid,db::query::gte,db::query::First),topic);
        };
        res.ctrl->listRoleOperations(
            std::move(ctx),
            std::move(cb),
            wrapQueryBuilder(queryBuilder,topic),
            topic
        );
    };

    auto addRelation=[&res,&topic,&subj1,&obj1,&relationId](auto&& listRelations, auto ctx, ObjectId roleOid)
    {
        BOOST_TEST_MESSAGE("Adding subject-object-role relation");

        auto cb=[listRelations=std::move(listRelations),&subj1,&obj1,roleOid,&relationId](auto ctx, const Error& ec, const HATN_DATAUNIT_NAMESPACE::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_TEST_MESSAGE(fmt::format("Added suject-object-role relation: {}-{}-{}, id {}",subj1,obj1,roleOid.string(),oid.string()));

            relationId=oid;
            listRelations(std::move(ctx),roleOid);
        };

        auto subjObjRole=makeShared<acl_relation::managed>();
        subjObjRole->setFieldValue(acl_relation::role,roleOid);
        subjObjRole->setFieldValue(acl_relation::subject,subj1);
        subjObjRole->setFieldValue(acl_relation::object,obj1);
        res.ctrl->addRelation(
            std::move(ctx),
            std::move(cb),
            subjObjRole,
            topic
        );
    };

    auto listRelations=[&res,&topic,&subj1,&obj1](auto&& updateRole,auto ctx, ObjectId roleOid)
    {
        BOOST_TEST_MESSAGE("Listing subject-object-role relations");

        auto cb=[updateRole=std::move(updateRole),&subj1,&obj1,roleOid](auto ctx, common::Result<common::pmr::vector<DbObject>> result) mutable
        {
            HATN_TEST_RESULT(result)
            BOOST_REQUIRE(!result);
            BOOST_TEST_MESSAGE(fmt::format("Subject-object-role relations listed, found {}",result->size()));
            BOOST_REQUIRE_EQUAL(1,result->size());
            const auto* rel=result->at(0).as<HATN_UTILITY_NAMESPACE::acl_relation::type>();
            BOOST_TEST_MESSAGE(fmt::format("Found relation {}-{}-{}, id {}",
                                           rel->fieldValue(HATN_UTILITY_NAMESPACE::acl_relation::subject),
                                           rel->fieldValue(HATN_UTILITY_NAMESPACE::acl_relation::object),
                                           rel->fieldValue(HATN_UTILITY_NAMESPACE::acl_relation::role).string(),
                                           rel->fieldValue(db::object::_id).string()));
            BOOST_CHECK_EQUAL(roleOid.string(),rel->fieldValue(HATN_UTILITY_NAMESPACE::acl_relation::role).string());
            BOOST_CHECK_EQUAL(subj1,rel->fieldValue(HATN_UTILITY_NAMESPACE::acl_relation::subject));
            BOOST_CHECK_EQUAL(obj1,rel->fieldValue(HATN_UTILITY_NAMESPACE::acl_relation::object));

            updateRole(std::move(ctx),roleOid);
        };

        auto queryBuilder=[&topic]()
        {
            return makeQuery(oidIdx(),query::where(Oid,db::query::gte,db::query::First),topic);
        };
        res.ctrl->listRelations(
            std::move(ctx),
            std::move(cb),
            wrapQueryBuilder(queryBuilder,topic),
            topic
        );
    };

    auto updateRole=[&res,&topic,&role1,&description1](auto&& listRolesAfterUpdate, auto ctx, ObjectId roleOid)
    {
        BOOST_TEST_MESSAGE("Updating role");

        auto cb=[listRolesAfterUpdate=std::move(listRolesAfterUpdate)](auto ctx, const Error& ec) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_TEST_MESSAGE(fmt::format("Updated role"));

            listRolesAfterUpdate(std::move(ctx));
        };

        role1="new_role";
        description1="new description";
        auto request=update::sharedRequest(
            update::field(HATN_UTILITY_NAMESPACE::acl_role::name,update::set,role1),
            update::field(HATN_UTILITY_NAMESPACE::acl_role::description,update::set,description1)
        );
        res.ctrl->updateRole(
            std::move(ctx),
            std::move(cb),
            roleOid,
            request,
            topic
        );
    };

    auto listRolesAfterUpdate=[&res,&topic,&role1,&description1](auto&& removeRoleOperation, auto ctx)
    {
        BOOST_TEST_MESSAGE("Listing roles after update");

        auto cb=[removeRoleOperation=std::move(removeRoleOperation),&role1,&description1](auto ctx, common::Result<common::pmr::vector<DbObject>> result) mutable
        {
            HATN_TEST_RESULT(result)
            BOOST_REQUIRE(!result);
            BOOST_TEST_MESSAGE(fmt::format("Roles listed, found {}",result->size()));
            BOOST_REQUIRE_EQUAL(1,result->size());
            const auto* role=result->at(0).as<HATN_UTILITY_NAMESPACE::acl_role::type>();
            BOOST_TEST_MESSAGE(fmt::format("Found role name {}, description {}, id {}",
                                           role->fieldValue(HATN_UTILITY_NAMESPACE::acl_role::name),
                                           role->fieldValue(HATN_UTILITY_NAMESPACE::acl_role::description),
                                           role->fieldValue(db::object::_id).string()));
            BOOST_CHECK_EQUAL(role1,std::string(role->fieldValue(HATN_UTILITY_NAMESPACE::acl_role::name)));
            BOOST_CHECK_EQUAL(description1,std::string(role->fieldValue(HATN_UTILITY_NAMESPACE::acl_role::description)));

            removeRoleOperation(std::move(ctx));
        };

        auto queryBuilder=[&topic]()
        {
            return makeQuery(oidIdx(),query::where(Oid,db::query::gte,db::query::First),topic);
        };
        res.ctrl->listRoles(
            std::move(ctx),
            std::move(cb),
            wrapQueryBuilder(queryBuilder,topic),
            topic
        );
    };

    auto removeRoleOperation=[&res,&topic,&roleOpId](auto&& listRoleOpsAfterRemove, auto ctx)
    {
        BOOST_TEST_MESSAGE("Removing role operation");

        auto cb=[listRoleOpsAfterRemove=std::move(listRoleOpsAfterRemove)](auto ctx, const Error& ec) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_TEST_MESSAGE(fmt::format("Removed role operation"));

            listRoleOpsAfterRemove(std::move(ctx));
        };

        res.ctrl->removeRoleOperation(
            std::move(ctx),
            std::move(cb),
            roleOpId,
            topic
        );
    };

    auto listRoleOpsAfterRemove=[&res,&topic](auto&& removeRelation, auto ctx)
    {
        BOOST_TEST_MESSAGE("Listing role operations after remove");

        auto cb=[removeRelation=std::move(removeRelation)](auto ctx, common::Result<common::pmr::vector<DbObject>> result) mutable
        {
            HATN_TEST_RESULT(result)
            BOOST_REQUIRE(!result);
            BOOST_TEST_MESSAGE(fmt::format("Role operations listed, found {}",result->size()));
            BOOST_REQUIRE_EQUAL(0,result->size());

            removeRelation(std::move(ctx));
        };

        auto queryBuilder=[&topic]()
        {
            return makeQuery(oidIdx(),query::where(Oid,db::query::gte,db::query::First),topic);
        };
        res.ctrl->listRoleOperations(
            std::move(ctx),
            std::move(cb),
            wrapQueryBuilder(queryBuilder,topic),
            topic
        );
    };

    auto removeRelation=[&res,&topic,&relationId](auto&& listRelationsAfterRemove, auto ctx)
    {
        BOOST_TEST_MESSAGE("Removing relation");

        auto cb=[listRelationsAfterRemove=std::move(listRelationsAfterRemove)](auto ctx, const Error& ec) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_TEST_MESSAGE(fmt::format("Removed relation"));

            listRelationsAfterRemove(std::move(ctx));
        };

        res.ctrl->removeRelation(
            std::move(ctx),
            std::move(cb),
            relationId,
            topic
        );
    };

    auto listRelationsAfterRemove=[&res,&topic](auto&& removeRole, auto ctx)
    {
        BOOST_TEST_MESSAGE("Listing relations after remove");

        auto cb=[removeRole=std::move(removeRole)](auto ctx, common::Result<common::pmr::vector<DbObject>> result) mutable
        {
            HATN_TEST_RESULT(result)
            BOOST_REQUIRE(!result);
            BOOST_TEST_MESSAGE(fmt::format("Relations listed, found {}",result->size()));
            BOOST_REQUIRE_EQUAL(0,result->size());

            removeRole(std::move(ctx));
        };

        auto queryBuilder=[&topic]()
        {
            return makeQuery(oidIdx(),query::where(Oid,db::query::gte,db::query::First),topic);
        };
        res.ctrl->listRelations(
            std::move(ctx),
            std::move(cb),
            wrapQueryBuilder(queryBuilder,topic),
            topic
        );
    };

    auto removeRole=[&res,&topic,&roleId](auto&& listRolesAfterRemove, auto ctx)
    {
        BOOST_TEST_MESSAGE("Removing role");

        auto cb=[listRolesAfterRemove=std::move(listRolesAfterRemove)](auto ctx, const Error& ec) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_TEST_MESSAGE(fmt::format("Removed role"));

            listRolesAfterRemove(std::move(ctx));
        };

        res.ctrl->removeRole(
            std::move(ctx),
            std::move(cb),
            roleId,
            topic
        );
    };

    auto listRolesAfterRemove=[&res,&topic](auto ctx)
    {
        BOOST_TEST_MESSAGE("Listing roles after remove");

        auto cb=[](auto ctx, common::Result<common::pmr::vector<DbObject>> result) mutable
        {
            HATN_TEST_RESULT(result)
            BOOST_REQUIRE(!result);
            BOOST_TEST_MESSAGE(fmt::format("Roles listed, found {}",result->size()));
            BOOST_REQUIRE_EQUAL(0,result->size());
        };

        auto queryBuilder=[&topic]()
        {
            return makeQuery(oidIdx(),query::where(Oid,db::query::gte,db::query::First),topic);
        };
        res.ctrl->listRoles(
            std::move(ctx),
            std::move(cb),
            wrapQueryBuilder(queryBuilder,topic),
            topic
        );
    };

    auto chain=hatn::chain(
            addRole,
            listRoles,
            addRoleOperation,
            listRoleOperations,
            addRelation,
            listRelations,
            updateRole,
            listRolesAfterUpdate,
            removeRoleOperation,
            listRoleOpsAfterRemove,
            removeRelation,
            listRelationsAfterRemove,
            removeRole,
            listRolesAfterRemove
        );
    chain();

    exec(1);

    res.app->close();
}

BOOST_AUTO_TEST_SUITE_END()
