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

HATN_DB_PARTITION_INDEX(partitionPf1Idx1,explicit_p::pf1)
HATN_DB_INDEX(pf1Idx1,explicit_p::pf1)
HATN_DB_MODEL_WITH_CFG(modelExplicit1,explicit_p,ModelConfig("explicit_p1"),partitionPf1Idx1(),pf1Idx1(),df2Idx(),df3Idx())

HATN_DB_INDEX(pf1Idx2,explicit_p::pf1,base_fields::df2)
HATN_DB_MODEL_WITH_CFG(modelExplicit2,explicit_p,ModelConfig("explicit_p2"),partitionPf1Idx1(),pf1Idx2(),df3Idx())

HATN_DB_OID_PARTITION_MODEL_WITH_CFG(modelImplicit1,implicit_p,ModelConfig("implicit_p1"),df2Idx(),df3Idx())

HATN_DB_UNIQUE_IN_PARTITION_INDEX(df2Idx2,base_fields::df2)
HATN_DB_OID_PARTITION_MODEL_WITH_CFG(modelImplicit2,implicit_p,ModelConfig("implicit_p2"),df2Idx2(),df3Idx())

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
            auto ec=client->addDatePartitions(modelInfos,common::Date::currentUtc().copyAddMonths(12));
            BOOST_REQUIRE(!ec);

            BOOST_TEST_MESSAGE("List partitions");
            auto ranges=client->listDatePartitions();
            BOOST_REQUIRE(!ranges);
            BOOST_CHECK_EQUAL(ranges->size(),12+1);

            BOOST_TEST_MESSAGE("Delete partitions");
            ec=client->deleteDatePartitions(modelInfos,common::Date::currentUtc().copyAddMonths(6),common::Date::currentUtc());
            BOOST_REQUIRE(!ec);

            BOOST_TEST_MESSAGE("List partitions after delete");
            ranges=client->listDatePartitions();
            BOOST_REQUIRE(!ranges);
            BOOST_CHECK_EQUAL(ranges->size(),6);

            BOOST_TEST_MESSAGE("Try to delete already deleted partitions");
            ec=client->deleteDatePartitions(modelInfos,common::Date::currentUtc().copyAddMonths(6),common::Date::currentUtc());
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

BOOST_AUTO_TEST_CASE(Crud)
{
    // ContextLogger::instance().setDefaultLogLevel(LogLevel::Debug);

    auto run=[](const auto& model,
                  const auto& obj,
                  const auto& partitionField,
                  const auto& queryIdx,
                  const auto& partitionIdx,
                  const auto& genFromDate,
                  bool checkUnique
                  )
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
            std::vector<std::pair<ObjectId,common::Date>> testObjects;

            std::vector<std::decay_t<decltype(obj.fieldValue(partitionField))>> vectorIn;

            setSchemaToClient(client,s1);

            auto ec=client->addDatePartitions(modelInfos,dates.back(),dates.front());
            BOOST_REQUIRE(!ec);

            // fill db with objects
            for (size_t i=0;i<count;i++)
            {
                auto partition=i%dates.size();

                auto o=makeInitObject<type>();
                o.setFieldValue(base_fields::df2,static_cast<uint32_t>(i));
                o.setFieldValue(base_fields::df3,fmt::format("value_{:04d}",i));
                common::DateTime dt{dates[partition],common::Time{1,1,1}};
                if constexpr (std::is_same<std::decay_t<decltype(partitionField)>,std::decay_t<decltype(object::_id)>>::value)
                {
                    auto df=o.fieldValue(partitionField);
                    df.setDateTime(dt);
                    o.setFieldValue(object::_id,df);
                    o.setFieldValue(object::created_at,dt);
                    o.setFieldValue(object::updated_at,dt);
                }
                else
                {
                    o.setFieldValue(partitionField,dt.date());
                }
                ec=client->create(topic1,model,&o);
                if (ec)
                {
                    HATN_CTX_ERROR_RECORDS(ec,"Failed to create object",{"i",i})
                }
                BOOST_REQUIRE(!ec);

                if (partition==1 && i<(objectsPerPartition*3))
                {
                    HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value()->setStackLockingEnabled(true);

                    BOOST_TEST_MESSAGE(fmt::format("Check unique index in partition={}, i={}",partition,i));
                    int ms=checkUnique?2:1;
                    common::DateTime dt{dates[partition],common::Time{1,1,ms}};
                    if (checkUnique)
                    {
                        o=makeInitObject<type>();
                    }
                    o.setFieldValue(base_fields::df2,static_cast<uint32_t>(i));
                    o.setFieldValue(base_fields::df3,fmt::format("value_{:04d}",i));
                    if constexpr (std::is_same<std::decay_t<decltype(partitionField)>,std::decay_t<decltype(object::_id)>>::value)
                    {
                        auto df=o.fieldValue(partitionField);
                        df.setDateTime(dt);
                        o.setFieldValue(object::_id,df);
                        o.setFieldValue(object::created_at,dt);
                        o.setFieldValue(object::updated_at,dt);
                    }
                    else
                    {
                        o.setFieldValue(partitionField,dt.date());
                    }
                    ec=client->create(topic1,model,&o);
                    if (ec)
                    {
                        HATN_CTX_ERROR_RECORDS(ec,"Expected failure on duplicate unique",{"i",i})
                    }
                    BOOST_CHECK(ec);
                    HATN_CTX_SCOPE_UNLOCK()

                    HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value()->setStackLockingEnabled(false);
                }

                if (partition==0 || partition==6 || partition==11)
                {
                    testObjects.emplace_back(o.fieldValue(object::_id),dates[partition]);
                }
                if ( (partition==2 && vectorIn.size()==0)
                     || (partition==7 && vectorIn.size()==1)
                     || (partition==9 && vectorIn.size()==2)
                    )
                {
                    vectorIn.push_back(o.fieldValue(partitionField));
                }
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
                BOOST_TEST_MESSAGE("Find all by partition field with partition query");
                auto q1=makeQuery(queryIdx,
                            query::where(partitionField,query::gte,query::First)
                                        .partitions(partitionIdx,partitionField,query::gte,query::First),
                            topic1);
                auto r1=client->find(model,q1);
                if (r1)
                {
                    HATN_CTX_ERROR(r1.error(),"failed to find all by partitions field")
                }
                BOOST_REQUIRE(!r1);
                BOOST_CHECK_EQUAL(r1->size(),count);

                // check query on partitioned field but without partititions query
                BOOST_TEST_MESSAGE("Find all by partition field without partition query");
#if 0
                auto q1_=makeQuery(queryIdx,
                                    query::where(partitionField,query::gte,query::First),
                                    topic1);
                r1=client->find(model,q1_);
#else
                r1=client->findAllPartitioned(topic1,model);
#endif
                BOOST_REQUIRE(!r1);
                BOOST_CHECK_EQUAL(r1->size(),count);
#if 0
                for (size_t i=0;i<r1->size();i++)
                {
                    std::cout<<"All by partition field "<<i<<":\n"<<r1->at(i).template as<type>()->toString(true)<<std::endl;
                }
#endif

                // find all objects from all partitions by not partition field
                BOOST_TEST_MESSAGE("Find all by not partition field");
                auto q2=makeQuery(df3Idx(),query::where(base_fields::df3,query::gte,query::First),topic1);
                auto r2=client->find(model,q2);
                BOOST_REQUIRE(!r2);
                BOOST_CHECK_EQUAL(r2->size(),count);
#if 0
                for (size_t i=0;i<r2->size();i++)
                {
                    std::cout<<"All by not partition field "<<i<<":\n"<<r2->at(i).template as<type>()->toString(true)<<std::endl;
                }
#endif

                // find objects in some partitions by not partition field
                BOOST_TEST_MESSAGE("Find in some partitions by not partition field");
                auto q2_=makeQuery(df3Idx(),
                                    query::where(base_fields::df3,query::gte,query::First)
                                    .partitions(partitionIdx,partitionField,query::gte,dates[dates.size()-2]),
                                    topic1
                                 );
                auto r2_=client->find(model,q2_);
                BOOST_REQUIRE(!r2_);
                BOOST_CHECK_EQUAL(r2_->size(),objectsPerPartition*2);

                // find objects from partition by partition field with gte
                BOOST_TEST_MESSAGE("Find by partition field with gte");
                auto val3=genFromDate(dates[dates.size()-2]);
                auto q3=makeQuery(queryIdx,query::where(partitionField,query::gte,val3)
                                                      .partitions(partitionIdx,partitionField,query::gte,dates[dates.size()-2])
                                    ,topic1);
                auto r3=client->find(model,q3);
                BOOST_REQUIRE(!r3);
                BOOST_CHECK_EQUAL(r3->size(),objectsPerPartition*2);
#if 0
                for (size_t i=0;i<r3->size();i++)
                {
                    std::cout<<"By gte partition field "<<i<<":\n"<<r3->at(i).template as<type>()->toString(true)<<std::endl;
                }
#endif
                // find objects from partition by partition field with lte
                BOOST_TEST_MESSAGE("Find by partition field with lte");
                auto val4=genFromDate(dates[1].copyAddDays(1));
                auto q4=makeQuery(queryIdx,query::where(partitionField,query::lte,val4,query::Desc)
                                                .partitions(partitionIdx,partitionField,query::lte,dates[1],query::Desc)
                                    ,topic1);
                auto r4=client->find(model,q4);
                BOOST_REQUIRE(!r4);
                BOOST_CHECK_EQUAL(r4->size(),objectsPerPartition*2);
#if 0
                for (size_t i=0;i<r4->size();i++)
                {
                    std::cout<<"By lte partition field "<<i<<":\n"<<r4->at(i).template as<type>()->toString(true)<<std::endl;
                }
#endif
                // find objects from partition by partition field using in vector
                BOOST_TEST_MESSAGE("Find by partition field using in vector");
                auto q5=makeQuery(queryIdx,query::where_partitioned(partitionIdx,partitionField,query::in,vectorIn),topic1);
                auto r5=client->find(model,q5);
                BOOST_REQUIRE(!r5);
                if constexpr (std::is_same<std::decay_t<decltype(partitionField)>,std::decay_t<decltype(object::_id)>>::value)
                {
                    BOOST_CHECK_EQUAL(r5->size(),3);
                }
                else
                {
                    // because query is by date which is only part of the index
                    BOOST_CHECK_EQUAL(r5->size(),3*objectsPerPartition);
                }
#if 0
                for (size_t i=0;i<r5->size();i++)
                {
                    std::cout<<"Partititon field in vector: "<<i<<":\n"<<r5->at(i).template as<type>()->toString(true)<<std::endl;
                }
#endif
                // find objects from partition by partition field in interval
                BOOST_TEST_MESSAGE("Find by partition field in interval");
                auto intv6=query::makeInterval(genFromDate(dates[3].copyAddDays(-14)),
                                    genFromDate(dates[4].copyAddDays(15))
                                    );
                std::cout<<"From "<<intv6.from.value.toString()<< "("<<dates[3].copyAddDays(-14).toString()
                          <<") to "<<
                            intv6.to.value.toString()
                          <<"("<<dates[4].copyAddDays(15).toString()<<")"<<std::endl;
                auto q6=makeQuery(queryIdx,query::where_partitioned(partitionIdx,partitionField,query::in,intv6),topic1);
                auto r6=client->find(model,q6);
                BOOST_REQUIRE(!r6);
                BOOST_CHECK_EQUAL(r6->size(),objectsPerPartition*2);
#if 0
                for (size_t i=0;i<r6->size();i++)
                {
                    std::cout<<"Partititon field in vector: "<<i<<":\n"<<r6->at(i).template as<type>()->toString(true)<<std::endl;
                }
#endif

                // find objects from partition by partition field using in interval [First,to]
                BOOST_TEST_MESSAGE("Find by partition field in interval [First,to]");
                auto intv7=query::makeInterval(query::IntervalType::First,
                                                 genFromDate(dates[2].copyAddDays(15))
                                                 );
                auto q7=makeQuery(queryIdx,query::where_partitioned(partitionIdx,partitionField,query::in,intv7),topic1);
                auto r7=client->find(model,q7);
                BOOST_REQUIRE(!r7);
                BOOST_CHECK_EQUAL(r7->size(),objectsPerPartition*3);
#if 0
                for (size_t i=0;i<r7->size();i++)
                {
                    std::cout<<"Partititon field in vector: "<<i<<":\n"<<r7->at(i).template as<type>()->toString(true)<<std::endl;
                }
#endif

                // find objects from partition by partition field using in interval [from,Last]
                BOOST_TEST_MESSAGE("Find by partition field using in interval [from,Last]");
                auto intv8=query::makeInterval(
                                genFromDate(dates[10].copyAddDays(-14)),
                                query::IntervalType::Last);
                auto q8=makeQuery(queryIdx,query::where_partitioned(partitionIdx,partitionField,query::in,intv8),topic1);
                auto r8=client->find(model,q8);
                BOOST_REQUIRE(!r8);
                BOOST_CHECK_EQUAL(r8->size(),objectsPerPartition*2);
#if 0
                for (size_t i=0;i<r8->size();i++)
                {
                    std::cout<<"Partititon field in vector: "<<i<<":\n"<<r8->at(i).template as<type>()->toString(true)<<std::endl;
                }
#endif

                // find objects from partition by partition field using in interval [First,Last]
                BOOST_TEST_MESSAGE("Find by partition field using in interval [First,Last]");
                auto intv9=query::makeInterval(
                    query::IntervalType::First,
                    query::IntervalType::Last);
                auto q9=makeQuery(queryIdx,query::where_partitioned(partitionIdx,partitionField,query::in,intv9),topic1);
                auto r9=client->find(model,q9);
                BOOST_REQUIRE(!r9);
                BOOST_CHECK_EQUAL(r9->size(),objectsPerPartition*dates.size());
#if 0
                for (size_t i=0;i<r9->size();i++)
                {
                    std::cout<<"Partititon field in vector: "<<i<<":\n"<<r9->at(i).template as<type>()->toString(true)<<std::endl;
                }
#endif
            };

#if 1
            // find objects in currently opened db
            BOOST_TEST_CONTEXT("original db"){findObjects();};
#endif
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
            BOOST_TEST_MESSAGE("Reopen db and re-run objects finding");
#if 1
            BOOST_TEST_CONTEXT("reopened db"){findObjects();};
#endif

#if 1
            // read-update-delete test objects
            size_t updateCount=0;
            for (auto&& it: testObjects)
            {
                if constexpr (std::is_same<std::decay_t<decltype(partitionField)>,std::decay_t<decltype(object::_id)>>::value)
                {
                    auto val1=fmt::format("str_{:04d}",updateCount++);
                    auto updateReq1=update::request(
                        update::field(base_fields::df3,update::set,val1)
                        );
                    auto val2=fmt::format("str_{:04d}",1+updateCount++);
                    auto updateReq2=update::request(
                        update::field(base_fields::df3,update::set,val2)
                        );

                    auto ru=client->update(topic1,model,it.first,updateReq1);
                    BOOST_REQUIRE(!ru);

                    auto r=client->read(topic1,model,it.first);
                    BOOST_REQUIRE(!r);
                    BOOST_CHECK(r.value()->fieldValue(base_fields::df3)==val1);

                    r=client->readUpdate(topic1,model,it.first,updateReq2);
                    BOOST_REQUIRE(!r);
                    BOOST_CHECK(r.value()->fieldValue(base_fields::df3)==val2);

                    auto rd=client->deleteObject(topic1,model,it.first);
                    BOOST_CHECK(!rd);
                    r=client->read(topic1,model,it.first);
                    BOOST_REQUIRE(r);
                }
                else
                {
                    auto val1=fmt::format("str_{:04d}",updateCount++);
                    auto updateReq1=update::request(
                        update::field(base_fields::df3,update::set,val1)
                        );
                    auto val2=fmt::format("str_{:04d}",1+updateCount++);
                    auto updateReq2=update::request(
                        update::field(base_fields::df3,update::set,val2)
                        );
                    auto val3=fmt::format("str_{:04d}",2+updateCount++);
                    auto updateReq3=update::request(
                        update::field(base_fields::df3,update::set,val3)
                        );
                    auto val4=fmt::format("str_{:04d}",3+updateCount++);
                    auto updateReq4=update::request(
                        update::field(base_fields::df3,update::set,val4)
                        );
                    auto val5=fmt::format("str_{:04d}",4+updateCount++);
                    auto updateReq5=update::request(
                        update::field(base_fields::df3,update::set,val5)
                        );
                    auto val6=fmt::format("str_{:04d}",4+updateCount++);
                    auto updateReq6=update::request(
                        update::field(base_fields::df3,update::set,val6)
                        );

                    // update with date
                    auto ru=client->update(topic1,model,it.first,updateReq1,it.second);
                    BOOST_REQUIRE(!ru);
                    // read with date
                    auto r=client->read(topic1,model,it.first,it.second);
                    if (r)
                    {
                        HATN_CTX_ERROR(r.error(),"failed to read object with date")
                    }
                    BOOST_REQUIRE(!r);
                    BOOST_CHECK(r.value()->fieldValue(base_fields::df3)==val1);

                    // update with date in wrong partition
                    ru=client->update(topic1,model,it.first,updateReq3,dates[3]);
                    BOOST_CHECK(ru);
                    HATN_CTX_SCOPE_UNLOCK()
                    // read with date in wrong partition
                    r=client->read(topic1,model,it.first, dates[3]);
                    BOOST_CHECK(r);
                    HATN_CTX_SCOPE_UNLOCK()
                    // read check
                    r=client->read(topic1,model,it.first,it.second);
                    BOOST_REQUIRE(!r);
                    BOOST_CHECK(r.value()->fieldValue(base_fields::df3)==val1);

                    // read-update with date
                    r=client->readUpdate(topic1,model,it.first,updateReq4,it.second);
                    BOOST_REQUIRE(!r);
                    BOOST_CHECK(r.value()->fieldValue(base_fields::df3)==val4);
                    // read-update in wrong partition
                    r=client->readUpdate(topic1,model,it.first,updateReq6,dates[3]);
                    BOOST_CHECK(r);
                    HATN_CTX_SCOPE_UNLOCK()
                    // read check
                    r=client->read(topic1,model,it.first,it.second);
                    BOOST_REQUIRE(!r);
                    BOOST_CHECK(r.value()->fieldValue(base_fields::df3)==val4);

                    // operations in not existing partition
                    auto dt=common::Date{2023,1,1};
                    r=client->read(topic1,model,it.first,dt);
                    BOOST_CHECK(r);
                    HATN_CTX_SCOPE_UNLOCK()
                    r=client->update(topic1,model,it.first,updateReq6,dt);
                    BOOST_CHECK(r);
                    HATN_CTX_SCOPE_UNLOCK()
                    r=client->readUpdate(topic1,model,it.first,updateReq6,dt);
                    BOOST_CHECK(r);
                    HATN_CTX_SCOPE_UNLOCK()
                    auto rd=client->deleteObject(topic1,model,it.first,dt);
                    BOOST_CHECK(rd);
                    HATN_CTX_SCOPE_UNLOCK()

                    // delete objects

                    // try to delete in wrong partition, no error
                    rd=client->deleteObject(topic1,model,it.first,dates[3]);
                    BOOST_CHECK(!rd);
                    // read check
                    r=client->read(topic1,model,it.first,it.second);
                    BOOST_REQUIRE(!r);
                    BOOST_CHECK(r.value()->fieldValue(base_fields::df3)==val4);

                    // delete with date
                    rd=client->deleteObject(topic1,model,it.first,it.second);
                    BOOST_CHECK(!rd);
                    // read check
                    r=client->read(topic1,model,it.first,it.second);
                    BOOST_CHECK(r);
                    HATN_CTX_SCOPE_UNLOCK()
                }
            }
#endif
            // delete objects from default partition
            ec=client->deleteMany(modelNoP(),
                                    makeQuery(oidIdx(),query::where(object::_id,query::gte,query::First),topic1)
                                  );
            BOOST_REQUIRE(!ec);
        };
        PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
    };

    auto oidFromDate=[](const common::Date& date)
    {
        ObjectId oid;
        oid.setDateTime(common::DateTime{date});
        return oid;
    };

    auto dateFromDate=[](const common::Date& date)
    {
        return date;
    };

    BOOST_TEST_CONTEXT("modelImplicit1()"){
        run(modelImplicit1(),implicit_p::type{},object::_id,oidIdx(),oidPartitionIdx(),oidFromDate,false);
    }

    BOOST_TEST_CONTEXT("modelImplicit2()"){
        run(modelImplicit2(),implicit_p::type{},object::_id,oidIdx(),oidPartitionIdx(),oidFromDate,true);
    }

    BOOST_TEST_CONTEXT("modelExplicit1()"){
        run(modelExplicit1(),explicit_p::type{},explicit_p::pf1,pf1Idx1(),partitionPf1Idx1(),dateFromDate,false);
    }

    BOOST_TEST_CONTEXT("modelExplicit2()"){
        run(modelExplicit2(),explicit_p::type{},explicit_p::pf1,pf1Idx2(),partitionPf1Idx1(),dateFromDate,false);
    }
}

BOOST_AUTO_TEST_SUITE_END()
