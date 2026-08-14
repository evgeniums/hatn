/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/****************************************************************************/
/** @file db/test/testfindrangein.cpp
  *
  *  Regression coverage for a composite index query combining a range
  *  condition on the LEADING field with an `in` condition on the TRAILING
  *  field, scanned in Order::Desc. Every existing find test either varies
  *  a single field's operator while pinning the other field to `eq`
  *  (findcompoundqueries.ipp/findcompoundqueries2.ipp), or exercises
  *  Order::Desc only on a single-field index (findhandlers.ipp), so this
  *  exact combination -- leading-field range + trailing-field `in`,
  *  Order::Desc -- was previously untested.
  *
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

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

HDU_UNIT_WITH(ri1,(HDU_BASE(object)),
    HDU_FIELD(seq,TYPE_UINT32,1)
    HDU_FIELD(cat,TYPE_STRING,2)
)

HATN_DB_INDEX(ri1_seq_cat_idx,ri1::seq,ri1::cat)

HATN_DB_MODEL(modelRi1,ri1,
              ri1_seq_cat_idx()
             )

//! Same shape as ri1 above, but with the leading/trailing field types
//! swapped in for the EXACT types the field being investigated actually
//! uses in production (TYPE_OBJECT_ID leading, a 16-byte fixed string
//! trailing) -- ri1's plain TYPE_UINT32/TYPE_STRING passed, so this
//! isolates whether the bug is specific to these field types' own key
//! encoding rather than the query shape itself.
HDU_UNIT_WITH(ri2,(HDU_BASE(object)),
    HDU_FIELD(sortId,TYPE_OBJECT_ID,1)
    HDU_FIELD(cat,HDU_TYPE_FIXED_STRING(16),2)
)

HATN_DB_INDEX(ri2_sortid_cat_idx,ri2::sortId,ri2::cat)

HATN_DB_MODEL(modelRi2,ri2,
              ri2_sortid_cat_idx()
             )

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void registerModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(modelRi1());
    rdb::RocksdbModels::instance().registerModel(modelRi2());
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
    auto schema1=makeSchema("schema_findrangein",std::forward<Models>(models)...);

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

auto makeRi1Object(uint32_t seq, std::string cat)
{
    auto o=makeInitObject<ri1::type>();
    o.setFieldValue(ri1::seq,seq);
    o.setFieldValue(ri1::cat,cat);
    return o;
}

//! seq<=boundary, no cat filter -- sanity baseline: a plain single-field
//! Desc range scan is already known to work (ordinary, unfiltered chat
//! scrolling relies on exactly this), so this must pass regardless of
//! whether the bug under test reproduces.
auto makeUnfilteredDescQuery(Topic topic, uint32_t boundary)
{
    return makeQuery(ri1_seq_cat_idx(),
                      query::where(ri1::seq,query::lte,boundary,query::Desc),
                      topic);
}

//! seq<=boundary AND cat==value, Desc -- a SINGLE eq condition (not `in`)
//! on the trailing field, to isolate whether the bug is specific to the
//! `in`-vector decomposition machinery or affects any second condition
//! combined with a Desc leading range.
auto makeEqFilteredDescQuery(Topic topic, uint32_t boundary, const std::string& value)
{
    return makeQuery(ri1_seq_cat_idx(),
                      query::where(ri1::seq,query::lte,boundary,query::Desc)
                            .and_(ri1::cat,query::eq,value),
                      topic);
}

//! seq<=boundary AND cat in {values}, Desc -- the suspected bug: a
//! trailing-field `in` condition combined with a leading-field Desc range.
//! `values` must outlive the query -- query conditions hold a reference to
//! vector values, see query.h's own note on this.
auto makeInFilteredDescQuery(Topic topic, uint32_t boundary, const std::vector<std::string>& values)
{
    return makeQuery(ri1_seq_cat_idx(),
                      query::where(ri1::seq,query::lte,boundary,query::Desc)
                            .and_(ri1::cat,query::in,values),
                      topic);
}

//! seq>boundary AND cat in {values}, Asc -- the same `in` condition as
//! makeInFilteredDescQuery(), but Asc instead of Desc, to confirm the `in`
//! machinery itself is sound and the bug (if it reproduces) is specific to
//! the Desc direction, not to `in` filtering in general.
auto makeInFilteredAscQuery(Topic topic, uint32_t boundary, const std::vector<std::string>& values)
{
    return makeQuery(ri1_seq_cat_idx(),
                      query::where(ri1::seq,query::gt,boundary,query::Asc)
                            .and_(ri1::cat,query::in,values),
                      topic);
}

std::vector<uint32_t> seqsOf(const common::pmr::vector<DbObject>& result)
{
    std::vector<uint32_t> seqs;
    for (const auto& item : result)
    {
        seqs.push_back(item.unit<ri1::type>()->fieldValue(ri1::seq));
    }
    return seqs;
}

//! ri2 (TYPE_OBJECT_ID/HDU_TYPE_FIXED_STRING(16)) counterparts of the ri1
//! helpers above -- sortId is a copy of the object's own generated _id
//! (same idiom production uses to derive its own leading sort field), so
//! sequentially created objects sort in creation order, same as ri1's
//! explicit incrementing seq.
auto makeRi2Object(const std::string& cat)
{
    auto o=makeInitObject<ri2::type>();
    o.setFieldValue(ri2::sortId,o.fieldValue(object::_id));
    o.setFieldValue(ri2::cat,cat);
    return o;
}

auto makeRi2UnfilteredDescQuery(Topic topic, const ObjectId& boundary)
{
    return makeQuery(ri2_sortid_cat_idx(),
                      query::where(ri2::sortId,query::lte,boundary,query::Desc),
                      topic);
}

auto makeRi2EqFilteredDescQuery(Topic topic, const ObjectId& boundary, const std::string& value)
{
    return makeQuery(ri2_sortid_cat_idx(),
                      query::where(ri2::sortId,query::lte,boundary,query::Desc)
                            .and_(ri2::cat,query::eq,value),
                      topic);
}

auto makeRi2InFilteredDescQuery(Topic topic, const ObjectId& boundary, const std::vector<std::string>& values)
{
    return makeQuery(ri2_sortid_cat_idx(),
                      query::where(ri2::sortId,query::lte,boundary,query::Desc)
                            .and_(ri2::cat,query::in,values),
                      topic);
}

auto makeRi2InFilteredAscQuery(Topic topic, const ObjectId& boundary, const std::vector<std::string>& values)
{
    return makeQuery(ri2_sortid_cat_idx(),
                      query::where(ri2::sortId,query::gt,boundary,query::Asc)
                            .and_(ri2::cat,query::in,values),
                      topic);
}

std::vector<ObjectId> sortIdsOf(const common::pmr::vector<DbObject>& result)
{
    std::vector<ObjectId> ids;
    for (const auto& item : result)
    {
        ids.push_back(item.unit<ri2::type>()->fieldValue(ri2::sortId));
    }
    return ids;
}

void runFindRangeIn()
{
    init();
    auto s1=initSchema(modelRi1());

    auto handler=[&s1](std::shared_ptr<DbPlugin>, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};

        // seq 0..9, cat cycling a/b/c by seq%3:
        //  seq  0 1 2 3 4 5 6 7 8 9
        //  cat  a b c a b c a b c a
        static const std::vector<std::string> cats{"a","b","c"};
        for (uint32_t seq=0;seq<10;seq++)
        {
            auto o=makeRi1Object(seq,cats[seq%3]);
            auto ec=client->create(topic1,modelRi1(),&o);
            BOOST_REQUIRE(!ec);
        }

        const uint32_t boundary=6;
        std::vector<std::string> filterAB{"a","b"};

        // Sanity: unfiltered Desc range already known-good (matches
        // ordinary, already-working Desc scrolling elsewhere in the
        // codebase) -- seq<=6, all categories: 6,5,4,3,2,1,0.
        {
            auto r=client->find(modelRi1(),makeUnfilteredDescQuery(topic1,boundary));
            BOOST_REQUIRE(!r);
            std::vector<uint32_t> expected{6,5,4,3,2,1,0};
            BOOST_CHECK(seqsOf(*r)==expected);
        }

        // seq<=6 AND cat=="a", Desc -- single eq condition on the trailing
        // field: 6,3,0.
        {
            auto r=client->find(modelRi1(),makeEqFilteredDescQuery(topic1,boundary,"a"));
            BOOST_REQUIRE(!r);
            std::vector<uint32_t> expected{6,3,0};
            BOOST_CHECK(seqsOf(*r)==expected);
        }

        // seq<=6 AND cat in {"a"}, Desc -- single-element `in`, expected to
        // match the eq-filtered result above exactly if the `in`-vector
        // decomposition itself is sound; a mismatch here (versus a
        // multi-element `in` mismatch below) would narrow the bug to the
        // decomposition machinery even for the trivial one-element case.
        {
            std::vector<std::string> filterA{"a"};
            auto r=client->find(modelRi1(),makeInFilteredDescQuery(topic1,boundary,filterA));
            BOOST_REQUIRE(!r);
            std::vector<uint32_t> expected{6,3,0};
            BOOST_CHECK(seqsOf(*r)==expected);
        }

        // seq>6 AND cat in {"a","b"}, Asc -- multi-element `in` combined
        // with Asc (the direction production code already exercises
        // successfully): 7,9.
        {
            auto r=client->find(modelRi1(),makeInFilteredAscQuery(topic1,boundary,filterAB));
            BOOST_REQUIRE(!r);
            std::vector<uint32_t> expected{7,9};
            BOOST_CHECK(seqsOf(*r)==expected);
        }

        // seq<=6 AND cat in {"a","b"}, Desc -- the reported bug: expected
        // 6,4,3,1,0 (including the boundary row seq==6, which `lte` must
        // include). A production trace of the equivalent query (composite
        // index leading range + trailing `in`, Desc) returned ZERO rows,
        // not even the boundary row, while the Asc case above worked.
        {
            auto r=client->find(modelRi1(),makeInFilteredDescQuery(topic1,boundary,filterAB));
            BOOST_REQUIRE(!r);
            std::vector<uint32_t> expected{6,4,3,1,0};
            BOOST_CHECK(seqsOf(*r)==expected);
        }
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

//! Same 5 queries as runFindRangeIn(), against ri2 (TYPE_OBJECT_ID leading /
//! HDU_TYPE_FIXED_STRING(16) trailing) instead of ri1 (TYPE_UINT32/
//! TYPE_STRING) -- isolates whether the bug is specific to these
//! production-matching field types' own key encoding.
void runFindRangeInObjectIdFixedString()
{
    init();
    auto s1=initSchema(modelRi2());

    auto handler=[&s1](std::shared_ptr<DbPlugin>, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        Topic topic1{"topic1"};

        // 10 objects, created sequentially so their own generated _id (and
        // hence sortId, a copy of it) sorts in creation order, cat cycling
        // a/b/c by creation index:
        //  index  0 1 2 3 4 5 6 7 8 9
        //  cat    a b c a b c a b c a
        static const std::vector<std::string> cats{"a","b","c"};
        std::vector<ObjectId> ids;
        ids.reserve(10);
        for (uint32_t i=0;i<10;i++)
        {
            auto o=makeRi2Object(cats[i%3]);
            auto ec=client->create(topic1,modelRi2(),&o);
            BOOST_REQUIRE(!ec);
            ids.push_back(o.fieldValue(ri2::sortId));
        }

        const auto& boundary=ids[6];
        std::vector<std::string> filterAB{"a","b"};

        // Sanity: unfiltered Desc range.
        {
            auto r=client->find(modelRi2(),makeRi2UnfilteredDescQuery(topic1,boundary));
            BOOST_REQUIRE(!r);
            std::vector<ObjectId> expected{ids[6],ids[5],ids[4],ids[3],ids[2],ids[1],ids[0]};
            BOOST_CHECK(sortIdsOf(*r)==expected);
        }

        // sortId<=ids[6] AND cat=="a", Desc -- single eq condition.
        {
            auto r=client->find(modelRi2(),makeRi2EqFilteredDescQuery(topic1,boundary,"a"));
            BOOST_REQUIRE(!r);
            std::vector<ObjectId> expected{ids[6],ids[3],ids[0]};
            BOOST_CHECK(sortIdsOf(*r)==expected);
        }

        // sortId<=ids[6] AND cat in {"a"}, Desc -- single-element `in`.
        {
            std::vector<std::string> filterA{"a"};
            auto r=client->find(modelRi2(),makeRi2InFilteredDescQuery(topic1,boundary,filterA));
            BOOST_REQUIRE(!r);
            std::vector<ObjectId> expected{ids[6],ids[3],ids[0]};
            BOOST_CHECK(sortIdsOf(*r)==expected);
        }

        // sortId>ids[6] AND cat in {"a","b"}, Asc.
        {
            auto r=client->find(modelRi2(),makeRi2InFilteredAscQuery(topic1,boundary,filterAB));
            BOOST_REQUIRE(!r);
            std::vector<ObjectId> expected{ids[7],ids[9]};
            BOOST_CHECK(sortIdsOf(*r)==expected);
        }

        // sortId<=ids[6] AND cat in {"a","b"}, Desc -- the reported bug,
        // reproduced (if it reproduces here at all) with the EXACT field
        // types production uses.
        {
            auto r=client->find(modelRi2(),makeRi2InFilteredDescQuery(topic1,boundary,filterAB));
            BOOST_REQUIRE(!r);
            std::vector<ObjectId> expected{ids[6],ids[4],ids[3],ids[1],ids[0]};
            BOOST_CHECK(sortIdsOf(*r)==expected);
        }
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE(TestFindRangeIn, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(CompositeIndexLeadingRangeTrailingInDesc)
{
    HATN_CTX_SCOPE("CompositeIndexLeadingRangeTrailingInDesc")
    runFindRangeIn();
}

BOOST_AUTO_TEST_CASE(CompositeIndexObjectIdFixedStringDesc)
{
    HATN_CTX_SCOPE("CompositeIndexObjectIdFixedStringDesc")
    runFindRangeInObjectIdFixedString();
}

BOOST_AUTO_TEST_SUITE_END()
