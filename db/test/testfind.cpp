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

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/db/schema.h>
#include <hatn/dataunit/ipp/objectid.ipp>

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

// struct U1
// {
//     static void registerU1();
// };

HDU_UNIT_WITH(u1,(HDU_BASE(object)),
              HDU_FIELD(f_bool,TYPE_BOOL,1)
              HDU_FIELD(f_int8,TYPE_INT8,2)
              HDU_FIELD(f_int16,TYPE_INT16,3)
              HDU_FIELD(f_int32,TYPE_INT32,4)
              HDU_FIELD(f_int64,TYPE_INT64,5)
              HDU_FIELD(f_uint8,TYPE_UINT8,6)
              HDU_FIELD(f_uint16,TYPE_UINT16,7)
              HDU_FIELD(f_uint32,TYPE_UINT32,8)
              HDU_FIELD(f_uint64,TYPE_UINT64,9)
              HDU_FIELD(f_float,TYPE_FLOAT,10)
              HDU_FIELD(f_double,TYPE_DOUBLE,11)
              HDU_FIELD(f_string,TYPE_STRING,12)
              HDU_FIELD(f_bytes,TYPE_BYTES,13)
              HDU_FIELD(f_dt,TYPE_DATETIME,14)
              HDU_FIELD(f_date,TYPE_DATE,15)
              HDU_FIELD(f_time,TYPE_TIME,16)
              HDU_FIELD(f_oid,TYPE_OBJECT_ID,17)
              HDU_FIELD(f_fix_str,HDU_TYPE_FIXED_STRING(8),18)
              HDU_ENUM(MyEnum,One=1,Two=2)
              HDU_DEFAULT_FIELD(f_enum,HDU_TYPE_ENUM(MyEnum),19,MyEnum::Two)
)

HDU_UNIT_WITH(u2,(HDU_BASE(object)),
              HDU_FIELD(ff_su,u1::TYPE,1)
              HDU_FIELD(ff_int32,TYPE_INT32,4)
)

namespace {

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

auto initU1()
{
    auto plainIndexes=makePlainIndexes(DefaultIndexConfig,
                                         u1::f_bool,
                                         u1::f_int8,
                                         u1::f_int16,
                                         u1::f_int32,
                                         u1::f_uint8,
                                         u1::f_uint16,
                                         u1::f_uint32,
                                         u1::f_uint64,
                                         u1::f_string,
                                         u1::f_fix_str,
                                         u1::f_enum,
                                         u1::f_dt,
                                         u1::f_date,
                                         u1::f_time,
                                         u1::f_oid
                                         );
    auto m1=makeModelWithIdx<u1::TYPE>(DefaultModelConfig,plainIndexes);
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(m1);
#endif
    return m1;
}

auto initU2()
{
    auto plainIndexes=makePlainIndexes(DefaultIndexConfig,
                                         u2::ff_int32
                                         );
    auto m=makeModelWithIdx<u2::TYPE>(DefaultModelConfig,plainIndexes);
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(m);
#endif
    return m;
}

template <typename ...Models>
auto initSchema(Models&& ...models)
{
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));

    ModelRegistry::free();
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::free();
    rdb::RocksdbModels::free();
#endif

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
    auto m1=initU1();
    auto m2=initU1();
    auto s1=initSchema(m1,m2);

    auto handler=[&s1,&m1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Namespace ns{"topic1"};

        auto o1=makeInitObject<u1::type>();
        BOOST_TEST_MESSAGE(fmt::format("Original o1: {}",o1.toString(true)));

        // create object
        auto ec=client->create(ns,m1,&o1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // find object by f_bool
        // auto r1=client->findOne(ns,m1,query::where(u1::f_bool,query::Operator::eq,false));
        // if (r1)
        // {
        //     BOOST_TEST_MESSAGE(r1.error().message());
        // }
        // BOOST_REQUIRE(!r1);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
