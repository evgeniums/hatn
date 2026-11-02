/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testuniquettl.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include <hatn/base/configtreeloader.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/db/schema.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/db/object.h>
#include <hatn/db/model.h>
#include <hatn/db/expire.h>

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

// The shape of ObjectsCache's cache_object: a unique index over a key field and a TTL on expire_at.
HDU_UNIT_WITH(uttl,(HDU_BASE(object),HDU_BASE(with_expire)),
    HDU_FIELD(key,TYPE_STRING,1)
)

HATN_DB_UNIQUE_INDEX(uttlKeyIdx,uttl::key)
HATN_DB_MODEL(modelUttl,uttl,uttlKeyIdx(),expireIdx())

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void init()
{
    ModelRegistry::free();
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::free();
    rdb::RocksdbModels::free();
    rdb::RocksdbModels::instance().registerModel(modelUttl());
#endif
}

auto initSchema()
{
    auto schema1=makeSchema("schema1",modelUttl());
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::instance().registerSchema(schema1);
#endif
    return schema1;
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestUniqueTtl)

/**
 * A row whose TTL is shortened by an update must not keep its unique key alive for the old TTL:
 * once the row has expired the key is free for a new row. (The unique entry used to be rewritten
 * under a key nothing read, so it kept the old, long TTL mark and blocked the value with
 * DbError::DUPLICATE_UNIQUE_KEY long after its row was gone.)
 */
BOOST_FIXTURE_TEST_CASE(UniqueKeyFreedAfterTtlShortened, HATN_TEST_NAMESPACE::DbTestFixture)
{
    init();

    auto s1=initSchema();

    auto handler=[&s1,this](std::shared_ptr<DbPlugin>, std::shared_ptr<Client> client)
    {
        auto ec=client->setSchema(s1);
        BOOST_REQUIRE(!ec);

        Topic topic1{"topic1"};
        const std::string key{"x"};

        // create a row that lives for an hour
        auto o1=makeInitObject<uttl::type>();
        o1.setFieldValue(uttl::key,key);
        auto longTtl=common::DateTime::currentUtc();
        longTtl.addSeconds(3600);
        o1.setFieldValue(with_expire::expire_at,longTtl);
        ec=client->create(topic1,modelUttl(),&o1);
        BOOST_REQUIRE(!ec);

        // while it is alive, its key is taken
        auto o2=makeInitObject<uttl::type>();
        o2.setFieldValue(uttl::key,key);
        ec=client->create(topic1,modelUttl(),&o2);
        BOOST_REQUIRE(ec);
        BOOST_CHECK(ec.is(DbError::DUPLICATE_UNIQUE_KEY));

        // shorten the TTL
        auto shortTtl=common::DateTime::currentUtc();
        shortTtl.addSeconds(1);
        auto updateReq=update::request(update::field(with_expire::expire_at,update::set,shortTtl));
        ec=client->update(topic1,modelUttl(),o1.fieldValue(object::_id),updateReq);
        BOOST_REQUIRE(!ec);

        BOOST_TEST_MESSAGE("Waiting for 4 seconds...");
        exec(4);

        // the expired row is gone and its key is free again
        auto o3=makeInitObject<uttl::type>();
        o3.setFieldValue(uttl::key,key);
        ec=client->create(topic1,modelUttl(),&o3);
        if (ec)
        {
            HATN_CTX_ERROR(ec,"failed to reuse unique key of an expired row");
        }
        BOOST_REQUIRE(!ec);

        auto q1=makeQuery(uttlKeyIdx(),query::where(uttl::key,query::eq,key),topic1);
        auto r1=client->find(modelUttl(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1->size(),1);
        BOOST_CHECK_EQUAL(r1->at(0).unit<uttl::type>()->fieldValue(object::_id).toString(),
                          o3.fieldValue(object::_id).toString());
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
