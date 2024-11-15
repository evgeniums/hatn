/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testfind.cpp
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

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#include "models1.h"
#if 0
#include "models9.h"
#include "models10.h"
#endif

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void init()
{
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));

    ModelRegistry::free();
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::free();
    rdb::RocksdbModels::free();
#endif

    registerModels1();
#if 0
    registerModels2();
    registerModels3();
    registerModels4();
    registerModels5();
    registerModels6();
    registerModels7();
    registerModels8();
    registerModels9();
    registerModels10();
#endif
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

BOOST_AUTO_TEST_SUITE(TestFind, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(OneLevel)
{
    init();

    auto s1=initSchema(m1_bool()

                //          ,
                // m1_int8(),
                // m1_int16(),
                // m1_int32(),
                // m1_int64(),
                // m1_uint8(),
                // m1_uint16(),
                // m1_uint32(),
                // m1_uint64(),
                // m1_str(),
                // m1_fix_str(),
                // m1_dt(),
                // m1_date(),
                // m1_time(),
                // m1_oid()
                         // ,
                // m2(),
                // m3()
            );

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        auto o1=makeInitObject<u1_bool::type>();
        BOOST_TEST_MESSAGE(fmt::format("Original o1: {}",o1.toString(true)));
        o1.setFieldValue(u1_bool::f1,false);
        BOOST_TEST_MESSAGE(fmt::format("o1 after f1 set: {}",o1.toString(true)));

        // create object
        auto ec=client->create(topic1,m1_bool(),&o1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // find object by f_bool
        auto q=makeQuery(u1_bool_f1_idx(),query::where(u1_bool::f1,query::Operator::eq,false));
        std::cerr<<"operand type="<<static_cast<int>(q.fields().at(0).value.typeId())<<std::endl;
        std::cerr<<"operand value="<<static_cast<bool>(q.fields().at(0).value.as<query::BoolValue>())<<std::endl;
        common::FmtAllocatedBufferChar buf;
        HATN_ROCKSDB_NAMESPACE::fieldValueToBuf(buf,q.fields().at(0));
        std::cerr<<"operand string="<<common::fmtBufToString(buf)<<std::endl;

        auto r1=client->findOne(m1_bool(),makeQuery(u1_bool_f1_idx(),query::where(u1_bool::f1,query::Operator::eq,false),topic1));
        if (r1)
        {
            BOOST_TEST_MESSAGE(r1.error().message());
        }
        BOOST_REQUIRE(!r1);
        BOOST_CHECK(!r1.value().isNull());
        auto r2=client->findOne(m1_bool(),makeQuery(u1_bool_f1_idx(),query::where(u1_bool::f1,query::Operator::eq,true),topic1));
        BOOST_REQUIRE(!r2);
        BOOST_CHECK(r2.value().isNull());
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()


/** @todo Test find:
 *
 *  1. Test scalar types
 *  2. Test strings
 *  3. Test Oid/DateTime/date/Time/DateRange
 *  4. Test intervals
 *  5. Test vectors
 *  6. Test ordering
 *  7. Test timepoint filtering
 *  8. Test limits
 *
 */
