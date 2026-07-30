/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testdeletecreate.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

#include <hatn/test/multithreadfixture.h>

#include <hatn/db/schema.h>

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
#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING
HATN_LOGCONTEXT_USING

namespace {

HDU_UNIT_WITH(dc1,(HDU_BASE(object)),
    HDU_FIELD(f1,TYPE_UINT32,1)
    HDU_FIELD(f2,TYPE_STRING,2)
    HDU_FIELD(f3,TYPE_INT64,3)
    HDU_FIELD(f4,TYPE_DATETIME,4)
    HDU_FIELD(marker,TYPE_STRING,5)
)

HATN_DB_INDEX(dc1_f1_idx,dc1::f1)
HATN_DB_INDEX(dc1_f2_idx,dc1::f2)
HATN_DB_INDEX(dc1_f3_idx,dc1::f3)
HATN_DB_INDEX(dc1_f4_idx,dc1::f4)
HATN_DB_INDEX(dc1_f1_f2_idx,dc1::f1,dc1::f2)

HATN_DB_MODEL(modelDc1,dc1,
              dc1_f1_idx(),
              dc1_f2_idx(),
              dc1_f3_idx(),
              dc1_f4_idx(),
              dc1_f1_f2_idx()
             )

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void registerModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(modelDc1());
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
    auto schema1=makeSchema("schema_deletecreate",std::forward<Models>(models)...);

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

// index field values shared by the deleted object and by its replacement,
// so that after the swap the only way to tell them apart is oid and marker
const uint32_t IdxF1=1000;
const std::string IdxF2{"shared index string"};
const int64_t IdxF3=-777777;

const std::string MarkerOld{"old object"};
const std::string MarkerNew{"new object"};

enum class DeleteMode : int
{
    ByOid,
    ByQuery,
    ByQueryBulk
};

auto makeDc1Object(const common::DateTime& dt, const std::string& marker)
{
    auto o=makeInitObject<dc1::type>();
    o.setFieldValue(dc1::f1,IdxF1);
    o.setFieldValue(dc1::f2,IdxF2);
    o.setFieldValue(dc1::f3,IdxF3);
    o.setFieldValue(dc1::f4,dt);
    o.setFieldValue(dc1::marker,marker);
    return o;
}

auto makeF1Query(Topic topic)
{
    return makeQuery(dc1_f1_idx(),query::where(dc1::f1,query::eq,IdxF1),topic);
}

// Checks that the shared index values resolve to exactly one object, through
// every index of the model, that it is the expected object, and that
// goneOid (when different) is nowhere to be found any more, including its
// own oid index entry.
//
// Note: count(model,topic) is deliberately never used here. It is backed by
// ModelTopics whose counters are merged directly on the DB outside of the
// transaction, so they are not rolled back together with the rest of the
// write batch. Only the index-query count(model,query) overload and
// find(...)/findAll(...) sizes are transaction-consistent.
void checkOnlyObject(std::shared_ptr<Client> client,
                     Topic topic,
                     const common::DateTime& dt,
                     const ObjectId& expectedOid,
                     const std::string& expectedMarker,
                     const ObjectId& goneOid)
{
    auto q1=makeF1Query(topic);

    // find by the index used before the transaction (requirement step 2/4)
    auto r1=client->find(modelDc1(),q1);
    if (r1)
    {
        BOOST_TEST_MESSAGE(r1.error().message());
    }
    BOOST_REQUIRE(!r1);
    BOOST_REQUIRE_EQUAL(r1->size(),1u);
    const auto* found=r1->at(0).unit<dc1::type>();
    BOOST_CHECK(found->fieldValue(object::_id)==expectedOid);
    BOOST_CHECK_EQUAL(found->fieldValue(dc1::marker),expectedMarker);
    BOOST_CHECK_EQUAL(found->fieldValue(dc1::f1),IdxF1);
    BOOST_CHECK_EQUAL(found->fieldValue(dc1::f2),IdxF2);
    BOOST_CHECK_EQUAL(found->fieldValue(dc1::f3),IdxF3);

    // findOne by the same index
    auto r2=client->findOne(modelDc1(),q1);
    BOOST_REQUIRE(!r2);
    BOOST_REQUIRE(!r2.value().isNull());
    BOOST_CHECK(r2.value()->fieldValue(object::_id)==expectedOid);
    BOOST_CHECK_EQUAL(r2.value()->fieldValue(dc1::marker),expectedMarker);

    // count by the same index
    auto r3=client->count(modelDc1(),q1);
    BOOST_REQUIRE(!r3);
    BOOST_CHECK_EQUAL(r3.value(),1u);

    // read the expected object by oid
    auto r4=client->read(topic,modelDc1(),expectedOid);
    BOOST_REQUIRE(!r4);
    BOOST_CHECK_EQUAL(r4.value()->fieldValue(dc1::marker),expectedMarker);

    if (goneOid!=expectedOid)
    {
        // the deleted object must not be readable any more
        auto r5=client->read(topic,modelDc1(),goneOid);
        BOOST_CHECK(r5);

        // ... and its oid index entry must be gone as well
        auto q6=makeQuery(oidIdx(),query::where(Oid,query::eq,goneOid),topic);
        auto r6=client->find(modelDc1(),q6);
        BOOST_REQUIRE(!r6);
        BOOST_CHECK(r6->empty());
    }

    // every other index of the model must resolve to the same single object,
    // proving that all index entries were rewritten, not just the one above
    auto checkIdx=[&](auto&& q)
    {
        auto r=client->find(modelDc1(),q);
        BOOST_REQUIRE(!r);
        BOOST_REQUIRE_EQUAL(r->size(),1u);
        const auto* o=r->at(0).template unit<dc1::type>();
        BOOST_CHECK(o->fieldValue(object::_id)==expectedOid);
        BOOST_CHECK_EQUAL(o->fieldValue(dc1::marker),expectedMarker);
    };
    checkIdx(makeQuery(dc1_f2_idx(),query::where(dc1::f2,query::eq,IdxF2),topic));
    checkIdx(makeQuery(dc1_f3_idx(),query::where(dc1::f3,query::eq,IdxF3),topic));
    checkIdx(makeQuery(dc1_f4_idx(),query::where(dc1::f4,query::eq,dt),topic));
    checkIdx(makeQuery(dc1_f1_f2_idx(),
                       query::where(dc1::f1,query::eq,IdxF1).and_(dc1::f2,query::eq,IdxF2),
                       topic));

    // and there must be exactly one object in the topic at all
    auto r7=client->findAll(topic,modelDc1());
    BOOST_REQUIRE(!r7);
    BOOST_CHECK_EQUAL(r7->size(),1u);
}

// Shared body: create an object, find it by index, then within a single
// transaction delete it (by oid, by query, or by bulk query, depending on
// mode) and create a replacement carrying the same indexed field values
// (optionally reusing the same object id), then find by the same index
// again and check that only the replacement is found.
void runDeleteCreate(DeleteMode mode, bool sameOid)
{
    init();
    auto s1=initSchema(modelDc1());

    auto handler=[mode,sameOid,&s1](std::shared_ptr<DbPlugin>, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};
        auto dt=common::DateTime::currentUtc();

        // 1. create the original object with a few indexes
        auto o1=makeDc1Object(dt,MarkerOld);
        auto ec=client->create(topic1,modelDc1(),&o1);
        BOOST_REQUIRE(!ec);
        auto oid1=o1.fieldValue(object::_id);

        // 2. find it by one of its indexes
        checkOnlyObject(client,topic1,dt,oid1,MarkerOld,oid1);

        // 3. delete it and create a new object with the same indexes
        //    within the same transaction
        auto o2=makeDc1Object(dt,MarkerNew);
        if (sameOid)
        {
            o2.setFieldValue(object::_id,oid1);
        }
        auto oid2=o2.fieldValue(object::_id);
        BOOST_CHECK(sameOid ? (oid2==oid1) : (oid2!=oid1));

        auto tx1=[&](Transaction* tx) -> Error
        {
            switch (mode)
            {
                case (DeleteMode::ByOid):
                {
                    auto ec=client->deleteObject(topic1,modelDc1(),oid1,tx);
                    HATN_CHECK_EC(ec)
                }
                break;

                case (DeleteMode::ByQuery):
                {
                    auto rDel=client->deleteMany(modelDc1(),makeF1Query(topic1),tx);
                    HATN_CHECK_RESULT(rDel)
                    BOOST_CHECK_EQUAL(rDel.value(),1u);
                }
                break;

                case (DeleteMode::ByQueryBulk):
                {
                    auto rDel=client->deleteManyBulk(modelDc1(),makeF1Query(topic1),tx);
                    HATN_CHECK_RESULT(rDel)
                    BOOST_CHECK_EQUAL(rDel.value(),1u);
                }
                break;
            }

            // Note: index searches (find/findOne/count) are not transaction
            // aware, only read() is. Use read() to check in-tx state.
            if (!sameOid)
            {
                auto rGone=client->read(topic1,modelDc1(),oid1,tx);
                BOOST_CHECK(rGone);
            }

            auto ec=client->create(topic1,modelDc1(),&o2,tx);
            HATN_CHECK_EC(ec)

            auto rNew=client->read(topic1,modelDc1(),oid2,tx);
            BOOST_REQUIRE(!rNew);
            BOOST_CHECK_EQUAL(rNew.value()->fieldValue(dc1::marker),MarkerNew);

            return Error{OK};
        };
        ec=client->transaction(tx1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // 4. find by the same index as in step 2: must return exactly the
        //    new object
        checkOnlyObject(client,topic1,dt,oid2,MarkerNew,oid1);

        if (sameOid)
        {
            auto rNew=client->read(topic1,modelDc1(),oid1);
            BOOST_REQUIRE(!rNew);
            BOOST_CHECK_EQUAL(rNew.value()->fieldValue(dc1::marker),MarkerNew);
            BOOST_CHECK(rNew.value()->fieldValue(object::created_at)==o2.fieldValue(object::created_at));
        }
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestDeleteCreate, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(DeleteObjectCreateInTx)
{
    HATN_CTX_SCOPE("DeleteObjectCreateInTx")
    runDeleteCreate(DeleteMode::ByOid,false);
}

BOOST_AUTO_TEST_CASE(DeleteManyCreateInTx)
{
    HATN_CTX_SCOPE("DeleteManyCreateInTx")
    runDeleteCreate(DeleteMode::ByQuery,false);
}

BOOST_AUTO_TEST_CASE(DeleteManyBulkCreateInTx)
{
    HATN_CTX_SCOPE("DeleteManyBulkCreateInTx")
    runDeleteCreate(DeleteMode::ByQueryBulk,false);
}

BOOST_AUTO_TEST_CASE(SameObjectIdInTx)
{
    HATN_CTX_SCOPE("SameObjectIdInTx")
    runDeleteCreate(DeleteMode::ByOid,true);
}

BOOST_AUTO_TEST_CASE(RollbackDeleteCreateInTx)
{
    HATN_CTX_SCOPE("RollbackDeleteCreateInTx")

    init();
    auto s1=initSchema(modelDc1());

    auto handler=[&s1](std::shared_ptr<DbPlugin>, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};
        auto dt=common::DateTime::currentUtc();

        // create the original object
        auto o1=makeDc1Object(dt,MarkerOld);
        auto ec=client->create(topic1,modelDc1(),&o1);
        BOOST_REQUIRE(!ec);
        auto oid1=o1.fieldValue(object::_id);

        // find it by one of its indexes
        checkOnlyObject(client,topic1,dt,oid1,MarkerOld,oid1);

        // delete it and create a replacement with the same index values in
        // one transaction, but then abort the transaction
        auto o2=makeDc1Object(dt,MarkerNew);
        auto oid2=o2.fieldValue(object::_id);

        auto tx1=[&](Transaction* tx) -> Error
        {
            auto delEc=client->deleteObject(topic1,modelDc1(),oid1,tx);
            HATN_CHECK_EC(delEc)

            auto createEc=client->create(topic1,modelDc1(),&o2,tx);
            HATN_CHECK_EC(createEc)

            return dbError(DbError::TX_ROLLBACK);
        };
        ec=client->transaction(tx1);
        BOOST_REQUIRE(ec);
        BOOST_CHECK(ec.is(DbError::TX_ROLLBACK));

        // after rollback, find by the same index must still resolve to the
        // original object: the transaction's delete must have been undone
        checkOnlyObject(client,topic1,dt,oid1,MarkerOld,oid2);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
