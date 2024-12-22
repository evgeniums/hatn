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

HDU_UNIT_WITH(simple1,(HDU_BASE(object)),
    HDU_FIELD(f1,TYPE_UINT32,1)
    HDU_FIELD(f2,TYPE_STRING,2)
)

HATN_DB_INDEX(idx4,simple1::f1)

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

auto initSimpleSchema()
{
    ModelRegistry::free();
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::free();
    rdb::RocksdbModels::free();
#endif

    auto simpleModel1=makeModel<simple1::TYPE>(DefaultModelConfig,idx4());
    auto simpleSchema1=makeSchema("schema_simple1",simpleModel1);

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(simpleModel1);
    rdb::RocksdbSchemas::instance().registerSchema(simpleSchema1);
#endif

    // ID of index outside model is empty
    BOOST_CHECK_EQUAL(idx4().id(),"");

    return std::make_tuple(simpleModel1,simpleSchema1);
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

BOOST_AUTO_TEST_SUITE(TestCrud, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(Simple1)
{
    HATN_CTX_SCOPE("Test Simple1")
    HATN_CTX_INFO("Test start")

    auto s=initSimpleSchema();

    auto handler=[&s](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        auto& s1=std::get<1>(s);
        auto& m1=std::get<0>(s);

        std::ignore=plugin;
        setSchemaToClient(client,s1);

        Topic topic{"topic1"};
        BOOST_CHECK_EQUAL(std::string("topic1"),std::string(topic.topic()));

        auto o1=makeInitObject<simple1::type>();
        o1.setFieldValue(simple1::f1,100);
        o1.setFieldValue(simple1::f2,"hi");
        const auto& id=o1.fieldValue(object::_id);
        BOOST_TEST_MESSAGE(fmt::format("Original o1: {}",o1.toString(true)));

        // try to read unknown object
        auto r1=client->read(topic,m1,id);
        if (r1)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected: {}",r1.error().message()));
        }
        BOOST_CHECK(r1);

        // create object
        auto ec=client->create(topic,m1,&o1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // read object
        auto r2=client->read(topic,m1,id);
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
        BOOST_CHECK_EQUAL(r2.value()->fieldValue(simple1::f1),100);
        BOOST_CHECK_EQUAL(r2.value()->fieldValue(simple1::f2),std::string("hi"));

        // find object in topic
        auto q3=makeQuery(idx4(),query::where(simple1::f1,query::Operator::eq,100),topic.topic());
        BOOST_TEST_MESSAGE(fmt::format("topic={}",topic.topic()));
        BOOST_REQUIRE_EQUAL(q3.topics().size(),1);
        BOOST_CHECK_EQUAL(q3.topics().at(0).topic(),topic.topic());
        auto r3=client->find(m1,q3);
        if (r3)
        {
            BOOST_TEST_MESSAGE(r3.error().message());
        }
        BOOST_REQUIRE(!r3);
        BOOST_REQUIRE_EQUAL(r3->size(),1);
        const auto* o3=r3->at(0).unit<simple1::type>();
        BOOST_CHECK_EQUAL(o3->fieldValue(simple1::f1),o1.fieldValue(simple1::f1));
        BOOST_CHECK(o3->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK(o3->fieldValue(object::created_at)==o1.fieldValue(object::created_at));
        BOOST_CHECK(o3->fieldValue(object::updated_at)==o1.fieldValue(object::updated_at));
        auto r3_=client->findOne(m1,q3);
        if (r3_)
        {
            BOOST_TEST_MESSAGE(r3_.error().message());
        }
        BOOST_REQUIRE(!r3_);
        BOOST_CHECK_EQUAL(r3_.value()->fieldValue(simple1::f1),o1.fieldValue(simple1::f1));
        BOOST_CHECK(r3_.value()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK(r3_.value()->fieldValue(object::created_at)==o1.fieldValue(object::created_at));
        BOOST_CHECK(r3_.value()->fieldValue(object::updated_at)==o1.fieldValue(object::updated_at));

        // try to find unknown object
        auto q4=makeQuery(idx4(),query::where(simple1::f1,query::Operator::eq,101),"topic1");
        auto r4=client->find(m1,q4);
        if (r4)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected: {}",r4.error().message()));
        }
        BOOST_REQUIRE(!r4);
        BOOST_CHECK(r4->empty());
        auto r4_=client->findOne(m1,q4);
        if (r4_)
        {
            BOOST_TEST_MESSAGE(r4_.error().message());
        }
        BOOST_REQUIRE(!r4_);
        BOOST_REQUIRE(r4_.value().isNull());

        // update object
        BOOST_REQUIRE_EQUAL(std::string(simple1::f1.name()),std::string("f1"));
        db::FieldInfo finf{simple1::f1};
        BOOST_REQUIRE_EQUAL(finf.name(),std::string("f1"));
        auto update1=update::makeRequest(
            update::Field(update::path(simple1::f1),update::set,101)
        );
        ec=client->update(topic,m1,id,update1);
        BOOST_REQUIRE(!ec);
        // read updated object
        auto r5=client->read(topic,m1,id);
        if (r5)
        {
            BOOST_TEST_MESSAGE(r5.error().message());
        }
        BOOST_REQUIRE(!r5);
        BOOST_TEST_MESSAGE(fmt::format("Read r5: {}",r5.value()->toString(true)));
        BOOST_CHECK_EQUAL(r5.value()->fieldValue(simple1::f1),101);
        BOOST_CHECK(r5.value()->fieldValue(object::_id)==id);
        BOOST_CHECK(r5.value()->fieldValue(object::created_at)==o1.fieldValue(object::created_at));
        BOOST_CHECK(r5.value()->fieldValue(object::updated_at)>=o1.fieldValue(object::updated_at));
        // try to find original object
        auto r6_=client->findOne(m1,q3);
        if (r6_)
        {
            BOOST_TEST_MESSAGE(r6_.error().message());
        }
        BOOST_REQUIRE(!r6_);
        BOOST_CHECK(r6_.value().isNull());
        // find updated object
        auto r6=client->findOne(m1,q4);
        if (r6)
        {
            BOOST_TEST_MESSAGE(r6.error().message());
        }
        BOOST_REQUIRE(!r6);
        BOOST_REQUIRE(!r6.value().isNull());
        BOOST_CHECK_EQUAL(r6.value()->fieldValue(simple1::f1),101);
        BOOST_CHECK(r6.value()->fieldValue(object::_id)==id);
        BOOST_CHECK(r6.value()->fieldValue(object::created_at)==o1.fieldValue(object::created_at));
        BOOST_CHECK(r6.value()->fieldValue(object::updated_at)==r5.value()->fieldValue(object::updated_at));

        // delete object
        ec=client->deleteObject(topic,m1,id);
        BOOST_REQUIRE(!ec);
        auto r7=client->read(topic,m1,id);
        if (r7)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected read after delete: {}",r7.error().message()));
        }
        BOOST_CHECK(r7);
        auto r7_=client->findOne(m1,q4);
        if (r7_)
        {
            BOOST_TEST_MESSAGE(r7_.error().message());
        }
        BOOST_REQUIRE(!r7_);
        BOOST_CHECK(r7_.value().isNull());
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");

    HATN_CTX_INFO("Test finish")
}

BOOST_AUTO_TEST_SUITE_END()
