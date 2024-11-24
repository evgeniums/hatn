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

    registerModels1();
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

BOOST_AUTO_TEST_SUITE(TestFind, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(OneLevel)
{
    init();

    auto s1=initSchema(m1_bool());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        auto o1=makeInitObject<u1_bool::type>();
        BOOST_TEST_MESSAGE(fmt::format("Original o1: {}",o1.toString(true)));
        o1.setFieldValue(u1_bool::f1,false);
        BOOST_TEST_MESSAGE(fmt::format("o1 after f1 set: {}",o1.toString(true)));

        // create object
        auto ec=client->create(topic1,m1_bool(),&o1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // find object by f_bool
        auto q=makeQuery(u1_bool_f1_idx(),query::where(u1_bool::f1,query::Operator::eq,false));
        std::cout<<"operand type="<<static_cast<int>(q.fields().at(0).value.typeId())<<std::endl;
        std::cout<<"operand value="<<static_cast<bool>(q.fields().at(0).value.as<query::BoolValue>())<<std::endl;
        common::FmtAllocatedBufferChar buf;
        HATN_ROCKSDB_NAMESPACE::fieldValueToBuf(buf,q.fields().at(0));
        std::cout<<"operand string="<<common::fmtBufToString(buf)<<std::endl;

        auto r1=client->findOne(m1_bool(),makeQuery(u1_bool_f1_idx(),query::where(u1_bool::f1,query::Operator::eq,false),topic1));
        if (r1)
        {
            BOOST_TEST_MESSAGE(r1.error().message());
        }
        BOOST_REQUIRE(!r1);
        BOOST_CHECK(!r1.value().isNull());
        BOOST_CHECK(r1.value().topic()==topic1);

        auto r2=client->findOne(m1_bool(),makeQuery(u1_bool_f1_idx(),query::where(u1_bool::f1,query::Operator::eq,true),topic1));
        BOOST_REQUIRE(!r2);
        BOOST_CHECK(r2.value().isNull());        
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(NullIndex)
{
    init();

    auto s1=initSchema(m1_uint32());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        // create object1 with field set
        auto o1=makeInitObject<u1_uint32::type>();
        o1.setFieldValue(u1_uint32::f1,0xaabb);
        auto ec=client->create(topic1,m1_uint32(),&o1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // create object2 with field not set
        auto o2=makeInitObject<u1_uint32::type>();
        ec=client->create(topic1,m1_uint32(),&o2);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // create object3 with field set
        auto o3=makeInitObject<u1_uint32::type>();
        o3.setFieldValue(u1_uint32::f1,0xccdd);
        ec=client->create(topic1,m1_uint32(),&o3);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // find object 2 with field not set
        auto q=makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::Operator::eq,query::Null),topic1);
        auto r1=client->findOne(m1_uint32(),q);
        if (r1)
        {
            BOOST_TEST_MESSAGE(r1.error().message());
        }
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE(!r1.value().isNull());
        BOOST_CHECK(r1.value()->fieldValue(object::_id)==o2.fieldValue(object::_id));

        // find not Null objects
        auto r2=client->find(m1_uint32(),makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::neq,query::Null),topic1));
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2.value().size(),2);
        BOOST_CHECK(r2.value().at(0).unit<u1_uint32::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK(r2.value().at(1).unit<u1_uint32::type>()->fieldValue(object::_id)==o3.fieldValue(object::_id));

        // find not Null objects with gte
        r2=client->find(m1_uint32(),makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::gte,0xaabb),topic1));
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2.value().size(),2);
        BOOST_CHECK(r2.value().at(0).unit<u1_uint32::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK(r2.value().at(1).unit<u1_uint32::type>()->fieldValue(object::_id)==o3.fieldValue(object::_id));
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(SortAndLimit)
{
    init();

    auto s1=initSchema(m1_uint32());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        // create object1 with field set
        auto o1=makeInitObject<u1_uint32::type>();
        o1.setFieldValue(u1_uint32::f1,0xaabb);
        auto ec=client->create(topic1,m1_uint32(),&o1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // create object2 with field not set
        auto o2=makeInitObject<u1_uint32::type>();
        ec=client->create(topic1,m1_uint32(),&o2);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // create object3 with field set
        auto o3=makeInitObject<u1_uint32::type>();
        o3.setFieldValue(u1_uint32::f1,0xccdd);
        ec=client->create(topic1,m1_uint32(),&o3);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // create object4 with field set
        auto o4=makeInitObject<u1_uint32::type>();
        o4.setFieldValue(u1_uint32::f1,0xeeee);
        ec=client->create(topic1,m1_uint32(),&o4);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        // find objects with default limit and Asc ordering
        auto q1=makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::gte,0xaabb),topic1);
        auto r2=client->find(m1_uint32(),q1);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2.value().size(),3);
        BOOST_CHECK(r2.value().at(0).unit<u1_uint32::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK(r2.value().at(1).unit<u1_uint32::type>()->fieldValue(object::_id)==o3.fieldValue(object::_id));
        BOOST_CHECK(r2.value().at(2).unit<u1_uint32::type>()->fieldValue(object::_id)==o4.fieldValue(object::_id));

        // find objects with default limit and Desc ordering
        auto q2=makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::gte,0xaabb,query::Desc),topic1);
        r2=client->find(m1_uint32(),q2);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2.value().size(),3);
        BOOST_CHECK(r2.value().at(2).unit<u1_uint32::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK(r2.value().at(1).unit<u1_uint32::type>()->fieldValue(object::_id)==o3.fieldValue(object::_id));
        BOOST_CHECK(r2.value().at(0).unit<u1_uint32::type>()->fieldValue(object::_id)==o4.fieldValue(object::_id));

        // find objects with limit and Asc ordering
        q1.setLimit(2);
        r2=client->find(m1_uint32(),q1);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2.value().size(),2);
        BOOST_CHECK(r2.value().at(0).unit<u1_uint32::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK(r2.value().at(1).unit<u1_uint32::type>()->fieldValue(object::_id)==o3.fieldValue(object::_id));

        // find objects with limit and Desc ordering
        q2.setLimit(2);
        r2=client->find(m1_uint32(),q2);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2.value().size(),2);
        BOOST_CHECK(r2.value().at(1).unit<u1_uint32::type>()->fieldValue(object::_id)==o3.fieldValue(object::_id));
        BOOST_CHECK(r2.value().at(0).unit<u1_uint32::type>()->fieldValue(object::_id)==o4.fieldValue(object::_id));
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(OffsetAndLimit)
{
    init();

    auto s1=initSchema(m1_uint32());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        // fill db with objects
        uint32_t startVal=1000000;
        size_t count=50;
        for (size_t i=0;i<count;i++)
        {
            uint32_t val=static_cast<uint32_t>(i+startVal);
            auto o=makeInitObject<u1_uint32::type>();
            o.setFieldValue(u1_uint32::f1,val);
            auto ec=client->create(topic1,m1_uint32(),&o);
            BOOST_REQUIRE(!ec);
        }

        // (offset+limit) is below count
        size_t offset=10;
        size_t limit=15;
        auto q3=makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::gte,startVal),topic1);
        q3.setOffset(offset);
        q3.setLimit(limit);
        auto r2=client->find(m1_uint32(),q3);
        BOOST_REQUIRE(!r2);
        size_t foundCount=limit;
        BOOST_REQUIRE_EQUAL(r2.value().size(),limit);
        BOOST_CHECK_EQUAL(r2.value().at(0).unit<u1_uint32::type>()->fieldValue(u1_uint32::f1),startVal+offset);
        BOOST_CHECK_EQUAL(r2.value().at(foundCount-1).unit<u1_uint32::type>()->fieldValue(u1_uint32::f1),startVal+offset+foundCount-1);

        // (offset+limit) is above count
        offset=count-10;
        limit=15;
        q3.setOffset(offset);
        q3.setLimit(limit);
        r2=client->find(m1_uint32(),q3);
        BOOST_REQUIRE(!r2);
        foundCount=count-offset;
        BOOST_REQUIRE_EQUAL(r2.value().size(),foundCount);
        BOOST_CHECK_EQUAL(r2.value().at(0).unit<u1_uint32::type>()->fieldValue(u1_uint32::f1),startVal+offset);
        BOOST_CHECK_EQUAL(r2.value().at(foundCount-1).unit<u1_uint32::type>()->fieldValue(u1_uint32::f1),startVal+count-1);

        // offset is above count
        offset=count+10;
        limit=15;
        q3.setOffset(offset);
        q3.setLimit(limit);
        r2=client->find(m1_uint32(),q3);
        BOOST_REQUIRE(!r2);
        foundCount=0;
        BOOST_REQUIRE_EQUAL(r2.value().size(),foundCount);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(Count)
{
    init();

    auto s1=initSchema(m1_uint32());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};

        // fill db with objects
        uint32_t startVal=1000000;
        size_t count=50;
        for (size_t i=0;i<count;i++)
        {
            uint32_t val=static_cast<uint32_t>(i+startVal);
            auto o=makeInitObject<u1_uint32::type>();
            o.setFieldValue(u1_uint32::f1,val);
            auto ec=client->create(topic1,m1_uint32(),&o);
            BOOST_REQUIRE(!ec);
        }

        // count all
        auto q1=makeQuery(oidIdx(),query::where(object::_id,query::gte,query::First),topic1);
        auto r1=client->count(m1_uint32(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1.value(),count);

        // (offset+limit) is below count
        size_t offset=10;
        size_t limit=15;
        auto q3=makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::gte,startVal),topic1);
        q3.setOffset(offset);
        q3.setLimit(limit);
        auto r2=client->count(m1_uint32(),q3);
        BOOST_REQUIRE(!r2);
        size_t foundCount=limit;
        BOOST_REQUIRE_EQUAL(r2.value(),limit);

        // (offset+limit) is above count
        offset=count-10;
        limit=15;
        q3.setOffset(offset);
        q3.setLimit(limit);
        r2=client->count(m1_uint32(),q3);
        BOOST_REQUIRE(!r2);
        foundCount=count-offset;
        BOOST_REQUIRE_EQUAL(r2.value(),foundCount);

        // offset is above count
        offset=count+10;
        limit=15;
        q3.setOffset(offset);
        q3.setLimit(limit);
        r2=client->count(m1_uint32(),q3);
        BOOST_REQUIRE(!r2);
        foundCount=0;
        BOOST_REQUIRE_EQUAL(r2.value(),foundCount);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(UniqueIndex)
{
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));
    auto ctx=HATN_COMMON_NAMESPACE::makeTaskContext<HATN_LOGCONTEXT_NAMESPACE::ContextWrapper>();
    ctx->beforeThreadProcessing();

    HATN_CTX_SCOPE("Test UniqueIndex")
    HATN_CTX_INFO("Test start")

    init();

    auto s1=initSchema(m1_str());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Error ec;
        Topic topic1{"topic1"};
        std::string str1{"Unique string 1"};
        std::string str2{"Not unique string"};
        std::string str3{"Unique string 2"};
        std::string str4{"Unique string 3"};

        auto q0=makeQuery(u1_str_f2_idx(),query::where(u1_str::f2,query::eq,str2),topic1);

        HATN_CTX_INFO("create object1 with unique field")
        auto o1=makeInitObject<u1_str::type>();
        o1.setFieldValue(u1_str::f1,str1);
        o1.setFieldValue(u1_str::f2,str2);
        ec=client->create(topic1,m1_str(),&o1);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        HATN_CTX_INFO("read object o1")
        auto  r0=client->read(topic1,m1_str(),o1.fieldValue(object::_id));
        BOOST_REQUIRE(!r0);
        BOOST_TEST_MESSAGE(fmt::format("Read o0: {}", r0.value()->toString(true)));
        BOOST_CHECK(r0.value()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK_EQUAL(r0.value()->fieldValue(u1_str::f2),str2);
        HATN_CTX_INFO("find objects by not unique field 1")
        auto r1=client->find(m1_str(),q0);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1.value().size(),1);
        BOOST_CHECK(r1.value().at(0).unit<u1_str::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_TEST_MESSAGE(fmt::format("Find o1: {}", r1.value().at(0).unit<u1_str::type>()->toString(true)));

        HATN_CTX_INFO("create object1 with the same unique field without non unique field")
        auto o2=makeInitObject<u1_str::type>();
        o2.setFieldValue(u1_str::f1,str1);
        ec=client->create(topic1,m1_str(),&o2);
        if (ec)
        {
            HATN_CTX_ERROR(ec,"expected error 1")
            HATN_CTX_SCOPE_UNLOCK()
        }
        BOOST_CHECK(ec);

        HATN_CTX_INFO("find objects by not unique field 2")
        r1=client->find(m1_str(),q0);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1.value().size(),1);
        BOOST_CHECK(r1.value().at(0).unit<u1_str::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));

        HATN_CTX_INFO("create object2 with the same unique field and with non unique field")
        auto o2_2=makeInitObject<u1_str::type>();
        o2_2.setFieldValue(u1_str::f1,str1);
        o2_2.setFieldValue(u1_str::f2,str2);
        ec=client->create(topic1,m1_str(),&o2_2);
        if (ec)
        {
            HATN_CTX_ERROR(ec,"expected error 2")
            HATN_CTX_SCOPE_UNLOCK()
        }
        BOOST_CHECK(ec);

        HATN_CTX_INFO("find objects by not unique field 3")
        r1=client->find(m1_str(),q0);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1.value().size(),1);
        BOOST_CHECK(r1.value().at(0).unit<u1_str::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));

        HATN_CTX_INFO("create object3 with new unique field")
        auto o3=makeInitObject<u1_str::type>();
        o3.setFieldValue(u1_str::f1,str3);
        o3.setFieldValue(u1_str::f2,str2);
        ec=client->create(topic1,m1_str(),&o3);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        HATN_CTX_INFO("read object o3")
        auto  r2=client->read(topic1,m1_str(),o3.fieldValue(object::_id));
        BOOST_REQUIRE(!r2);
        BOOST_TEST_MESSAGE(fmt::format("Read o3: {}", r2.value()->toString(true)));
        BOOST_CHECK(r2.value()->fieldValue(object::_id)==o3.fieldValue(object::_id));
        BOOST_CHECK_EQUAL(r2.value()->fieldValue(u1_str::f2),str2);

        HATN_CTX_INFO("find objects by not unique field 4")
        r1=client->find(m1_str(),q0);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1.value().size(),2);
        BOOST_CHECK(r1.value().at(0).unit<u1_str::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK(r1.value().at(1).unit<u1_str::type>()->fieldValue(object::_id)==o3.fieldValue(object::_id));

        HATN_CTX_INFO("create object4 with new unique field")
        auto o4=makeInitObject<u1_str::type>();
        o4.setFieldValue(u1_str::f1,str4);
        o4.setFieldValue(u1_str::f2,str2);
        ec=client->create(topic1,m1_str(),&o4);
        if (ec)
        {
            BOOST_TEST_MESSAGE(ec.message());
        }
        BOOST_REQUIRE(!ec);

        HATN_CTX_INFO("find objects by not unique field 5")
        auto r3=client->find(m1_str(),q0);
        if (r3)
        {
            BOOST_TEST_MESSAGE(r3.error().message());
        }
        BOOST_REQUIRE(!r3);
        BOOST_REQUIRE_EQUAL(r3.value().size(),3);
        BOOST_CHECK(r3.value().at(0).unit<u1_str::type>()->fieldValue(object::_id)==o1.fieldValue(object::_id));
        BOOST_CHECK(r3.value().at(1).unit<u1_str::type>()->fieldValue(object::_id)==o3.fieldValue(object::_id));
        BOOST_CHECK(r3.value().at(2).unit<u1_str::type>()->fieldValue(object::_id)==o4.fieldValue(object::_id));

        BOOST_TEST_MESSAGE(fmt::format("Find o3: {}", r3.value().at(1).unit<u1_str::type>()->toString(true)));
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");

    HATN_CTX_INFO("Test end")
    ctx->afterThreadProcessing();
}

BOOST_AUTO_TEST_CASE(DeleteWithQuery)
{
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));
    auto ctx=HATN_COMMON_NAMESPACE::makeTaskContext<HATN_LOGCONTEXT_NAMESPACE::ContextWrapper>();
    ctx->beforeThreadProcessing();

    HATN_CTX_SCOPE("Test UniqueIndex")
    HATN_CTX_INFO("Test start")

    init();

    auto s1=initSchema(m1_uint32());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Error ec;
        Topic topic1{"topic1"};
        uint32_t v0=0;
        uint32_t v1=100;
        uint32_t v2=200;
        uint32_t v3=300;

        // fill db with objects
        for (size_t i=0;i<50;i++)
        {
            size_t offset=v0;

            if (i>=10 && i<20)
            {
                offset=v1;
            }
            else if (i>=20 && i<30)
            {
                offset=v2;
            }
            else if (i>=30)
            {
                offset=v3;
            }
            uint32_t val=static_cast<uint32_t>(i+offset);

            auto o=makeInitObject<u1_uint32::type>();
            o.setFieldValue(u1_uint32::f1,val);
            ec=client->create(topic1,m1_uint32(),&o);
            BOOST_REQUIRE(!ec);
        }

        // check objects
        auto q1=makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::in,query::makeInterval(v0,v1)),topic1);
        auto r1=client->find(m1_uint32(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1.value().size(),10);
        auto q2=makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::in,query::makeInterval(v1,v2)),topic1);
        auto r2=client->find(m1_uint32(),q2);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2.value().size(),10);
        auto q3=makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::in,query::makeInterval(v2,v3)),topic1);
        auto r3=client->find(m1_uint32(),q3);
        BOOST_REQUIRE(!r3);
        BOOST_REQUIRE_EQUAL(r3.value().size(),10);
        auto q4=makeQuery(u1_uint32_f1_idx(),query::where(u1_uint32::f1,query::gte,v3),topic1);
        auto r4=client->find(m1_uint32(),q4);
        BOOST_REQUIRE(!r4);
        BOOST_REQUIRE_EQUAL(r4.value().size(),20);

        // delete objects
        ec=client->deleteMany(m1_uint32(),q2);
        BOOST_REQUIRE(!ec);

        // check objects after delete
        r1=client->find(m1_uint32(),q1);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1.value().size(),10);
        r2=client->find(m1_uint32(),q2);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2.value().size(),0);
        r3=client->find(m1_uint32(),q3);
        BOOST_REQUIRE(!r3);
        BOOST_REQUIRE_EQUAL(r3.value().size(),10);
        r4=client->find(m1_uint32(),q4);
        BOOST_REQUIRE(!r4);
        BOOST_REQUIRE_EQUAL(r4.value().size(),20);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");

    HATN_CTX_INFO("Test end")
    ctx->afterThreadProcessing();
}

BOOST_AUTO_TEST_SUITE_END()


/** @todo Test:
 *
 *  1. Test find scalar types - done
 *  2. Test find strings - done
 *  3. Test find Oid/DateTime/date/Time/DateRange - done
 *  4. Test find intervals - done
 *  5. Test vectors - done
 *  6. Test ordering - done
 *  7. Test timepoint filtering
 *  8. Test limits - done
 *  9. Test partitions
 *  10. Test TTL
 *  11. Test nested index fields - done
 *  12. Test compound indexes - done
 *  13. Test repeated field indexes
 *  14. Test query offset - done
 *  15. Test delete with query - done
 *  16. Test find-update-create - done
 *  17. Test update - done
 *  18. Test find Null - done
 *  19. Test unique indexes - done
 *  20. Test vectors of intervals
 *  21. Test transactions
 *  22. Test update nested fields
 *  23. Test update arrays
 *  24. Test count - done
 *  25. Test multiple topics: find/findOne/findUpdate/findUpdateCreate
 */
