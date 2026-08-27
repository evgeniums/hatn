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

// Retraction of a stale index entry after a PARTIAL field update.
//
// The case: a bool field that is part of a composite index's key, left UNSET at creation, then
// flipped to true via db::update::field() (not a whole-object save). The object's own row is
// updated correctly, but a query that filters on that field AT THE INDEX LAYER can still return
// it -- the old index entry was never retracted, so the scan still finds the record at its
// previous key position and returns content that has since changed underneath it.
//
// Established 2026-08-27 by running the identical scenario against both index kinds: the
// retraction works on a NON-UNIQUE composite index and fails on a UNIQUE one. That is the whole
// finding, so both are kept below -- modelUniqueFlag() (fails) and modelPlainFlag() (passes) are
// identical in every respect except HATN_DB_UNIQUE_INDEX vs HATN_DB_INDEX.
//
// NOTE on probing an unset field: `flag` has no explicit default (HDU_FIELD), so an unset value
// encodes as a Null sentinel that is byte-different from an explicit `false`. `eq false`
// therefore does NOT match a never-set row -- only an `in` over a From-First interval spans both
// Null and false. Every "is it still on the not-true side" probe below uses that interval for
// exactly this reason; asking with `eq false` reports 0 for untouched records and looks like a
// second bug when it is only the documented Null semantics.
template <typename ModelT>
auto notTrueInterval()
{
    return query::Interval<query::BoolValue>(false,query::IntervalType::First,
                                             false,query::IntervalType::Closed);
}

// All records currently on the not-true side of the index (Null or explicit false).
template <typename ModelT, typename IdxT, typename SortFieldT, typename FlagFieldT>
auto notTrueQuery(std::shared_ptr<Client> client, Topic topic,
                  const ModelT& model, const IdxT& idx, SortFieldT sortField, FlagFieldT flagField)
{
    auto notTrue=notTrueInterval<ModelT>();
    auto w=query::where(sortField,query::lte,query::Last).and_(flagField,query::in,notTrue);
    auto q=makeQuery(idx,std::move(w),topic);
    return client->find(model,q);
}

// All records explicitly on the true side.
template <typename ModelT, typename IdxT, typename SortFieldT, typename FlagFieldT>
auto trueQuery(std::shared_ptr<Client> client, Topic topic,
               const ModelT& model, const IdxT& idx, SortFieldT sortField, FlagFieldT flagField)
{
    auto w=query::where(sortField,query::lte,query::Last).and_(flagField,query::eq,true);
    auto q=makeQuery(idx,std::move(w),topic);
    return client->find(model,q);
}

// Single-record probes, pinning the leading sort field to one exact value so the query can only
// ever match that one object -- this is what proves the finding is about the index entry itself
// and not about range/interval semantics.
template <typename ModelT, typename IdxT, typename SortFieldT, typename FlagFieldT>
auto oneRecordNotTrue(std::shared_ptr<Client> client, Topic topic, const ObjectId& sortVal,
                      const ModelT& model, const IdxT& idx, SortFieldT sortField, FlagFieldT flagField)
{
    auto notTrue=notTrueInterval<ModelT>();
    auto w=query::where(sortField,query::eq,sortVal).and_(flagField,query::in,notTrue);
    auto q=makeQuery(idx,std::move(w),topic);
    return client->find(model,q);
}

template <typename ModelT, typename IdxT, typename SortFieldT, typename FlagFieldT>
auto oneRecordTrue(std::shared_ptr<Client> client, Topic topic, const ObjectId& sortVal,
                   const ModelT& model, const IdxT& idx, SortFieldT sortField, FlagFieldT flagField)
{
    auto w=query::where(sortField,query::eq,sortVal).and_(flagField,query::eq,true);
    auto q=makeQuery(idx,std::move(w),topic);
    return client->find(model,q);
}

// The scenario itself, run identically against either model. `startExplicitFalse` selects
// whether flag begins as a Null sentinel or as an explicit false, which isolates whether the
// Null starting position is what matters. `useTransaction` runs the update inside an explicit
// transaction, since a transaction's write-batch could plausibly handle index retraction
// differently from the auto-committing single-call path.
template <typename ModelT, typename IdxT, typename UnitT, typename SortFieldT, typename FlagFieldT>
void runFlagUpdateScenario(std::shared_ptr<Client> client,
                           const ModelT& model, const IdxT& idx,
                           SortFieldT sortField, FlagFieldT flagField,
                           bool startExplicitFalse, bool useTransaction)
{
    Topic topic1{"topic1"};

    std::vector<ObjectId> oids;
    std::vector<ObjectId> sorts;
    for (size_t i=0;i<3;i++)
    {
        auto o=makeInitObject<UnitT>();
        // sort MUST be set: an unset field encodes as a Null sentinel that the leading
        // `lte Last` clause does not match, which would make every query below return 0.
        auto sortVal=ObjectId::generateId();
        o.setFieldValue(sortField,sortVal);
        if (startExplicitFalse)
        {
            o.setFieldValue(flagField,false);
        }
        auto ec=client->create(topic1,model,&o);
        BOOST_REQUIRE(!ec);
        oids.push_back(o.fieldValue(object::_id));
        sorts.push_back(sortVal);
    }

    // sanity: all three sit on the not-true side to begin with
    auto r0=notTrueQuery(client,topic1,model,idx,sortField,flagField);
    BOOST_REQUIRE(!r0);
    BOOST_REQUIRE_EQUAL(r0.value().size(),3);

    // the partial field update under test
    auto updateReq=update::request(update::field(flagField,update::set,true));
    if (useTransaction)
    {
        auto txFn=[&](Transaction* tx) -> Error
        {
            return client->update(topic1,model,oids[0],updateReq,tx);
        };
        auto ec=client->transaction(txFn);
        BOOST_REQUIRE(!ec);
    }
    else
    {
        auto ur=client->update(topic1,model,oids[0],updateReq);
        BOOST_REQUIRE(!ur);
    }

    // the row's own content is always correct -- only the index is ever in question
    auto r1=client->read(topic1,model,oids[0]);
    BOOST_REQUIRE(!r1);
    BOOST_CHECK_EQUAL(r1.value()->fieldValue(flagField),true);

    // per-record probes first: these are the decisive ones, since each pins `sort` to a single
    // exact value and so cannot be explained by range semantics
    for (size_t i=0;i<3;i++)
    {
        BOOST_TEST_CONTEXT("i="<<i)
        {
            auto asNotTrue=oneRecordNotTrue(client,topic1,sorts[i],model,idx,sortField,flagField);
            BOOST_REQUIRE(!asNotTrue);
            auto asTrue=oneRecordTrue(client,topic1,sorts[i],model,idx,sortField,flagField);
            BOOST_REQUIRE(!asTrue);
            BOOST_TEST_MESSAGE(fmt::format("record[{}]{} indexed under not-true={} true={}",
                                           i,(i==0?" (UPDATED)":""),
                                           asNotTrue.value().size(),asTrue.value().size()));
            if (i==0)
            {
                // the updated record must have MOVED: off the not-true side, onto the true side
                BOOST_CHECK_EQUAL(asNotTrue.value().size(),0);
                BOOST_CHECK_EQUAL(asTrue.value().size(),1);
            }
            else
            {
                BOOST_CHECK_EQUAL(asNotTrue.value().size(),1);
                BOOST_CHECK_EQUAL(asTrue.value().size(),0);
            }
        }
    }

    // aggregate view of the same thing
    auto rTrue=trueQuery(client,topic1,model,idx,sortField,flagField);
    BOOST_REQUIRE(!rTrue);
    BOOST_TEST_MESSAGE(fmt::format("aggregate: true -> {} (expect 1)",rTrue.value().size()));
    BOOST_CHECK_EQUAL(rTrue.value().size(),1);

    auto r2=notTrueQuery(client,topic1,model,idx,sortField,flagField);
    BOOST_REQUIRE(!r2);
    BOOST_TEST_MESSAGE(fmt::format("aggregate: not-true -> {} (expect 2)",r2.value().size()));
    BOOST_CHECK_EQUAL(r2.value().size(),2);
}

// --- the failing model: UNIQUE composite index ---

BOOST_AUTO_TEST_CASE(UniqueIndexFlagUpdateFromUnset)
{
    HATN_CTX_SCOPE("UniqueIndexFlagUpdateFromUnset")
    init();
    auto s1=initSchema(modelUniqueFlag());
    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        runFlagUpdateScenario<decltype(modelUniqueFlag()),decltype(uidxflag_sort_idx()),uidxflag::type>(
            client,modelUniqueFlag(),uidxflag_sort_idx(),uidxflag::sort,uidxflag::flag,false,false);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(UniqueIndexFlagUpdateFromExplicitFalse)
{
    HATN_CTX_SCOPE("UniqueIndexFlagUpdateFromExplicitFalse")
    init();
    auto s1=initSchema(modelUniqueFlag());
    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        runFlagUpdateScenario<decltype(modelUniqueFlag()),decltype(uidxflag_sort_idx()),uidxflag::type>(
            client,modelUniqueFlag(),uidxflag_sort_idx(),uidxflag::sort,uidxflag::flag,true,false);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(UniqueIndexFlagUpdateInTransaction)
{
    HATN_CTX_SCOPE("UniqueIndexFlagUpdateInTransaction")
    init();
    auto s1=initSchema(modelUniqueFlag());
    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        runFlagUpdateScenario<decltype(modelUniqueFlag()),decltype(uidxflag_sort_idx()),uidxflag::type>(
            client,modelUniqueFlag(),uidxflag_sort_idx(),uidxflag::sort,uidxflag::flag,false,true);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

// --- the control: identical scenario on a NON-UNIQUE index, which passes ---
// If this ever starts failing too, the bug is not unique-index-specific after all and the
// diagnosis above needs revisiting.

BOOST_AUTO_TEST_CASE(PlainIndexFlagUpdateFromUnset)
{
    HATN_CTX_SCOPE("PlainIndexFlagUpdateFromUnset")
    init();
    auto s1=initSchema(modelPlainFlag());
    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        runFlagUpdateScenario<decltype(modelPlainFlag()),decltype(nidxflag_sort_idx()),nidxflag::type>(
            client,modelPlainFlag(),nidxflag_sort_idx(),nidxflag::sort,nidxflag::flag,false,false);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(PlainIndexFlagUpdateInTransaction)
{
    HATN_CTX_SCOPE("PlainIndexFlagUpdateInTransaction")
    init();
    auto s1=initSchema(modelPlainFlag());
    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        runFlagUpdateScenario<decltype(modelPlainFlag()),decltype(nidxflag_sort_idx()),nidxflag::type>(
            client,modelPlainFlag(),nidxflag_sort_idx(),nidxflag::sort,nidxflag::flag,false,true);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
