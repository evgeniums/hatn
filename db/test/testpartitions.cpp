/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testpartitions.cpp
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

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {

HDU_UNIT(base_fields,
    HDU_FIELD(df2,TYPE_UINT32,2)
    HDU_FIELD(df3,TYPE_STRING,3)
)

HDU_UNIT_WITH(explicit_p,(HDU_BASE(object),HDU_BASE(base_fields)),
    HDU_FIELD(pf1,TYPE_DATE,1)
)

HDU_UNIT_WITH(implicit_p,(HDU_BASE(object),HDU_BASE(base_fields))
)

HATN_DB_INDEX(df2Idx,base_fields::df2)
HATN_DB_INDEX(df3Idx,base_fields::df3)

HATN_DB_PARTITION_INDEX(pf1Idx1,explicit_p::pf1)
HATN_DB_MODEL_WITH_CFG(modelExplicit1,explicit_p,ModelConfig("explicit_p1"),pf1Idx1(),df2Idx(),df3Idx())

HATN_DB_PARTITION_INDEX(pf1Idx2,explicit_p::pf1,base_fields::df2)
HATN_DB_MODEL_WITH_CFG(modelExplicit2,explicit_p,ModelConfig("explicit_p2"),pf1Idx2(),df3Idx())

HATN_DB_IMPLICIT_PARTITION_MODEL_WITH_CFG(modelImplicit1,implicit_p,ModelConfig("implicit_p1"),df2Idx(),df3Idx())

HATN_DB_PARTITION_INDEX(oidIdx2,object::_id,base_fields::df2)
HATN_DB_EXPLICIT_ID_IDX_MODEL_WITH_CFG(modelImplicit2,implicit_p,ModelConfig("implicit_p2"),oidIdx2(),df3Idx())

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void registerModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(modelExplicit1());
    rdb::RocksdbModels::instance().registerModel(modelExplicit2());
    rdb::RocksdbModels::instance().registerModel(modelImplicit1());
    rdb::RocksdbModels::instance().registerModel(modelImplicit2());
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

BOOST_AUTO_TEST_SUITE(TestPartitions, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(InitSchema)
{
    init();

    auto s1=initSchema(modelExplicit1(),modelExplicit2(),modelImplicit1(),modelImplicit2());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(PartitionsOperations)
{
    auto run=[](const auto& model)
    {
        init();

        auto s1=initSchema(model);
        std::vector<ModelInfo> modelInfos{*(model->info)};

        auto handler=[&s1,&modelInfos](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
        {
            setSchemaToClient(client,s1);

            BOOST_TEST_MESSAGE("Add partitions");
            auto ec=client->addDatePartitions(modelInfos,common::Date::currentUtc().copyAddDays(365));
            BOOST_REQUIRE(!ec);

            BOOST_TEST_MESSAGE("List partitions");
            auto ranges=client->listDatePartitions();
            BOOST_REQUIRE(!ranges);
            BOOST_CHECK_EQUAL(ranges->size(),12+1);

            BOOST_TEST_MESSAGE("Delete partitions");
            ec=client->deleteDatePartitions(modelInfos,common::Date::currentUtc().copyAddDays(183),common::Date::currentUtc());
            BOOST_REQUIRE(!ec);

            BOOST_TEST_MESSAGE("List partitions after delete");
            ranges=client->listDatePartitions();
            BOOST_REQUIRE(!ranges);
            BOOST_CHECK_EQUAL(ranges->size(),6);

            BOOST_TEST_MESSAGE("Try to delete already deleted partitions");
            ec=client->deleteDatePartitions(modelInfos,common::Date::currentUtc().copyAddDays(183),common::Date::currentUtc());
            BOOST_REQUIRE(!ec);

            BOOST_TEST_MESSAGE("List partitions after retry delete");
            ranges=client->listDatePartitions();
            BOOST_REQUIRE(!ranges);
            BOOST_CHECK_EQUAL(ranges->size(),6);
        };
        PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
    };

    BOOST_TEST_CONTEXT("modelImplicit1()"){run(modelImplicit1());}
    BOOST_TEST_CONTEXT("modelImplicit2()"){run(modelImplicit2());}
    BOOST_TEST_CONTEXT("modelExplicit1()"){run(modelExplicit1());}
    BOOST_TEST_CONTEXT("modelExplicit2()"){run(modelExplicit2());}
}

BOOST_AUTO_TEST_SUITE_END()
