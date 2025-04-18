/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testupdate.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include <hatn/db/schema.h>
#include <hatn/db/update.h>
#include <hatn/db/ipp/updateunit.ipp>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

namespace tt = boost::test_tools;

#include "modelplain.h"
#include "findplain.h"

#include "updatescalarops.ipp"

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

BOOST_AUTO_TEST_SUITE(TestUpdate, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(FieldPathMap)
{
    std::multimap<FieldPath,int,FieldPathCompare> s;

    auto p1=fieldPath(FieldInt8);
    auto p2=fieldPath(FieldInt16);

    s.insert(std::make_pair(p1,10));
    s.insert(std::make_pair(p2,20));

    BOOST_REQUIRE_EQUAL(s.size(),2);

    BOOST_CHECK(s.find(p1)!=s.end());
    BOOST_CHECK(s.find(p2)!=s.end());
}

BOOST_AUTO_TEST_CASE(SetSingle)
{
    setSingle<plain::type>();
}

BOOST_AUTO_TEST_CASE(UnsetSingle)
{
    unsetSingle<plain::type>();
}

BOOST_AUTO_TEST_CASE(IncSingle)
{
    incSingle<plain::type>();
}

BOOST_AUTO_TEST_CASE(MultipleFields)
{
    multipleFields<plain::type>();
}

BOOST_AUTO_TEST_CASE(Bytes)
{
    testBytes<plain::type>();
}

BOOST_AUTO_TEST_CASE(CheckIndexes)
{
    HATN_CTX_SCOPE("CheckIndexes")

    init();

    auto s1=initSchema(modelPlain());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        checkIndexes<plain::type>(modelPlain(),client);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(UpdateMany)
{
    HATN_CTX_SCOPE("UpdateMany")

    init();

    auto s1=initSchema(modelPlain());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        int16_t val1=100;
        uint32_t val2=1000;
        size_t count=100;

        for (size_t i=0;i<count;i++)
        {
            auto o1=makeInitObject<plain::type>();
            o1.setFieldValue(FieldInt16,val1+i);
            o1.setFieldValue(FieldUInt32,val2+i);

            // create object in db
            auto ec=client->create(topic1,modelPlain(),&o1);
            BOOST_REQUIRE(!ec);
        }

        // find all objects by first field
        auto q1=makeQuery(IdxInt16,query::where(FieldInt16,query::lt,val1+count),topic1);
        auto r1=client->find(modelPlain(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value().size(),count);

        // find all object by second field
        auto q2=makeQuery(IdxUInt32,query::where(FieldUInt32,query::lt,val2+count),topic1);
        auto r2=client->find(modelPlain(),q2);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value().size(),count);

        // find half of objects by first field
        auto q3=makeQuery(IdxInt16,query::where(FieldInt16,query::lt,val1+count/2),topic1);
        r1=client->find(modelPlain(),q3);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value().size(),count/2);

        // find half of object by second field
        auto q4=makeQuery(IdxUInt32,query::where(FieldUInt32,query::lt,val2+count/2),topic1);
        r2=client->find(modelPlain(),q4);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value().size(),count/2);

        // update objects
        auto inc=count;
        auto request=update::request(
            update::field(FieldInt16,update::inc,inc)
        );
        auto ru=client->updateMany(modelPlain(),q3,request);
        BOOST_REQUIRE(!ru);
        BOOST_CHECK_EQUAL(ru.value(),count/2);

        // find half of objects by first field - changed
        r1=client->find(modelPlain(),q3);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value().size(),0);

        // find all of objects by first field - changed
        r1=client->find(modelPlain(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value().size(),count/2);

        // find half of object by second field - did not change
        r2=client->find(modelPlain(),q4);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value().size(),count/2);

        // find not existant
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(AllTopics)
{
    HATN_CTX_SCOPE("AllTopics")

    init();

    auto s1=initSchema(modelPlain());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        std::vector<Topic> topics{"topic1","topic2","topic3","topic4"};

        int16_t val1=100;
        uint32_t val2=1000;
        size_t count=100;

        for (size_t i=0;i<count;i++)
        {
            auto o1=makeInitObject<plain::type>();
            o1.setFieldValue(FieldInt16,val1+i);
            o1.setFieldValue(FieldUInt32,val2+i);

            // create object in db
            auto ec=client->create(topics[i%topics.size()],modelPlain(),&o1);
            BOOST_REQUIRE(!ec);
        }

        // find all objects by first field
        auto q1=makeQuery(IdxInt16,query::where(FieldInt16,query::lt,val1+count));
        auto r1=client->find(modelPlain(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value().size(),count);

        // find all object by second field
        auto q2=makeQuery(IdxUInt32,query::where(FieldUInt32,query::lt,val2+count));
        auto r2=client->find(modelPlain(),q2);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value().size(),count);

        // find half of objects by first field
        auto q3=makeQuery(IdxInt16,query::where(FieldInt16,query::lt,val1+count/2));
        r1=client->find(modelPlain(),q3);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value().size(),count/2);

        // find half of object by second field
        auto q4=makeQuery(IdxUInt32,query::where(FieldUInt32,query::lt,val2+count/2));
        r2=client->find(modelPlain(),q4);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value().size(),count/2);

        // update objects
        auto inc=count;
        auto request=update::request(
            update::field(FieldInt16,update::inc,inc)
            );
        auto ru=client->updateMany(modelPlain(),q3,request);
        BOOST_REQUIRE(!ru);
        BOOST_CHECK_EQUAL(ru.value(),count/2);

        // find half of objects by first field - changed
        r1=client->find(modelPlain(),q3);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value().size(),0);

        // find all of objects by first field - changed
        r1=client->find(modelPlain(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value().size(),count/2);

        // find half of object by second field - did not change
        r2=client->find(modelPlain(),q4);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value().size(),count/2);

        // find not existant
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(ReadUpdate)
{
    HATN_CTX_SCOPE("ReadUpdate")

    init();

    auto s1=initSchema(modelPlain());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        int16_t val1=100;
        uint32_t val2=1000;
        size_t count=10;

        // create objects
        ObjectId middleOid;
        for (size_t i=0;i<count;i++)
        {
            auto o1=makeInitObject<plain::type>();
            o1.setFieldValue(FieldInt16,val1+i);
            o1.setFieldValue(FieldUInt32,val2+i);

            if (i==count/2)
            {
                middleOid=o1.fieldValue(object::_id);
            }

            // create object in db
            auto ec=client->create(topic1,modelPlain(),&o1);
            BOOST_REQUIRE(!ec);
        }

        // find all objects
        auto r1=client->findAll(topic1,modelPlain());
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1->size(),count);
        for (size_t i=0;i<count;i++)
        {
            BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldInt16),val1+i);
            BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldUInt32),val2+i);
        }

        // read and update object, return before
        auto updateReq1=update::request(
            update::field(FieldInt16,update::set,1000)
        );
        auto r2=client->readUpdate(topic1,modelPlain(),middleOid,updateReq1,update::ReturnBefore);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE(!r2.value().isNull());
        BOOST_CHECK_EQUAL(r2.value()->fieldValue(FieldInt16),val1+count/2);
        BOOST_CHECK_EQUAL(r2.value()->fieldValue(FieldUInt32),val2+count/2);

        // read updated object
        auto r2_=client->read(topic1,modelPlain(),middleOid);
        BOOST_REQUIRE(!r2_);
        BOOST_REQUIRE(!r2_.value().isNull());
        BOOST_CHECK_EQUAL(r2_.value()->fieldValue(FieldInt16),1000);
        BOOST_CHECK_EQUAL(r2_.value()->fieldValue(FieldUInt32),val2+count/2);

        // read and update object, return after
        auto updateReq2=update::request(
            update::field(FieldInt16,update::set,2000)
            );
        auto r3=client->readUpdate(topic1,modelPlain(),middleOid,updateReq2,update::ReturnAfter);
        BOOST_REQUIRE(!r3);
        BOOST_REQUIRE(!r3.value().isNull());
        BOOST_CHECK_EQUAL(r3.value()->fieldValue(FieldInt16),2000);
        BOOST_CHECK_EQUAL(r3.value()->fieldValue(FieldUInt32),val2+count/2);

        // read updated object
        auto r3_=client->read(topic1,modelPlain(),middleOid);
        BOOST_REQUIRE(!r3_);
        BOOST_REQUIRE(!r3_.value().isNull());
        BOOST_CHECK_EQUAL(r3_.value()->fieldValue(FieldInt16),2000);
        BOOST_CHECK_EQUAL(r3_.value()->fieldValue(FieldUInt32),val2+count/2);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(FindUpdate)
{
    HATN_CTX_SCOPE("FindUpdate")

    init();

    auto s1=initSchema(modelPlain());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        int16_t val1=100;
        uint32_t val2=1000;
        size_t count=10;

        // create objects
        ObjectId middleOid;
        for (size_t i=0;i<count;i++)
        {
            auto o1=makeInitObject<plain::type>();
            o1.setFieldValue(FieldInt16,val1+i);
            o1.setFieldValue(FieldUInt32,val2+i);

            if (i==count/2)
            {
                middleOid=o1.fieldValue(object::_id);
            }

            // create object in db
            auto ec=client->create(topic1,modelPlain(),&o1);
            BOOST_REQUIRE(!ec);
        }
        // find all objects
        auto q1=makeQuery(oidIdx(),query::where(object::_id,query::gte,query::First),topic1);
        auto r1=client->find(modelPlain(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1->size(),count);
        for (size_t i=0;i<count;i++)
        {
            BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldInt16),val1+i);
            BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldUInt32),val2+i);
        }

        // find and update object, return before
        auto q2=makeQuery(IdxInt16,query::where(FieldInt16,query::eq,val1+count/2),topic1);
        auto updateReq1=update::request(
            update::field(FieldInt16,update::set,1000)
            );
        auto r2=client->findUpdate(modelPlain(),q2,updateReq1,update::ReturnBefore);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE(!r2.value().isNull());
        BOOST_CHECK_EQUAL(r2.value()->fieldValue(FieldInt16),val1+count/2);
        BOOST_CHECK_EQUAL(r2.value()->fieldValue(FieldUInt32),val2+count/2);

        // read updated object
        auto r2_=client->read(topic1,modelPlain(),middleOid);
        BOOST_REQUIRE(!r2_);
        BOOST_REQUIRE(!r2_.value().isNull());
        BOOST_CHECK_EQUAL(r2_.value()->fieldValue(FieldInt16),1000);
        BOOST_CHECK_EQUAL(r2_.value()->fieldValue(FieldUInt32),val2+count/2);

        // find and update object, return after
        auto q3=makeQuery(IdxUInt32,query::where(FieldUInt32,query::eq,val2+count/2),topic1);
        auto updateReq2=update::request(
            update::field(FieldUInt32,update::set,2000)
            );
        auto r3=client->findUpdate(modelPlain(),q3,updateReq2,update::ReturnAfter);
        BOOST_REQUIRE(!r3);
        BOOST_REQUIRE(!r3.value().isNull());
        BOOST_CHECK_EQUAL(r3.value()->fieldValue(FieldInt16),1000);
        BOOST_CHECK_EQUAL(r3.value()->fieldValue(FieldUInt32),2000);

        // read updated object
        auto r3_=client->read(topic1,modelPlain(),middleOid);
        BOOST_REQUIRE(!r3_);
        BOOST_REQUIRE(!r3_.value().isNull());
        BOOST_CHECK_EQUAL(r3_.value()->fieldValue(FieldInt16),1000);
        BOOST_CHECK_EQUAL(r3_.value()->fieldValue(FieldUInt32),2000);

        // check objects
        r1=client->find(modelPlain(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1->size(),count);
        for (size_t i=0;i<count;i++)
        {
            if (i==count/2)
            {
                BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldInt16),1000);
                BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldUInt32),2000);
            }
            else
            {
                BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldInt16),val1+i);
                BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldUInt32),val2+i);
            }
        }
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(FindUpdateCreate)
{
    HATN_CTX_SCOPE("FindUpdateCreate")

    init();

    auto s1=initSchema(modelPlain());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        int16_t val1=100;
        uint32_t val2=1000;
        size_t count=10;

        // create objects
        ObjectId middleOid;
        for (size_t i=0;i<count;i++)
        {
            auto o1=makeInitObject<plain::type>();
            o1.setFieldValue(FieldInt16,val1+i);
            o1.setFieldValue(FieldUInt32,val2+i);

            if (i==count/2)
            {
                middleOid=o1.fieldValue(object::_id);
            }

            // create object in db
            auto ec=client->create(topic1,modelPlain(),&o1);
            BOOST_REQUIRE(!ec);
        }
        // find all objects
        auto q1=makeQuery(oidIdx(),query::where(object::_id,query::gte,query::First),topic1);
        auto r1=client->find(modelPlain(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1->size(),count);
        for (size_t i=0;i<count;i++)
        {
            BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldInt16),val1+i);
            BOOST_CHECK_EQUAL(r1->at(i).unit<plain::type>()->fieldValue(FieldUInt32),val2+i);
        }

        // try to update unknown object
        uint32_t newUInt32=10000;
        int16_t newInt16=1000;
        auto q2=makeQuery(IdxUInt32,query::where(FieldUInt32,query::eq,newUInt32),topic1);
        auto updateReq1=update::request(
            update::field(FieldInt16,update::set,newInt16)
        );
        auto r2=client->updateMany(modelPlain(),q2,updateReq1);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2.value(),0);

        // create not existent object, return before
        auto o2Ptr=makeInitObjectPtr<plain::type>();
        auto& o2=*o2Ptr;
        o2.setFieldValue(FieldInt16,newInt16);
        o2.setFieldValue(FieldUInt32,newUInt32);
        auto r3=client->findUpdateCreate(modelPlain(),q2,updateReq1,o2Ptr,update::ReturnBefore);
        BOOST_REQUIRE(!r3);
        BOOST_CHECK(r3.value().isNull());

        // find created object
        auto r3_=client->find(modelPlain(),q2);
        BOOST_REQUIRE(!r3_);
        BOOST_REQUIRE_EQUAL(r3_->size(),1);
        BOOST_CHECK_EQUAL(r3_->at(0).unit<plain::type>()->fieldValue(FieldInt16),newInt16);
        BOOST_CHECK_EQUAL(r3_->at(0).unit<plain::type>()->fieldValue(FieldUInt32),newUInt32);
        BOOST_CHECK(r3_->at(0).unit<plain::type>()->fieldValue(object::_id)==o2.fieldValue(object::_id));

        // update existent object, return before
        int16_t newInt16_2=2000;
        auto updateReq2=update::request(
            update::field(FieldInt16,update::set,newInt16_2)
        );
        auto o3Ptr=makeInitObjectPtr<plain::type>();
        auto& o3=*o3Ptr;
        o3.setFieldValue(FieldInt16,newInt16_2);
        o3.setFieldValue(FieldUInt32,newUInt32);
        auto r4=client->findUpdateCreate(modelPlain(),q2,updateReq2,o3Ptr,update::ReturnBefore);
        BOOST_REQUIRE(!r4);
        BOOST_REQUIRE(!r4.value().isNull());
        BOOST_CHECK_EQUAL(r4.value()->fieldValue(FieldInt16),newInt16);
        BOOST_CHECK_EQUAL(r4.value()->fieldValue(FieldUInt32),newUInt32);
        BOOST_CHECK(r4.value()->fieldValue(object::_id)==o2.fieldValue(object::_id));

        // find updated object
        auto r5=client->find(modelPlain(),q2);
        BOOST_REQUIRE(!r5);
        BOOST_REQUIRE_EQUAL(r5->size(),1);
        BOOST_CHECK_EQUAL(r5->at(0).unit<plain::type>()->fieldValue(FieldInt16),newInt16_2);
        BOOST_CHECK_EQUAL(r5->at(0).unit<plain::type>()->fieldValue(FieldUInt32),newUInt32);
        BOOST_CHECK(r5->at(0).unit<plain::type>()->fieldValue(object::_id)==o2.fieldValue(object::_id));

        // update existent object, return after
        int16_t newInt16_3=3000;
        auto updateReq3=update::request(
            update::field(FieldInt16,update::set,newInt16_3)
            );
        auto o4Ptr=makeInitObjectPtr<plain::type>();
        auto& o4=*o4Ptr;
        o4.setFieldValue(FieldInt16,newInt16_3);
        o4.setFieldValue(FieldUInt32,newUInt32);
        auto r6=client->findUpdateCreate(modelPlain(),q2,updateReq3,o4Ptr,update::ReturnAfter);
        BOOST_REQUIRE(!r6);
        BOOST_REQUIRE(!r6.value().isNull());
        BOOST_CHECK_EQUAL(r6.value()->fieldValue(FieldInt16),newInt16_3);
        BOOST_CHECK_EQUAL(r6.value()->fieldValue(FieldUInt32),newUInt32);
        BOOST_CHECK(r6.value()->fieldValue(object::_id)==o2.fieldValue(object::_id));

        // create not existent object, return after
        uint32_t newUInt32_2=20000;
        int16_t newInt16_4=5000;
        auto updateReq4=update::request(
            update::field(FieldInt16,update::set,newInt16_4)
            );
        auto o5Ptr=makeInitObjectPtr<plain::type>();
        auto& o5=*o5Ptr;
        o5.setFieldValue(FieldInt16,newInt16_4);
        o5.setFieldValue(FieldUInt32,newUInt32_2);
        auto q3=makeQuery(IdxUInt32,query::where(FieldUInt32,query::eq,newUInt32_2),topic1);
        auto r7=client->findUpdateCreate(modelPlain(),q3,updateReq4,o5Ptr,update::ReturnAfter);
        BOOST_REQUIRE(!r7);
        BOOST_REQUIRE(!r7.value().isNull());
        BOOST_CHECK_EQUAL(r7.value()->fieldValue(FieldInt16),newInt16_4);
        BOOST_CHECK_EQUAL(r7.value()->fieldValue(FieldUInt32),newUInt32_2);
        BOOST_CHECK(r7.value()->fieldValue(object::_id)==o5.fieldValue(object::_id));

        // find created object
        auto r7_=client->find(modelPlain(),q3);
        BOOST_REQUIRE(!r7_);
        BOOST_REQUIRE_EQUAL(r7_->size(),1);
        BOOST_CHECK_EQUAL(r7_->at(0).unit<plain::type>()->fieldValue(FieldInt16),newInt16_4);
        BOOST_CHECK_EQUAL(r7_->at(0).unit<plain::type>()->fieldValue(FieldUInt32),newUInt32_2);
        BOOST_CHECK(r7_->at(0).unit<plain::type>()->fieldValue(object::_id)==o5.fieldValue(object::_id));

        // find all objects
        r1=client->find(modelPlain(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1->size(),count+2);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
