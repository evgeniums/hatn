/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testtransaction.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

#include <hatn/test/multithreadfixture.h>

#include <hatn/db/schema.h>

#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>

#include <hatn/dataunit/syntax.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/db/object.h>
#include <hatn/db/model.h>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING
HATN_LOGCONTEXT_USING

namespace {

HDU_UNIT_WITH(u1,(HDU_BASE(object)),
    HDU_FIELD(f1,TYPE_UINT32,1)
    HDU_FIELD(f2,TYPE_STRING,2)
    HDU_FIELD(f3,TYPE_UINT32,3)
)

HATN_DB_INDEX(f1Idx,u1::f1)
HATN_DB_MODEL_WITH_CFG(model1,u1,ModelConfig{"model1"},f1Idx())

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void registerModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(model1());
#endif
}

void init()
{
    ModelRegistry::free();
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::free();
    rdb::RocksdbModels::free();
#endif
    registerModels();
}

template <typename ...Models>
auto initSchema(Models&& ...models)
{
    auto schema1=makeSchema("schema1",std::forward<Models>(models)...);

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::instance().registerSchema(schema1);
#endif

    return schema1;
}

template <typename T>
void setSchemaToClient(std::shared_ptr<Client> client, const T& schema)
{
    auto ec=client->setSchema(schema);
    BOOST_REQUIRE(!ec);
    auto s=client->schema();
    BOOST_REQUIRE(!s);
    BOOST_CHECK_EQUAL(s->get()->name(),schema->name());
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestTransaction)

BOOST_FIXTURE_TEST_CASE(Atomic, HATN_TEST_NAMESPACE::DbTestFixture)
{
    init();

    auto s1=initSchema(model1());

    auto handler=[&s1,this](std::shared_ptr<DbPlugin>&, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};

        // create objects
        auto o=makeInitObject<u1::type>();
        auto ec=client->create(topic1,model1(),&o);
        BOOST_REQUIRE(!ec);
        auto oid=o.fieldValue(object::_id);

        BOOST_TEST_MESSAGE("Increment object's field concurrently");
        auto incReq=update::request(update::field(u1::f1,update::inc,1),update::field(u1::f3,update::inc,10));
#ifdef BUILD_DEBUG
    size_t count=1000;
#else
    size_t count=10000;
#endif
        int jobs=8;
        std::atomic<size_t> doneCount{0};
        auto handler=[&,this](size_t idx)
        {
            auto logCtx=HATN_COMMON_NAMESPACE::makeTaskContext<HATN_LOGCONTEXT_NAMESPACE::ContextWrapper>();
            logCtx->beforeThreadProcessing();

            for (size_t i=0;i<count;i++)
            {
                auto ec=client->update(topic1,model1(),oid,incReq);
                if (ec)
                {
                    HATN_CTX_ERROR(ec,"failed to update");
                }
                BOOST_REQUIRE(!ec);
            }

            HATN_TEST_MESSAGE_TS(fmt::format("Done handler for thread {}",idx));
            if (++doneCount==jobs)
            {
                this->quit();
            }

            logCtx->afterThreadProcessing();
        };

        // run threads
        createThreads(jobs+1);
        thread(0)->start(false);
        for (int j=0;j<jobs;j++)
        {
            thread(j+1)->execAsync(
                [&handler,j]()
                {
                    handler(j);
                }
                );
            thread(j+1)->start(false);
        }
        exec(60);
        thread(0)->stop();
        for (int j=0;j<jobs;j++)
        {
            thread(j+1)->stop();
        }

        // check result
        auto r1=client->read(topic1,model1(),oid);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value()->fieldValue(u1::f1),count*jobs);
        BOOST_CHECK_EQUAL(r1.value()->fieldValue(u1::f3),count*jobs*10);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_FIXTURE_TEST_CASE(Rollback, HATN_TEST_NAMESPACE::DbTestFixture)
{
    init();

    auto s1=initSchema(model1());

    auto handler=[&s1,this](std::shared_ptr<DbPlugin>&, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};

        // create objects
        auto o1=makeInitObject<u1::type>();
        auto ec=client->create(topic1,model1(),&o1);
        BOOST_REQUIRE(!ec);
        auto oid1=o1.fieldValue(object::_id);
        auto o2=makeInitObject<u1::type>();
        o2.setFieldValue(u1::f3,100);
        auto oid2=o2.fieldValue(object::_id);
        auto o3=makeInitObject<u1::type>();
        o3.setFieldValue(u1::f2,"Hello!");
        ec=client->create(topic1,model1(),&o3);
        BOOST_REQUIRE(!ec);
        auto oid3=o3.fieldValue(object::_id);
        auto o4=makeInitObject<u1::type>();
        o4.setFieldValue(u1::f1,120);
        o4.setFieldValue(u1::f2,"Object 4");
        o4.setFieldValue(u1::f3,25);
        ec=client->create(topic1,model1(),&o4);
        BOOST_REQUIRE(!ec);
        auto oid4=o4.fieldValue(object::_id);
        auto o5=makeInitObjectPtr<u1::type>();
        o5->setFieldValue(u1::f1,700);
        auto oid5=o5->fieldValue(object::_id);
        auto o6=makeInitObject<u1::type>();
        o6.setFieldValue(u1::f2,"Would delete");
        auto oid6=o6.fieldValue(object::_id);
        ec=client->create(topic1,model1(),&o6);
        BOOST_REQUIRE(!ec);
        auto o7=makeInitObject<u1::type>();
        o7.setFieldValue(u1::f2,"Would delete many");
        auto oid7=o7.fieldValue(object::_id);
        ec=client->create(topic1,model1(),&o7);
        BOOST_REQUIRE(!ec);
        auto o8=makeInitObject<u1::type>();
        o8.setFieldValue(u1::f2,"Would update many 8");
        auto oid8=o8.fieldValue(object::_id);
        ec=client->create(topic1,model1(),&o8);
        BOOST_REQUIRE(!ec);
        auto o9=makeInitObject<u1::type>();
        o9.setFieldValue(u1::f2,"Would update many 9");
        auto oid9=o9.fieldValue(object::_id);
        ec=client->create(topic1,model1(),&o9);
        BOOST_REQUIRE(!ec);

        auto incReq1=update::request(update::field(u1::f1,update::inc,15),update::field(u1::f3,update::inc,16));
        auto setReq3=update::request(update::field(u1::f2,update::set,"Hi!"));
        auto setReq4=update::request(update::field(u1::f2,update::set,"Hi Object 4!"));
        auto setReq5=update::request(update::field(u1::f3,update::set,27));
        auto setReq6=update::request(update::field(u1::f1,update::set,240));
        auto setReq89=update::request(update::field(u1::f2,update::set,"Updated many"));

        int brokenPoint=0;
        auto tx1=[&](Transaction* tx) -> Error
        {
            auto ec=client->update(topic1,model1(),oid1,incReq1,tx);
            HATN_CHECK_EC(ec)

            if (brokenPoint==1)
            {
                return dbError(DbError::TX_ROLLBACK);
            }

            ec=client->create(topic1,model1(),&o2,tx);
            BOOST_REQUIRE(!ec);
            if (brokenPoint==2)
            {
                return dbError(DbError::TX_ROLLBACK);
            }

            auto r3=client->read(topic1,model1(),oid3,tx,true);
            BOOST_REQUIRE(!r3);
            BOOST_CHECK_EQUAL(std::string(r3.value()->fieldValue(u1::f2)),std::string("Hello!"));
            if (brokenPoint==3)
            {
                return dbError(DbError::TX_ROLLBACK);
            }

            ec=client->update(topic1,model1(),oid3,setReq3,tx);
            HATN_CHECK_EC(ec)

            ec=client->readUpdate(topic1,model1(),oid4,setReq4,update::ModifyReturn::After,tx);
            HATN_CHECK_EC(ec)

            auto q4=makeQuery(oidIdx(),query::where(object::_id,query::eq,oid4),topic1);
            auto r4=client->findUpdate(model1(),q4,setReq5,update::ModifyReturn::After,tx);
            HATN_CHECK_RESULT(r4)

            auto cb=[&](DbObject obj, Error& ec)
            {
                ec=client->update(topic1,model1(),obj.as<u1::type>()->fieldValue(object::_id),setReq6,tx);
                return true;
            };
            ec=client->findCb(model1(),q4,cb,tx,true);
            HATN_CHECK_EC(ec)

            auto q5=makeQuery(f1Idx(),query::where(u1::f1,query::eq,700),topic1);
            auto r5=client->findUpdateCreate(model1(),q5,setReq5,o5,update::ModifyReturn::After,tx);
            HATN_CHECK_RESULT(r5)

            ec=client->deleteObject(topic1,model1(),oid6,tx);
            HATN_CHECK_EC(ec)

            auto q7=makeQuery(oidIdx(),query::where(Oid,query::eq,oid7),topic1);
            ec=client->deleteMany(model1(),q7,tx);
            HATN_CHECK_EC(ec)

            std::vector<ObjectId> v89{oid8,oid9};
            auto q89=makeQuery(oidIdx(),query::where(Oid,query::in,v89),topic1);
            ec=client->updateMany(model1(),q89,setReq89,tx);
            HATN_CHECK_EC(ec)

            if (brokenPoint==4)
            {
                return dbError(DbError::TX_ROLLBACK);
            }

            return Error{OK};
        };

        // test broken transactions
        auto testBroken=[&](int p)
        {
            brokenPoint=p;
            ec=client->transaction(tx1);
            BOOST_CHECK(ec);
            BOOST_CHECK(ec.is(DbError::TX_ROLLBACK));
            auto r1=client->read(topic1,model1(),oid1);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1.value()->fieldValue(u1::f1),0);
            BOOST_CHECK_EQUAL(r1.value()->fieldValue(u1::f3),0);
            auto r2=client->read(topic1,model1(),oid2);
            BOOST_REQUIRE(r2);
            auto r3=client->read(topic1,model1(),oid3);
            BOOST_REQUIRE(!r3);
            BOOST_CHECK_EQUAL(std::string(r3.value()->fieldValue(u1::f2)),std::string("Hello!"));
            auto r4=client->read(topic1,model1(),oid4);
            BOOST_REQUIRE(!r4);
            BOOST_CHECK_EQUAL(r4.value()->fieldValue(u1::f1),120);
            BOOST_CHECK_EQUAL(std::string(r4.value()->fieldValue(u1::f2)),std::string("Object 4"));
            BOOST_CHECK_EQUAL(r4.value()->fieldValue(u1::f3),25);
            auto r5=client->read(topic1,model1(),oid5);
            BOOST_CHECK(r5);
            auto r6=client->read(topic1,model1(),oid6);
            BOOST_CHECK(!r6);
            BOOST_CHECK_EQUAL(std::string(r6.value()->fieldValue(u1::f2)),std::string("Would delete"));
            auto r7=client->read(topic1,model1(),oid7);
            BOOST_CHECK(!r7);
            BOOST_CHECK_EQUAL(std::string(r7.value()->fieldValue(u1::f2)),std::string("Would delete many"));
            auto r8=client->read(topic1,model1(),oid8);
            BOOST_REQUIRE(!r8);
            BOOST_CHECK_EQUAL(std::string(r8.value()->fieldValue(u1::f2)),std::string("Would update many 8"));
            auto r9=client->read(topic1,model1(),oid9);
            BOOST_REQUIRE(!r9);
            BOOST_CHECK_EQUAL(std::string(r9.value()->fieldValue(u1::f2)),std::string("Would update many 9"));
        };
        BOOST_TEST_CONTEXT("broken1"){testBroken(1);}
        BOOST_TEST_CONTEXT("broken2"){testBroken(2);}
        BOOST_TEST_CONTEXT("broken3"){testBroken(3);}
        BOOST_TEST_CONTEXT("broken4"){testBroken(4);}

        // test good transaction
        brokenPoint=0;
        ec=client->transaction(tx1);
        BOOST_REQUIRE(!ec);
        auto r1=client->read(topic1,model1(),oid1);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value()->fieldValue(u1::f1),15);
        BOOST_CHECK_EQUAL(r1.value()->fieldValue(u1::f3),16);
        auto r2=client->read(topic1,model1(),oid2);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value()->fieldValue(u1::f3),100);
        auto r3=client->read(topic1,model1(),oid3);
        BOOST_REQUIRE(!r3);
        BOOST_CHECK_EQUAL(std::string(r3.value()->fieldValue(u1::f2)),std::string("Hi!"));
        auto r4=client->read(topic1,model1(),oid4);
        BOOST_REQUIRE(!r4);
        BOOST_CHECK_EQUAL(r4.value()->fieldValue(u1::f1),240);
        BOOST_CHECK_EQUAL(std::string(r4.value()->fieldValue(u1::f2)),std::string("Hi Object 4!"));
        BOOST_CHECK_EQUAL(r4.value()->fieldValue(u1::f3),27);
        auto r5=client->read(topic1,model1(),oid5);
        BOOST_REQUIRE(!r5);
        BOOST_CHECK_EQUAL(r5.value()->fieldValue(u1::f1),700);
        auto r6=client->read(topic1,model1(),oid6);
        BOOST_CHECK(r6);
        auto r7=client->read(topic1,model1(),oid7);
        BOOST_CHECK(r7);
        auto r8=client->read(topic1,model1(),oid8);
        BOOST_REQUIRE(!r8);
        BOOST_CHECK_EQUAL(std::string(r8.value()->fieldValue(u1::f2)),std::string("Updated many"));
        auto r9=client->read(topic1,model1(),oid9);
        BOOST_REQUIRE(!r9);
        BOOST_CHECK_EQUAL(std::string(r9.value()->fieldValue(u1::f2)),std::string("Updated many"));
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_FIXTURE_TEST_CASE(Concurrent, HATN_TEST_NAMESPACE::DbTestFixture)
{
    init();

    auto s1=initSchema(model1());

    auto handler=[&s1,this](std::shared_ptr<DbPlugin>&, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};

        // create objects
        auto o=makeInitObject<u1::type>();
        auto ec=client->create(topic1,model1(),&o);
        BOOST_REQUIRE(!ec);
        auto oid=o.fieldValue(object::_id);

        BOOST_TEST_MESSAGE("Increment object's field concurrently");
        size_t count=10000;
        int jobs=8;
        std::atomic<size_t> doneCount{0};
        auto handler=[&,this](size_t idx)
        {
            auto logCtx=HATN_COMMON_NAMESPACE::makeTaskContext<HATN_LOGCONTEXT_NAMESPACE::ContextWrapper>();
            logCtx->beforeThreadProcessing();

            for (size_t i=0;i<count;i++)
            {
                auto tx1=[&](Transaction* tx) -> Error
                {
                    auto r=client->read(topic1,model1(),oid,tx,true);
                    HATN_CHECK_RESULT(r);
                    auto current=r.value()->fieldValue(u1::f3);

                    auto incReq=update::request(update::field(u1::f1,update::inc,1),
                                                update::field(u1::f3,update::set,current+10)
                                               );
                    auto ec=client->update(topic1,model1(),oid,incReq,tx);
                    HATN_CHECK_EC(ec);
                    return Error{OK};
                };
                ec=client->transaction(tx1);
                BOOST_REQUIRE(!ec);
            }

            HATN_TEST_MESSAGE_TS(fmt::format("Done handler for thread {}",idx));
            if (++doneCount==jobs)
            {
                this->quit();
            }

            logCtx->afterThreadProcessing();
        };

        // run threads
        createThreads(jobs+1);
        thread(0)->start(false);
        for (int j=0;j<jobs;j++)
        {
            thread(j+1)->execAsync(
                [&handler,j]()
                {
                    handler(j);
                }
                );
            thread(j+1)->start(false);
        }
        exec(60);
        thread(0)->stop();
        for (int j=0;j<jobs;j++)
        {
            thread(j+1)->stop();
        }

        // check result
        auto r1=client->read(topic1,model1(),oid);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value()->fieldValue(u1::f1),count*jobs);
        BOOST_CHECK_EQUAL(r1.value()->fieldValue(u1::f3),count*jobs*10);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
