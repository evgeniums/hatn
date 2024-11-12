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
)

HATN_DB_INDEX(idx4,simple1::f1)

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

auto initSimpleSchema() -> decltype(auto)
{
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));

    ModelRegistry::free();
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::free();
    rdb::RocksdbModels::free();
#endif

    // auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    // auto idx2=makeIndex(DefaultIndexConfig,object::created_at);
    // auto idx3=makeIndex(DefaultIndexConfig,object::updated_at);
    // auto idx4=makeIndex(DefaultIndexConfig,simple1::f1);

    auto simpleModel1=makeModel<simple1::TYPE>(DefaultModelConfig,/*idx1,idx2,idx3,*/idx4());
    auto simpleSchema1=makeSchema("schema_simple1",simpleModel1);

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(simpleModel1);
    rdb::RocksdbSchemas::instance().registerSchema(simpleSchema1);
#endif

    BOOST_TEST_MESSAGE(fmt::format("idx4 ID={}",idx4().id()));

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
    auto s=initSimpleSchema();

    auto handler=[&s](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        auto& s1=std::get<1>(s);
        auto& m1=std::get<0>(s);

        std::ignore=plugin;
        setSchemaToClient(client,s1);

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
#if 0
        auto w1=query::where(simple1::f1,query::Operator::eq,100);
        const auto& wf1=hana::at(w1.conditions,hana::size_c<0>);
        query::Field qf1{idx4.fieldInfo(hana::at(wf1,hana::size_c<0>)),
                           hana::at(wf1,hana::size_c<1>),
                           hana::at(wf1,hana::size_c<2>),
                           hana::at(wf1,hana::size_c<3>)
                        };
        common::pmr::vector<query::Field> fs1;

        fs1.emplace_back(idx4.fieldInfo(hana::at(wf1,hana::size_c<0>)),
                         hana::at(wf1,hana::size_c<1>),
                         hana::at(wf1,hana::size_c<2>),
                         hana::at(wf1,hana::size_c<3>)
                         );

        auto emplaceField=[&fs1,&idx4](const auto& cond)
        {
            fs1.emplace_back(
                idx4.fieldInfo(hana::at(cond,hana::size_c<0>)),
                hana::at(cond,hana::size_c<1>),
                hana::at(cond,hana::size_c<2>),
                hana::at(cond,hana::size_c<3>)
                );
        };

        emplaceField(hana::at(w1.conditions,hana::size_c<0>));

        fs1.reserve(w1.size());
        hana::for_each(
            w1.conditions,
            emplaceField
        );

        IndexQuery q1{idx4,ns.topic()};
        query::Field qf1{idx4.fieldInfo(simple1::f1),query::Operator::eq,100};
        q1.setField(qf1);
#endif
        auto q3=makeQuery(idx4(),ns.topic(),query::where(simple1::f1,query::Operator::eq,100));
        auto r3=client->find(ns,m1,q3);
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
        auto r3_=client->findOne(ns,m1,q3);
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
        auto q4=makeQuery(idx4(),ns.topic(),query::where(simple1::f1,query::Operator::eq,101));
        auto r4=client->find(ns,m1,q4);
        if (r4)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected: {}",r4.error().message()));
        }
        BOOST_REQUIRE(!r4);
        BOOST_CHECK(r4->empty());
        auto r4_=client->findOne(ns,m1,q4);
        if (r4_)
        {
            BOOST_TEST_MESSAGE(r4_.error().message());
        }
        BOOST_REQUIRE(!r4_);
        BOOST_REQUIRE(r4_.value().isNull());

        // update object
        BOOST_REQUIRE_EQUAL(std::string(simple1::f1.name()),std::string("f1"));
        update::FieldInfo finf{simple1::f1};
        BOOST_REQUIRE_EQUAL(finf.name(),std::string("f1"));
        auto update1=update::Request{
            {finf,update::Operator::set,101}
        };
        ec=client->update(ns,m1,update1,id);
        BOOST_REQUIRE(!ec);
        // read updated object
        auto r5=client->read(ns,m1,id);
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
        auto r6_=client->findOne(ns,m1,q3);
        if (r6_)
        {
            BOOST_TEST_MESSAGE(r6_.error().message());
        }
        BOOST_REQUIRE(!r6_);
        BOOST_CHECK(r6_.value().isNull());
        // find updated object
        auto r6=client->findOne(ns,m1,q4);
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
        ec=client->deleteObject(ns,m1,id);
        BOOST_REQUIRE(!ec);
        auto r7=client->read(ns,m1,id);
        if (r7)
        {
            BOOST_TEST_MESSAGE(fmt::format("Expected read after delete: {}",r7.error().message()));
        }
        BOOST_CHECK(r7);
        auto r7_=client->findOne(ns,m1,q4);
        if (r7_)
        {
            BOOST_TEST_MESSAGE(r7_.error().message());
        }
        BOOST_REQUIRE(!r7_);
        BOOST_CHECK(r7_.value().isNull());
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()