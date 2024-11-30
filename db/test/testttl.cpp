/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testttl.cpp
*/

/****************************************************************************/

#include <cmath>
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include <hatn/base/configtreeloader.h>
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
    HDU_FIELD(f0,TYPE_DATETIME,1)
    HDU_FIELD(f1,TYPE_DATETIME,2)
    HDU_FIELD(f2,TYPE_UINT32,3)
)

HATN_DB_TTL_INDEX(f0Idx,5,u1::f0)
HATN_DB_TTL_INDEX(f1Idx,5,u1::f1)
HATN_DB_INDEX(f2Idx,u1::f2)
HATN_DB_MODEL(model1,u1,f0Idx(),f1Idx(),f2Idx())

HATN_DB_INDEX(f0Idx2,u1::f0)
HATN_DB_MODEL_WITH_CFG(model2,u1,ModelConfig{"model2"},f0Idx2())

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void registerModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(model1());
    rdb::RocksdbModels::instance().registerModel(model2());
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

BOOST_AUTO_TEST_SUITE(TestTtl)

BOOST_FIXTURE_TEST_CASE(TtlOperations, HATN_TEST_NAMESPACE::DbTestFixture)
{
    init();

    auto s1=initSchema(model1());

    auto handler=[&s1,this](std::shared_ptr<DbPlugin>&, std::shared_ptr<Client> client)
    {
        size_t count=20;
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};
        std::vector<ObjectId> oids;

        // fill db
        for (size_t i=0;i<count;i++)
        {
            auto o=makeInitObject<u1::type>();
            auto dt=common::DateTime::currentUtc();
            if (i>=(count/4))
            {
                dt.addSeconds(5);
            }
            o.setFieldValue(u1::f2,static_cast<uint32_t>(i));
            o.setFieldValue(u1::f1,dt);
            dt.addSeconds(15);
            o.setFieldValue(u1::f0,dt);
            auto ec=client->create(topic1,model1(),&o);
            BOOST_REQUIRE(!ec);
            oids.emplace_back(o.fieldValue(object::_id));

            // test updating ttl field
            if (i==2)
            {
                HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value()->setStackLockingEnabled(true);

                auto dt1=common::DateTime::currentUtc();
                dt1.addSeconds(50);
                auto updateReq=update::request(update::field(u1::f1,update::set,dt1));
                auto ec=client->update(topic1,model1(),oids[i],updateReq);
                if (ec)
                {
                    HATN_CTX_ERROR(ec,"failed to update TTL")
                    HATN_CTX_SCOPE_UNLOCK()
                }
                BOOST_REQUIRE(!ec);

                HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value()->setStackLockingEnabled(false);
            }
            // update another TTL field, ensure that least ttl is used
            if (i==count-5)
            {
                auto dt1=common::DateTime::currentUtc();
                auto updateReq=update::request(update::field(u1::f0,update::set,dt1));
                auto ec=client->update(topic1,model1(),oids[i],updateReq);
                BOOST_REQUIRE(!ec);
            }
        }

        // read before ttl
        auto q1=makeQuery(f1Idx(),query::where(u1::f1,query::gte,query::First),topic1);
        auto r1=client->find(model1(),q1);
        BOOST_CHECK(!r1);
        BOOST_CHECK_EQUAL(r1->size(),count);

        // wait for 7 seconds
        BOOST_TEST_MESSAGE("Waiting for 7 seconds...");
        exec(7);

        // read after ttl
        r1=client->find(model1(),q1);
        BOOST_CHECK(!r1);
        BOOST_CHECK_EQUAL(r1->size(),3*count/4);

        auto r2=client->read(topic1,model1(),oids[1]);
        BOOST_REQUIRE(r2);
        BOOST_CHECK(r2.error().is(DbError::EXPIRED));
        auto r3=client->read(topic1,model1(),oids[oids.size()-1]);
        BOOST_REQUIRE(!r3);

        // close and re-open database
        auto ec=client->closeDb();
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("failed to close database: {}",ec.message()));
        }
        BOOST_REQUIRE(!ec);
        base::config_object::LogRecords records;
        ec=client->openDb(*PrepareDbAndRun::currentCfg,records);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("failed to re-open database: {}",ec.message()));
        }
        BOOST_REQUIRE(!ec);

        // read after reopen
        r1=client->find(model1(),q1);
        BOOST_CHECK(!r1);
        BOOST_CHECK_EQUAL(r1->size(),3*count/4);

        // wait for 3 seconds
        BOOST_TEST_MESSAGE("Waiting for 3 seconds...");
        exec(3);

        // close and re-open database
        ec=client->closeDb();
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("failed to close database: {}",ec.message()));
        }
        BOOST_REQUIRE(!ec);
        ec=client->openDb(*PrepareDbAndRun::currentCfg,records);
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("failed to re-open database: {}",ec.message()));
        }
        BOOST_REQUIRE(!ec);

        // read after reopen 2
        r1=client->find(model1(),q1);
        BOOST_CHECK(!r1);
        BOOST_CHECK_EQUAL(r1->size(),3*count/4);

        r2=client->read(topic1,model1(),oids[1]);
        BOOST_REQUIRE(r2);
        BOOST_CHECK(r2.error().is(DbError::NOT_FOUND));
        r3=client->read(topic1,model1(),oids[oids.size()-1]);
        BOOST_REQUIRE(!r3);

        // clear all
        auto r4=client->deleteMany(model1(),q1);
        BOOST_CHECK(!r4);
        auto r5=client->count(model1(),q1);
        BOOST_CHECK(!r5);
        BOOST_CHECK_EQUAL(r5.value(),0);

        // add object with TTL fields not set
        auto o6=makeInitObject<u1::type>();
        o6.setFieldValue(u1::f2,static_cast<uint32_t>(100));
        ec=client->create(topic1,model1(),&o6);
        BOOST_REQUIRE(!ec);
        // wait for 2 seconds
        BOOST_TEST_MESSAGE("Waiting for 2 seconds...");
        exec(2);
        auto q7=makeQuery(createdAtIdx(),query::where(object::created_at,query::gte,query::First),topic1);
        r5=client->count(model1(),q7);
        BOOST_CHECK(!r5);
        BOOST_CHECK_EQUAL(r5.value(),1);
        r1=client->find(model1(),q7);
        BOOST_CHECK(!r1);
        BOOST_REQUIRE_EQUAL(r1->size(),1);
        BOOST_CHECK_EQUAL(r1->at(0).as<u1::type>()->fieldValue(u1::f2),100);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_FIXTURE_TEST_CASE(TimeFilter, HATN_TEST_NAMESPACE::DbTestFixture)
{
    auto tpInt1=query::makeInterval(uint32_t(10),uint32_t(100));
    BOOST_CHECK(tpInt1.contains(50));

    auto tpInt2=query::Interval<uint32_t>(10,query::IntervalType::Closed,100,query::IntervalType::Open);
    BOOST_CHECK(!tpInt2.contains(1));
    BOOST_CHECK(tpInt2.contains(50));
    BOOST_CHECK(tpInt2.contains(10));
    BOOST_CHECK(!tpInt2.contains(100));
    BOOST_CHECK(!tpInt2.contains(200));

    auto tpInt3=query::Interval<uint32_t>(10,query::IntervalType::Open,100,query::IntervalType::Open);
    BOOST_CHECK(!tpInt3.contains(1));
    BOOST_CHECK(tpInt3.contains(50));
    BOOST_CHECK(!tpInt3.contains(10));
    BOOST_CHECK(!tpInt3.contains(100));
    BOOST_CHECK(!tpInt3.contains(200));

    auto tpInt4=query::Interval<uint32_t>(10,query::IntervalType::Closed,100,query::IntervalType::Closed);
    BOOST_CHECK(!tpInt4.contains(1));
    BOOST_CHECK(tpInt4.contains(50));
    BOOST_CHECK(tpInt4.contains(10));
    BOOST_CHECK(tpInt4.contains(100));
    BOOST_CHECK(!tpInt4.contains(200));

    auto tpInt5=query::Interval<uint32_t>(10,query::IntervalType::Open,100,query::IntervalType::Closed);
    BOOST_CHECK(!tpInt5.contains(1));
    BOOST_CHECK(tpInt5.contains(50));
    BOOST_CHECK(!tpInt5.contains(10));
    BOOST_CHECK(tpInt5.contains(100));
    BOOST_CHECK(!tpInt5.contains(200));

    auto tpInt6=query::Interval<uint32_t>(10,query::IntervalType::Open,100,query::IntervalType::Open);
    BOOST_CHECK(!tpInt6.contains(1));
    BOOST_CHECK(tpInt6.contains(50));
    BOOST_CHECK(!tpInt6.contains(10));
    BOOST_CHECK(!tpInt6.contains(100));
    BOOST_CHECK(!tpInt6.contains(200));

    auto tpInt7=query::Interval<uint32_t>(10,query::IntervalType::First,100,query::IntervalType::Open);
    BOOST_CHECK(tpInt7.contains(1));
    BOOST_CHECK(tpInt7.contains(50));
    BOOST_CHECK(tpInt7.contains(10));
    BOOST_CHECK(!tpInt7.contains(100));
    BOOST_CHECK(!tpInt7.contains(200));

    auto tpInt8=query::Interval<uint32_t>(10,query::IntervalType::First,100,query::IntervalType::Closed);
    BOOST_CHECK(tpInt8.contains(1));
    BOOST_CHECK(tpInt8.contains(50));
    BOOST_CHECK(tpInt8.contains(10));
    BOOST_CHECK(tpInt8.contains(100));
    BOOST_CHECK(!tpInt8.contains(200));

    auto tpInt9=query::Interval<uint32_t>(10,query::IntervalType::Open,100,query::IntervalType::Last);
    BOOST_CHECK(!tpInt9.contains(1));
    BOOST_CHECK(tpInt9.contains(50));
    BOOST_CHECK(!tpInt9.contains(10));
    BOOST_CHECK(tpInt9.contains(100));
    BOOST_CHECK(tpInt9.contains(200));

    auto tpInt10=query::Interval<uint32_t>(10,query::IntervalType::Closed,100,query::IntervalType::Last);
    BOOST_CHECK(!tpInt10.contains(1));
    BOOST_CHECK(tpInt10.contains(50));
    BOOST_CHECK(tpInt10.contains(10));
    BOOST_CHECK(tpInt10.contains(100));
    BOOST_CHECK(tpInt10.contains(200));

    init();

    auto s1=initSchema(model2());

    auto handler=[&s1](std::shared_ptr<DbPlugin>&, std::shared_ptr<Client> client)
    {
        size_t count=20;
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};
        std::vector<ObjectId> oids;

        // fill db
        std::set<size_t> checkVec;
        for (size_t i=0;i<count;i++)
        {
            checkVec.insert(i);
            auto o=makeInitObject<u1::type>();
            if ((i>=5 && i<10) || (i>=15 && i<20))
            {
                auto dt=common::DateTime::currentUtc();
                dt.addDays(-i);
                dt.addSeconds(60);
                o.setFieldValue(object::created_at,dt);
                if (i==5 || (i>=15 && i<20))
                {
                    checkVec.erase(i);
                }
            }
            o.setFieldValue(u1::f2,static_cast<uint32_t>(i));
            auto ec=client->create(topic1,model2(),&o);
            BOOST_REQUIRE(!ec);
            oids.emplace_back(o.fieldValue(object::_id));
#if 0
            std::cout<<"i="<<i<<" created_at="<< o.fieldValue(object::created_at).toEpoch() <<std::endl;
#endif
        }

        // read without timepoint filter
        BOOST_TEST_MESSAGE("read without timepoint filter");
        auto q1=makeQuery(createdAtIdx(),query::where(object::created_at,query::gte,query::First),topic1);
        auto r1=client->find(model2(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1->size(),count);
#if 0
        for (size_t i=0;i<r1->size();i++)
        {
            std::cout<<"i="<<i<<":\n"
            <<   r1->at(i).get()->toString(true)
            <<std::endl;
        }
#endif
        // read with timepoint filter
        BOOST_TEST_MESSAGE("read with timepoint filter");
        auto from1=common::DateTime::currentUtc();
        from1.addDays(-21);
        auto to1=common::DateTime::currentUtc();
        to1.addDays(-14);
        auto from2=common::DateTime::currentUtc();
        from2.addDays(-5);
        auto to2=common::DateTime::currentUtc();
        to2.addDays(-4);
        auto tpIntervals=makeTpIntervals(
                tpInterval(from1,to1),
                tpInterval(from2,to2)
            );
        auto q2=makeQuery(oidIdx(),query::where(object::_id,query::gte,query::First),topic1);
        q2.setFilterTimePoints(tpIntervals);
        r1=client->find(model2(),q2);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1->size(),checkVec.size());
        auto it=checkVec.begin();
        for (size_t i=0;i<r1->size();i++)
        {
#if 0
            std::cout<<"i="<<i<<":\n"
                      <<   r1->at(i).get()->toString(true)
                      <<std::endl;
#endif
            BOOST_CHECK_EQUAL(r1->at(i).as<u1::type>()->fieldValue(u1::f2),*it);
            ++it;
        }
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
