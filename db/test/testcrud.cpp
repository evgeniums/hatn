/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testcrud.cpp
*/

/****************************************************************************/

#include <cmath>
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>
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
#include <hatn/db/plugins/rocksdb/ipp/rocksdbschema.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {

HDU_UNIT_WITH(simple1,(HDU_BASE(object)),
    HDU_FIELD(f1,TYPE_UINT32,1)
)

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

auto initSimpleSchema() -> decltype(auto)
{
    ModelRegistry::free();
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::free();
#endif

    auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    auto idx2=makeIndex(DefaultIndexConfig,object::created_at);
    auto idx3=makeIndex(DefaultIndexConfig,object::updated_at);
    auto idx4=makeIndex(DefaultIndexConfig,simple1::f1);

    auto simpleModel1=makeModel<simple1::TYPE>(DefaultModelConfig,idx1,idx2,idx3,idx4);
    auto simpleSchema1=makeSchema("schema_simple1",simpleModel1);

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::instance().registerSchema(simpleSchema1);
#endif

    return std::make_tuple(simpleModel1,simpleSchema1,idx4);
}

template <typename T>
void addSchemaToClient(std::shared_ptr<Client> client, const T& schema)
{
    auto ec=client->addSchema(schema);
    BOOST_REQUIRE(!ec);
    auto s=client->schema(schema->name());
    BOOST_REQUIRE(!s);
    BOOST_CHECK_EQUAL(s->get()->name(),schema->name());
    auto sl=client->listSchemas();
    BOOST_REQUIRE(!sl);
    BOOST_REQUIRE(!sl->empty());
    BOOST_CHECK_EQUAL(sl->at(0)->name(),schema->name());
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestCrud, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(Simple1)
{
    auto s=initSimpleSchema();

    auto handler=[&s](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        auto& s1=std::get<1>(s);
        auto& m1=std::get<0>(s);
        auto& idx4=std::get<2>(s);

        std::ignore=plugin;
        addSchemaToClient(client,s1);

        Namespace ns{"topic1"};
        BOOST_CHECK_EQUAL(std::string("topic1"),std::string(ns.topic()));

        auto o1=makeInitObject<simple1::type>();
        o1.setFieldValue(simple1::f1,100);
        const auto& id=o1.fieldValue(object::_id);
        BOOST_TEST_MESSAGE(fmt::format("Original o1: {}",o1.toString(true)));

        // try to read unknown object
        auto r1=client->read(ns,m1,id);
        if (r1)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected: {}",r1.error().message()));
        }
        BOOST_CHECK(r1);

        // create object
        auto ec=client->create(ns,m1,&o1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // read object
        auto r2=client->read(ns,m1,id);
        if (r2)
        {
            BOOST_TEST_MESSAGE(r2.error().message());
        }
        BOOST_REQUIRE(!r2);
        auto oid1=o1.fieldValue(object::_id);
        auto oid2=r2.value()->fieldValue(object::_id);
        BOOST_TEST_MESSAGE(fmt::format("Read r2: {}",r2.value()->toString(true)));
        BOOST_TEST_MESSAGE(fmt::format("oid1: {}, tp={}, seq={}, rand={}",oid1.toString(),
                                                                          oid1.timepoint(),
                                                                          oid1.seq(),oid1.rand()
                                       ));
        BOOST_TEST_MESSAGE(fmt::format("oid2: {}, tp={}, seq={}, rand={}",oid2.toString(),
                                                                          oid2.timepoint(),
                                                                          oid2.seq(),oid2.rand()
                                       ));
        BOOST_CHECK(oid1==oid2);
        BOOST_CHECK_EQUAL(oid1.timepoint(),oid2.timepoint());
        BOOST_CHECK_EQUAL(oid1.seq(),oid2.seq());
        BOOST_CHECK_EQUAL(oid1.rand(),oid2.rand());
        BOOST_CHECK_EQUAL(r2.value()->fieldValue(simple1::f1),o1.fieldValue(simple1::f1));
        BOOST_CHECK_EQUAL(oid1.toString(),oid2.toString());
        BOOST_CHECK(r2.value()->fieldValue(object::created_at)==o1.fieldValue(object::created_at));
        BOOST_CHECK(r2.value()->fieldValue(object::updated_at)==o1.fieldValue(object::updated_at));

        // find object
        IndexQuery q1{idx4,ns.topic()};
        query::Field qf1{IndexFieldInfo{"f1",1},query::Operator::eq,100};
        q1.setField(qf1);
        auto r3=client->find(ns,m1,q1);
        if (r3)
        {
            BOOST_TEST_MESSAGE(r3.error().message());
        }
        BOOST_CHECK(!r3);
        BOOST_CHECK_EQUAL(r3->size(),1);
        // BOOST_CHECK_EQUAL(r3.value()->fieldValue(simple1::f1),o1.fieldValue(simple1::f1));
        // BOOST_CHECK(r3.value()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        // BOOST_CHECK(r3.value()->fieldValue(object::created_at)==o1.fieldValue(object::created_at));
        // BOOST_CHECK(r3.value()->fieldValue(object::updated_at)==o1.fieldValue(object::updated_at));
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
