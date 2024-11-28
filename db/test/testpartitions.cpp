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
HATN_LOGCONTEXT_USING

namespace {

HDU_UNIT(base_fields,
    HDU_FIELD(df2,TYPE_UINT32,2)
    HDU_FIELD(df3,TYPE_STRING,3)
)

HDU_UNIT_WITH(no_p,(HDU_BASE(object),HDU_BASE(base_fields))
)

HDU_UNIT_WITH(explicit_p,(HDU_BASE(object),HDU_BASE(base_fields)),
    HDU_FIELD(pf1,TYPE_DATE,1)
)

HDU_UNIT_WITH(implicit_p,(HDU_BASE(object),HDU_BASE(base_fields))
)

HATN_DB_INDEX(df2Idx,base_fields::df2)
HATN_DB_INDEX(df3Idx,base_fields::df3)

HATN_DB_MODEL(modelNoP,no_p,df2Idx(),df3Idx())

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
    rdb::RocksdbModels::instance().registerModel(modelNoP());
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

    auto s1=initSchema(modelNoP(),modelExplicit1(),modelExplicit2(),modelImplicit1(),modelImplicit2());

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

BOOST_AUTO_TEST_CASE(CrudImplicit)
{
    ContextLogger::instance().setDefaultLogLevel(LogLevel::Debug);

    auto run=[](const auto& model, const auto& obj, const auto& partitionField)
    {
        using type=std::decay_t<decltype(obj)>;

        init();

        auto s1=initSchema(model,modelNoP());
        std::vector<ModelInfo> modelInfos{*(model->info)};
        std::vector<common::Date> dates{
            {2024,1,15},{2024,2,15},{2024,3,15},
            {2024,4,15},{2024,5,15},{2024,6,15},
            {2024,7,15},{2024,8,15},{2024,9,15},
            {2024,10,15},{2024,11,15},{2024,12,15}
        };

        auto handler=[&](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
        {
            size_t objectsPerPartition=5;
            size_t count=dates.size()*objectsPerPartition;
            Topic topic1{"topic1"};

            setSchemaToClient(client,s1);

            auto ec=client->addDatePartitions(modelInfos,dates.back(),dates.front());
            BOOST_REQUIRE(!ec);

            // fill db with objects
            for (size_t i=0;i<count;i++)
            {
                auto o=makeInitObject<type>();
                o.setFieldValue(base_fields::df2,static_cast<uint32_t>(i));
                o.setFieldValue(base_fields::df3,fmt::format("value_{:04d}",i));
                common::DateTime dt{dates[i%dates.size()],common::Time{1,1,1}};
                auto df=o.fieldValue(partitionField);
                if constexpr (std::is_same<decltype(df),du::ObjectId>::value)
                {
                    df.setDateTime(dt);
                    o.setFieldValue(partitionField,df);
                }
                else
                {
                    o.setFieldValue(partitionField,dt.date());
                }
                o.setFieldValue(object::created_at,dt);
                o.setFieldValue(object::updated_at,dt);
                auto ec=client->create(topic1,model,&o);
                if (ec)
                {
                    BOOST_TEST_MESSAGE(fmt::format("Failed to create object {}: {}",i,ec.message()));
                }
                BOOST_REQUIRE(!ec);
            }

            // add object to default partition
            auto o1=makeInitObject<no_p::type>();
            o1.setFieldValue(base_fields::df3,"Hello world!");
            ec=client->create(topic1,modelNoP(),&o1);
            BOOST_REQUIRE(!ec);

            auto findObjects=[&]()
            {
                // find object in default partition
                auto q0=makeQuery(df3Idx(),query::where(base_fields::df3,query::eq,"Hello world!"),topic1);
                auto r0=client->findOne(modelNoP(),q0);
                BOOST_REQUIRE(!r0);
                BOOST_CHECK(r0.value()->fieldValue(base_fields::df3)==lib::string_view("Hello world!"));

                // find all objects from all partitions by partition field
                BOOST_TEST_MESSAGE("Find all by partition field");
                auto q1=makeQuery(oidPartitionIdx(),query::where(object::_id,query::gte,query::First),topic1);
                auto r1=client->find(model,q1);
                if (r1)
                {
                    HATN_CTX_ERROR(r1.error(),"failed to find all by partition field")
                }
                BOOST_REQUIRE(!r1);
                BOOST_CHECK_EQUAL(r1->size(),count);
                for (size_t i=0;i<r1->size();i++)
                {
                    std::cout<<"All by partition field "<<i<<":\n"<<r1->at(i).template as<type>()->toString(true)<<std::endl;
                }

                // find all objects from all partitions by not partition field
                BOOST_TEST_MESSAGE("Find all by not partition field");
                auto q2=makeQuery(df3Idx(),query::where(base_fields::df3,query::gte,query::First),topic1);
                auto r2=client->find(model,q2);
                BOOST_REQUIRE(!r2);
                BOOST_CHECK_EQUAL(r2->size(),count);
                for (size_t i=0;i<r2->size();i++)
                {
                    std::cout<<"All by not partition field "<<i<<":\n"<<r2->at(i).template as<type>()->toString(true)<<std::endl;
                }

                // find objects from partition by partition field with gte
                BOOST_TEST_MESSAGE("Find by partition field with gte");
                ObjectId oid3;
                oid3.setDateTime(common::DateTime(dates[dates.size()-2]));
                std::cout<<"oid3="<<oid3.toString()<<std::endl;
                auto q3=makeQuery(oidPartitionIdx(),query::where(object::_id,query::gte,oid3),topic1);
                auto r3=client->find(model,q3);
                BOOST_REQUIRE(!r3);
                BOOST_CHECK_EQUAL(r3->size(),objectsPerPartition*2);
                for (size_t i=0;i<r3->size();i++)
                {
                    std::cout<<"By gte partition field "<<i<<":\n"<<r3->at(i).template as<type>()->toString(true)<<std::endl;
                }

                // find objects from partition by partition field with lte
                BOOST_TEST_MESSAGE("Find by partition field with lte");
                ObjectId oid4;
                oid4.setDateTime(common::DateTime(dates[1].copyAddDays(1)));
                std::cout<<"oid4="<<oid4.toString()<<std::endl;
                auto q4=makeQuery(oidPartitionIdx(),query::where(object::_id,query::lte,oid4,query::Desc),topic1);
                auto r4=client->find(model,q4);
                BOOST_REQUIRE(!r4);
                BOOST_CHECK_EQUAL(r4->size(),objectsPerPartition*2);
                for (size_t i=0;i<r4->size();i++)
                {
                    std::cout<<"By lte partition field "<<i<<":\n"<<r4->at(i).template as<type>()->toString(true)<<std::endl;
                }
            };

            // find objects in currently opened db
            BOOST_TEST_CONTEXT("original db"){findObjects();};

            // close and re-open database
            ec=client->closeDb();
            if (ec)
            {
                BOOST_TEST_MESSAGE(fmt::format("failed to close database: {}",ec.message()));
            }
            BOOST_REQUIRE(!ec);
            base::config_object::LogRecords records;
            ec=client->openDb(*PrepareDbAndRun::currentCfg,records);
            if (ec)
            {
                BOOST_TEST_MESSAGE(fmt::format("failed to re-open database: {}",ec.message()));
            }
            BOOST_REQUIRE(!ec);

            // find objects in reopened db
            BOOST_TEST_CONTEXT("reopened db"){findObjects();};
        };
        PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
    };

    BOOST_TEST_CONTEXT("modelImplicit1()"){run(modelImplicit1(),implicit_p::type{},object::_id);}
    // BOOST_TEST_CONTEXT("modelImplicit2()"){run(modelImplicit2());}
}

BOOST_AUTO_TEST_SUITE_END()
