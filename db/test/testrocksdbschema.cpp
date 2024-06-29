/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testrocksdbschema.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>
#include <hatn/base/configtreeloader.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/db/schema.h>
#include <hatn/db/ipp/objectid.ipp>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbschema.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace rdb=HATN_ROCKSDB_NAMESPACE;

namespace {

HDU_UNIT(n1,
    HDU_FIELD(f1,TYPE_DATETIME,1)
)

HDU_UNIT_WITH(nu1,(HDU_BASE(object)),
    HDU_FIELD(nf1,n1::TYPE,1)
    HDU_FIELD(f2,TYPE_UINT32,2)
)

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(RocksdbSchema, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(RegisterRocksdbSchema)
{
    ModelRegistry::free();
    rdb::RocksdbSchemas::free();

    auto midx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    auto mo1=unitModel<object::TYPE>(ModelConfig{},midx1);
    using mo1t=decltype(mo1);
    using t1=ModelWithInfo<mo1t>;
    using mt1=typename t1::ModelType;
    static_assert(std::is_same<mt1,mo1t>::value,"");
    mo1t a(mo1);
    auto moi1=makeModelWithInfo(mo1);

    auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    auto idx2=makeIndex(IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},object::created_at);
    auto idx3=makeIndex(IndexConfig<>{},object::updated_at);
    auto m1=unitModel<nu1::TYPE>(ModelConfig{},idx1,idx2,idx3);
    static_assert(std::is_same<nu1::TYPE,decltype(m1)::UnitType>::value,"");

    auto mi1=makeModelWithInfo(m1);
    BOOST_CHECK(mi1->info.nativeModelV()==nullptr);
    BOOST_CHECK(!mi1->info.schema());
    std::string s1Name{"schema1"};
    auto s1=makeSchema(s1Name,mi1);
    BOOST_CHECK(mi1->info.nativeModelV()==nullptr);
    BOOST_REQUIRE(mi1->info.schema());
    BOOST_CHECK_EQUAL(mi1->info.schema()->name(),s1Name);

    rdb::RocksdbSchemas::instance().registerSchema(s1);
    BOOST_CHECK(mi1->info.nativeModelV()!=nullptr);
    auto rs1=rdb::RocksdbSchemas::instance().schema(s1Name);
    BOOST_REQUIRE(rs1);
    auto rm1=rs1->findModel(mi1->info);
    BOOST_REQUIRE(rm1);
    BOOST_REQUIRE(rm1->createObject);

    auto handler=[&s1Name,s1,&mi1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        auto ec=client->addSchema(s1);
        BOOST_REQUIRE(!ec);
        auto s1_=client->schema(s1->name());
        BOOST_REQUIRE(!s1_);
        BOOST_CHECK_EQUAL(s1_->get()->name(),s1->name());
        auto sl=client->listSchemas();
        BOOST_REQUIRE(!sl);
        BOOST_REQUIRE(!sl->empty());
        BOOST_CHECK_EQUAL(sl->at(0)->name(),s1->name());

        std::string s2Name{"schema2"};
        auto s2=makeSchema(s2Name,mi1);
        auto s2_=client->schema(s2->name());
        BOOST_REQUIRE(s2_);
    };
    std::vector<ModelInfo> models{mo1};
    auto today=common::Date::currentUtc();
    auto from=today.copyAddDays(-365);
    auto to=today.copyAddDays(365);
    PartitionRange pr{std::move(models),from,to};
    PrepareDbAndRun::eachPlugin(handler,"createopenclosedestroy.jsonc",pr);
}

BOOST_AUTO_TEST_CASE(ModelCollectionName)
{
    rdb::RocksdbSchemas::free();
    ModelRegistry::free();

    auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    auto idx2=makeIndex(IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},object::created_at);
    auto idx3=makeIndex(DefaultIndexConfig,object::updated_at);

    auto mi2=makeModel<nu1::TYPE>(DefaultModelConfig,idx1,idx2,idx3);
    BOOST_CHECK_EQUAL(mi2->info.collection(),"nu1");
    BOOST_CHECK_EQUAL(mi2->info.modelId(),818672101);

    auto mi3=makeModel<object::TYPE>(ModelConfig{"obj"},idx1,idx2,idx3);
    BOOST_CHECK_EQUAL(mi3->info.collection(),"obj");
    BOOST_CHECK_EQUAL(mi3->info.modelId(),3649981262);
}

BOOST_AUTO_TEST_SUITE_END()
