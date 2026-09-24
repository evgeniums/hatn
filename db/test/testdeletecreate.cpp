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
#include <hatn/common/threadwithqueue.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/taskcontext.h>

#include <hatn/test/multithreadfixture.h>

#include <hatn/db/schema.h>
#include <hatn/db/asyncclient.h>

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

// Mirrors whitemclient::files2's upload_file_item: a plain (non-object-base)
// subunit type embedded in a repeated field of the parent unit below.
HDU_UNIT(dc1_item,
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(state,TYPE_UINT32,2)
)

HDU_UNIT_WITH(dc1,(HDU_BASE(object)),
    HDU_FIELD(f1,TYPE_UINT32,1)
    HDU_FIELD(f2,TYPE_STRING,2)
    HDU_FIELD(f3,TYPE_INT64,3)
    HDU_FIELD(f4,TYPE_DATETIME,4)
    HDU_FIELD(marker,TYPE_STRING,5)
    // Mirrors whitemclient::files2::UploadQueue's upload_batch::sending flag:
    // a boolean that is part of a non-unique composite index and that flips
    // value exactly across a delete+create-in-one-tx replace (claimed=true
    // when the row is deleted, released=false on the replacement). None of
    // the other indexed fields above ever change value between the deleted
    // object and its replacement, so they cannot catch a bug that is
    // specific to an indexed value actually changing across the swap.
    HDU_FIELD(flag,TYPE_BOOL,6)
    // Second datetime field, paired with flag in a SECOND composite index
    // below - mirrors upload_batch::valid_till, which (together with
    // upload_batch::sending) forms uploadClaimIdx, a distinct index from
    // uploadReadyIdx(sending,next_attempt_at) that also keys on `sending`.
    HDU_FIELD(f5,TYPE_DATETIME,7)
    // Mirrors upload_batch::files: a repeated subunit field. dc1 had no
    // repeated fields until now, and cloneBatchWithItemChanges()/
    // replaceBatch() (the delete+create pattern under test) predate
    // db::update() support for fields nested inside repeated-subunit
    // elements (now covered by TestUpdateRepeatedSub) - so a repeated field
    // being present on the object is the one structural aspect of
    // upload_batch that no prior variant of this test has exercised. That
    // gap in update() is unrelated to what this test actually verifies
    // (index rewriting across a delete+create swap), so the pattern and
    // this test both remain valid on their own merits.
    HDU_REPEATED_FIELD(items,dc1_item::TYPE,8)
)

HATN_DB_INDEX(dc1_f1_idx,dc1::f1)
HATN_DB_INDEX(dc1_f2_idx,dc1::f2)
HATN_DB_INDEX(dc1_f3_idx,dc1::f3)
HATN_DB_INDEX(dc1_f4_idx,dc1::f4)
HATN_DB_INDEX(dc1_f1_f2_idx,dc1::f1,dc1::f2)
// Same shape as whitemclient::files2's uploadReadyIdx(sending,next_attempt_at):
// a non-unique composite index of (bool, datetime).
HATN_DB_INDEX(dc1_flag_f4_idx,dc1::flag,dc1::f4)
// Same shape as whitemclient::files2's uploadClaimIdx(sending,valid_till):
// a SECOND non-unique composite index that also keys on `flag`, paired with
// a different datetime field - reproduces upload_batch having `sending` in
// two distinct composite indexes simultaneously.
HATN_DB_INDEX(dc1_flag_f5_idx,dc1::flag,dc1::f5)

HATN_DB_MODEL(modelDc1,dc1,
              dc1_f1_idx(),
              dc1_f2_idx(),
              dc1_f3_idx(),
              dc1_f4_idx(),
              dc1_f1_f2_idx(),
              dc1_flag_f4_idx(),
              dc1_flag_f5_idx()
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

auto makeDc1Object(const common::DateTime& dt, const std::string& marker, bool flag=true)
{
    auto o=makeInitObject<dc1::type>();
    o.setFieldValue(dc1::f1,IdxF1);
    o.setFieldValue(dc1::f2,IdxF2);
    o.setFieldValue(dc1::f3,IdxF3);
    o.setFieldValue(dc1::f4,dt);
    o.setFieldValue(dc1::marker,marker);
    o.setFieldValue(dc1::flag,flag);
    o.setFieldValue(dc1::f5,dt);
    // Same appendSharedSubunit() pattern UploadQueue::enqueue()/
    // cloneBatchWithItemChanges() use to populate upload_batch::files.
    auto item=o.mutableField(dc1::items).appendSharedSubunit();
    item.setFieldValue(dc1_item::name,marker);
    item.setFieldValue(dc1_item::state,IdxF1);
    return o;
}

auto makeF1Query(Topic topic)
{
    return makeQuery(dc1_f1_idx(),query::where(dc1::f1,query::eq,IdxF1),topic);
}

// Same shape as UploadQueue::processTopic()'s uploadReadyIdx claim query
// (where(sending,eq,false).and_(next_attempt_at,lte,now)): a range
// comparison on the datetime field, not an exact match - `boundary` should
// be at or after the object's own f4 value for the row to match, exactly
// like `now` is always at or after a not-yet-due `next_attempt_at`.
auto makeReadyQuery(Topic topic, const common::DateTime& boundary, bool flag)
{
    return makeQuery(dc1_flag_f4_idx(),
                      query::where(dc1::flag,query::eq,flag).and_(dc1::f4,query::lte,boundary),
                      topic);
}

// Same shape as UploadQueue's uploadClaimIdx(sending,valid_till) - a SECOND
// composite index that also keys on `flag`, alongside dc1_flag_f4_idx.
auto makeClaimQuery(Topic topic, const common::DateTime& boundary, bool flag)
{
    return makeQuery(dc1_flag_f5_idx(),
                      query::where(dc1::flag,query::eq,flag).and_(dc1::f5,query::lte,boundary),
                      topic);
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

// Reproduces whitemclient::files2::UploadQueue::cloneBatchWithItemChanges()/
// replaceBatch(): delete an object by oid and create its replacement with
// the SAME oid in one transaction, same as runDeleteCreate(ByOid,sameOid=true)
// - but here the replacement flips the value of a field (flag) that is part
// of TWO non-unique composite indexes simultaneously (dc1_flag_f4_idx and
// dc1_flag_f5_idx, matching upload_batch's uploadReadyIdx(sending,
// next_attempt_at) and uploadClaimIdx(sending,valid_till) both keying on
// `sending`), and both queries use a range (lte) comparison on the datetime
// field rather than an exact match, exactly like the real claim query
// (sending==false && next_attempt_at<=now). None of this is exercised by
// runDeleteCreate(), where every indexed field keeps the same value across
// the swap and only a single, non-shared index is queried. This is exactly
// what UploadQueue does: the claimed row (sending=true) is deleted and
// replaced by a released row (sending=false) with the same _id, then the
// very next claim query (sending==false && next_attempt_at<=now) must find
// it.
void runDeleteCreateFlagFlip()
{
    init();
    auto s1=initSchema(modelDc1());

    auto handler=[&s1](std::shared_ptr<DbPlugin>, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};
        auto dt=common::DateTime::currentUtc();
        auto boundary=dt;
        boundary.addSeconds(5);

        // 1. create the original object with flag=true
        auto o1=makeDc1Object(dt,MarkerOld,true);
        auto ec=client->create(topic1,modelDc1(),&o1);
        BOOST_REQUIRE(!ec);
        auto oid1=o1.fieldValue(object::_id);

        // 2. sanity: findable under flag==true through BOTH composite
        //    indexes, not under flag==false through either
        {
            auto rReadyTrue=client->find(modelDc1(),makeReadyQuery(topic1,boundary,true));
            BOOST_REQUIRE(!rReadyTrue);
            BOOST_REQUIRE_EQUAL(rReadyTrue->size(),1u);
            BOOST_CHECK(rReadyTrue->at(0).unit<dc1::type>()->fieldValue(object::_id)==oid1);

            auto rClaimTrue=client->find(modelDc1(),makeClaimQuery(topic1,boundary,true));
            BOOST_REQUIRE(!rClaimTrue);
            BOOST_REQUIRE_EQUAL(rClaimTrue->size(),1u);
            BOOST_CHECK(rClaimTrue->at(0).unit<dc1::type>()->fieldValue(object::_id)==oid1);

            auto rReadyFalse=client->find(modelDc1(),makeReadyQuery(topic1,boundary,false));
            BOOST_REQUIRE(!rReadyFalse);
            BOOST_CHECK(rReadyFalse->empty());

            auto rClaimFalse=client->find(modelDc1(),makeClaimQuery(topic1,boundary,false));
            BOOST_REQUIRE(!rClaimFalse);
            BOOST_CHECK(rClaimFalse->empty());
        }

        // 3. delete it and create a replacement with the SAME oid but
        //    flag=false, within a single transaction - exactly the
        //    deleteObject()+create() pattern used by UploadQueue::replaceBatch()
        auto o2=makeDc1Object(dt,MarkerNew,false);
        o2.setFieldValue(object::_id,oid1);
        auto oid2=o2.fieldValue(object::_id);
        BOOST_CHECK(oid2==oid1);

        auto tx1=[&](Transaction* tx) -> Error
        {
            auto delEc=client->deleteObject(topic1,modelDc1(),oid1,tx);
            HATN_CHECK_EC(delEc)

            auto createEc=client->create(topic1,modelDc1(),&o2,tx);
            HATN_CHECK_EC(createEc)

            return Error{OK};
        };
        ec=client->transaction(tx1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // 4. read-by-id must see the replacement (flag=false, MarkerNew) -
        //    this direction is known to work; it is not what is under test
        auto rRead=client->read(topic1,modelDc1(),oid1);
        BOOST_REQUIRE(!rRead);
        BOOST_CHECK_EQUAL(rRead.value()->fieldValue(dc1::marker),MarkerNew);
        BOOST_CHECK_EQUAL(rRead.value()->fieldValue(dc1::flag),false);

        // 5. the old bucket (flag==true) must now be empty in BOTH indexes:
        //    the stale index entries left by the deleted object must have
        //    been removed
        {
            auto rReadyTrue=client->find(modelDc1(),makeReadyQuery(topic1,boundary,true));
            BOOST_REQUIRE(!rReadyTrue);
            BOOST_CHECK(rReadyTrue->empty());

            auto rClaimTrue=client->find(modelDc1(),makeClaimQuery(topic1,boundary,true));
            BOOST_REQUIRE(!rClaimTrue);
            BOOST_CHECK(rClaimTrue->empty());
        }

        // 6. the new bucket (flag==false) must resolve to exactly the
        //    replacement object in BOTH indexes - this is the exact query
        //    shape (UploadQueue::processTopic()'s claim query: sending==
        //    false && next_attempt_at<=now) that returned zero rows against
        //    the real files2 queue after a claimed batch went through
        //    cloneBatchWithItemChanges()+replaceBatch()
        {
            auto rReadyFalse=client->find(modelDc1(),makeReadyQuery(topic1,boundary,false));
            BOOST_REQUIRE(!rReadyFalse);
            BOOST_REQUIRE_EQUAL(rReadyFalse->size(),1u);
            const auto* foundReady=rReadyFalse->at(0).unit<dc1::type>();
            BOOST_CHECK(foundReady->fieldValue(object::_id)==oid1);
            BOOST_CHECK_EQUAL(foundReady->fieldValue(dc1::marker),MarkerNew);

            auto rClaimFalse=client->find(modelDc1(),makeClaimQuery(topic1,boundary,false));
            BOOST_REQUIRE(!rClaimFalse);
            BOOST_REQUIRE_EQUAL(rClaimFalse->size(),1u);
            const auto* foundClaim=rClaimFalse->at(0).unit<dc1::type>();
            BOOST_CHECK(foundClaim->fieldValue(object::_id)==oid1);
            BOOST_CHECK_EQUAL(foundClaim->fieldValue(dc1::marker),MarkerNew);
        }

        // 7. every index untouched by the flip (same field values across
        //    the swap) must still resolve correctly too
        checkOnlyObject(client,topic1,dt,oid1,MarkerNew,oid1);
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

BOOST_AUTO_TEST_CASE(SameObjectIdFlagFlipInTx)
{
    HATN_CTX_SCOPE("SameObjectIdFlagFlipInTx")
    runDeleteCreateFlagFlip();
}

// Same scenario as SameObjectIdFlagFlipInTx, but driven through
// db::AsyncClient with two real threads instead of the synchronous Client
// interface directly - reproducing whitemclient::files2::UploadQueue's
// actual call topology:
//
//   - a "caller" thread mirrors UploadQueue::stageEncrypting()'s dedicated
//     crypto worker thread, which is the thread that actually calls
//     AsyncClient::transaction()/find() (NOT the app thread);
//   - a "db" thread mirrors the single app thread that every AsyncClient
//     call in whitemclient's test harness config actually runs its work
//     on. This is not an assumption: whitemclient::Files2TestApp's config
//     sets "app":{"thread_count":1}; App_p::makeDbMappedThreads() (app.cpp)
//     only switches its MappedThreadQWithTaskContext from
//     MappedThreadMode::Default to Mapped when db_config::thread_count>1,
//     and db_config::thread_count defaults to 1 and is never overridden by
//     that harness - so MappedThreadMode stays Default, meaning EVERY
//     topic resolves to the one app thread regardless of which thread
//     calls in.
//   - common::postAsyncTask() (threadwithqueue.h) posts the actual work to
//     the target thread, then routes the completion callback back to
//     whichever thread ORIGINALLY CALLED the async method - not to the
//     thread that ran the work. So AsyncClient::transaction()'s completion
//     callback (txCb below) fires back on the CALLER thread, exactly like
//     UploadQueue::stageEncrypting()'s txCb does - and it is from inside
//     that callback, running back on the caller thread, that
//     UploadQueue::nextItem()->processTopic() posts the very next claim
//     query. This test reproduces exactly that bounce: caller posts
//     transaction to db thread -> callback bounces back to caller -> caller
//     posts the claim query to db thread from inside that callback.
//
// None of the synchronous Client-based tests above (including
// SameObjectIdFlagFlipInTx, which reproduces every other structural aspect
// of upload_batch) exercise this cross-thread posting/callback-routing
// mechanism at all - client->transaction()/find() run inline, with no
// posting and no thread hop, so they cannot catch a bug that lives in
// db::AsyncClient/common::postAsyncTask rather than in the underlying
// rocksdb storage/index engine.
BOOST_AUTO_TEST_CASE(AsyncClientSameOidFlagFlipInTx)
{
    HATN_CTX_SCOPE("AsyncClientSameOidFlagFlipInTx")

    init();
    auto s1=initSchema(modelDc1());

    // TestDeleteCreate attaches DbTestFixture (which owns createThreads()/
    // thread()/exec()/quit()) as a *decorator* (see the suite declaration:
    // *boost::unit_test::fixture<...>()), not via BOOST_FIXTURE_TEST_CASE -
    // so this test case is a plain, non-member function and has no `this`
    // to reach those methods through. A local MultiThreadFixture instance
    // provides the same thread/exec machinery directly.
    HATN_TEST_NAMESPACE::MultiThreadFixture threading;

    auto handler=[&threading,&s1](std::shared_ptr<DbPlugin>, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};
        auto dt=common::DateTime::currentUtc();
        auto boundary=dt;
        boundary.addSeconds(5);

        // 1. create the original object synchronously - setup only, not
        //    what is under test
        auto o1=makeDc1Object(dt,MarkerOld,true);
        auto ec=client->create(topic1,modelDc1(),&o1);
        BOOST_REQUIRE(!ec);
        auto oid1=o1.fieldValue(object::_id);

        auto o2=makeDc1Object(dt,MarkerNew,false);
        o2.setFieldValue(object::_id,oid1);

        // 2. caller thread + db thread, AsyncClient targeting the db thread
        //    with MappedThreadMode::Default - see the comment above the
        //    test case for why this specific mode/thread-count combination
        //    matches whitemclient's actual test harness.
        threading.createThreads(2);
        threading.thread(0)->start();
        threading.thread(1)->start();
        auto callerThread=dynamic_cast<common::TaskWithContextThread*>(threading.thread(0).get());
        auto dbThread=dynamic_cast<common::TaskWithContextThread*>(threading.thread(1).get());
        BOOST_REQUIRE(callerThread!=nullptr);
        BOOST_REQUIRE(dbThread!=nullptr);

        auto asyncClient=std::make_shared<AsyncClient>(client,common::MappedThreadMode::Default,dbThread);
        auto quitFn=[&threading](){ threading.quit(); };

        bool sawCallback=false;
        bool findFailed=false;
        size_t foundCount=0;

        // 3. from the caller thread: post a transaction (delete+create,
        //    same oid, flag true->false) - UploadQueue::replaceBatch()'s
        //    exact pattern - then, from inside ITS completion callback
        //    (routed back to the caller thread by postAsyncTask), post the
        //    claim query (same shape as processTopic()'s uploadReadyIdx
        //    query: where(flag,eq,false).and_(f4,lte,boundary)) - exactly
        //    what UploadQueue::nextItem()->processTopic() does immediately
        //    after replaceBatch() completes.
        auto runOnCaller=[&]()
        {
            HATN_CHECK_TS(callerThread->id()==common::Thread::currentThreadID())

            auto txHandler=[client,topic1,oid1,o2,dbThread](Transaction* tx) -> Error
            {
                HATN_CHECK_TS(dbThread->id()==common::Thread::currentThreadID())
                auto delEc=client->deleteObject(topic1,modelDc1(),oid1,tx);
                HATN_CHECK_EC(delEc)
                auto createEc=client->create(topic1,modelDc1(),&o2,tx);
                HATN_CHECK_EC(createEc)
                return Error{OK};
            };

            auto txCb=[&,asyncClient,topic1,boundary,quitFn](auto, const Error& ec)
            {
                HATN_CHECK_TS(callerThread->id()==common::Thread::currentThreadID())
                if (ec)
                {
                    HATN_TEST_MESSAGE_TS(ec.message())
                }
                HATN_REQUIRE_TS(!ec)

                auto findCb=[&,quitFn](auto, auto r)
                {
                    HATN_CHECK_TS(callerThread->id()==common::Thread::currentThreadID())
                    sawCallback=true;
                    if (r)
                    {
                        HATN_TEST_MESSAGE_TS(r.error().message())
                        findFailed=true;
                    }
                    else
                    {
                        foundCount=r.value().size();
                    }
                    quitFn();
                };

                // AsyncClient::find()'s QueryT parameter must be a deferred
                // query BUILDER (it calls query() internally, on whatever
                // thread the task actually runs on) - not a Query object
                // directly, which is what makeReadyQuery() returns and what
                // the synchronous client->find() calls above use. Wrap it
                // exactly like UploadQueue::processTopic()'s claim query does.
                auto q=db::wrapQueryBuilder(
                    [topic1,boundary]()
                    {
                        return makeReadyQuery(topic1,boundary,false);
                    },
                    topic1.topic()
                );
                asyncClient->find(common::makeShared<common::TaskContext>(),findCb,modelDc1(),q,topic1);
            };

            asyncClient->transaction(common::makeShared<common::TaskContext>(),txCb,std::move(txHandler),topic1);
        };

        callerThread->execAsync(runOnCaller);

        threading.exec(5);

        threading.thread(0)->stop();
        threading.thread(1)->stop();
        // eachPlugin() below calls this handler more than once (unencrypted,
        // then encrypted rocksdb) reusing the same `threading` instance -
        // destroyThreads() (not ~MultiThreadFixture(), which also calls the
        // static Thread::releaseMainThread()/WeakPool::free() and would
        // disturb the suite-wide fixture) resets it so the next invocation's
        // createThreads(2) does not hit "You have already created test
        // threads".
        threading.destroyThreads();

        // 4. this is the exact query shape (UploadQueue::processTopic()'s
        //    claim query) that returned zero rows against the real files2
        //    queue after a claimed batch went through
        //    cloneBatchWithItemChanges()+replaceBatch() - if it comes back
        //    empty here too, that confirms the bug lives in the
        //    async/threaded dispatch path (db::AsyncClient/postAsyncTask),
        //    not in the underlying rocksdb storage/index engine, which
        //    every synchronous test case above already proved correct.
        BOOST_REQUIRE(sawCallback);
        BOOST_CHECK(!findFailed);
        BOOST_CHECK_EQUAL(foundCount,1u);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
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
