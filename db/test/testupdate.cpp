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

#include "modelplain.h"
#include "findplain.h"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace tt = boost::test_tools;

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

template <typename FieldT>
void checkOtherFields(
        const Unit& o,
        FieldT&& testField
    )
{
    BOOST_TEST_CONTEXT(testField.name()){
        o.iterateFieldsConst(
            [&](const du::Field& field)
            {
                if (field.getID()!=testField.id())
                {
                    BOOST_CHECK(!field.isSet());
                }
                return true;
            }
        );
    }
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
    plain::type o;

    o.setFieldValue(FieldInt8,10);
    BOOST_CHECK_EQUAL(o.fieldValue(FieldInt8),10);
    checkOtherFields(o,FieldInt8);
    o.reset();

    auto checkScalar=[&o](const auto& field, auto val)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::set,val)));
        BOOST_CHECK_EQUAL(o.fieldValue(field),val);
        checkOtherFields(o,field);
        o.reset();
    };
    auto checkExtra=[&o](const auto& field, auto val)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::set,val)));
        BOOST_CHECK(o.fieldValue(field)==val);
        checkOtherFields(o,field);
        o.reset();
    };

    checkScalar(FieldInt8,int8_t(0xaa));
    checkScalar(FieldInt16,int16_t(0xaabb));
    checkScalar(FieldInt32,int32_t(0xaabbcc));
    checkScalar(FieldInt64,int64_t(0xaabbccddee));
    checkScalar(FieldUInt8,uint8_t(0xaa));
    checkScalar(FieldUInt16,uint16_t(0xaabb));
    checkScalar(FieldUInt32,uint32_t(0xaabbcc));
    checkScalar(FieldUInt64,uint64_t(0xaabbccddee));
    checkScalar(FieldBool,true);
    checkScalar(FieldString,"hello world");
    checkScalar(FieldFixedString,"hi");
    checkExtra(FieldDateTime,common::DateTime::currentUtc());
    checkExtra(FieldDate,common::Date::currentUtc());
    checkExtra(FieldTime,common::Time::currentUtc());
    checkExtra(FieldDateRange,common::DateRange(common::Date::currentUtc()));
    checkExtra(FieldObjectId,ObjectId::generateId());
    checkExtra(FieldEnum,plain::MyEnum::Two);
}

BOOST_AUTO_TEST_CASE(UnsetSingle)
{
    plain::type o;

    auto checkScalar=[&o](const auto& field, auto val)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::set,val)));
        BOOST_CHECK_EQUAL(o.fieldValue(field),val);
        BOOST_CHECK(o.field(field).isSet());
        checkOtherFields(o,field);
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::unset)));
        BOOST_CHECK(!o.field(field).isSet());
        o.reset();
    };
    auto checkExtra=[&o](const auto& field, auto val)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::set,val)));
        BOOST_CHECK(o.fieldValue(field)==val);
        BOOST_CHECK(o.field(field).isSet());
        checkOtherFields(o,field);
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::unset)));
        BOOST_CHECK(!o.field(field).isSet());
        o.reset();
    };

    checkScalar(FieldInt8,int8_t(0xaa));
    checkScalar(FieldInt16,int16_t(0xaabb));
    checkScalar(FieldInt32,int32_t(0xaabbcc));
    checkScalar(FieldInt64,int64_t(0xaabbccddee));
    checkScalar(FieldUInt8,uint8_t(0xaa));
    checkScalar(FieldUInt16,uint16_t(0xaabb));
    checkScalar(FieldUInt32,uint32_t(0xaabbcc));
    checkScalar(FieldUInt64,uint64_t(0xaabbccddee));
    checkScalar(FieldBool,true);
    checkScalar(FieldString,"hello world");
    checkScalar(FieldFixedString,"hi");
    checkExtra(FieldDateTime,common::DateTime::currentUtc());
    checkExtra(FieldDate,common::Date::currentUtc());
    checkExtra(FieldTime,common::Time::currentUtc());
    checkExtra(FieldDateRange,common::DateRange(common::Date::currentUtc()));
    checkExtra(FieldObjectId,ObjectId::generateId());
    checkExtra(FieldEnum,plain::MyEnum::Two);
}

BOOST_AUTO_TEST_CASE(IncSingle)
{
    plain::type o;

    auto checkScalar=[&o](const auto& field, auto val)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::set,val)));
        BOOST_CHECK_EQUAL(o.fieldValue(field),val);
        BOOST_CHECK(o.field(field).isSet());
        checkOtherFields(o,field);
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::inc,3)));
        BOOST_CHECK_EQUAL(o.fieldValue(field),val+3);
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::inc,-3)));
        BOOST_CHECK_EQUAL(o.fieldValue(field),val);
        checkOtherFields(o,field);
        o.reset();
    };

    checkScalar(FieldInt8,int8_t(0xaa));
    checkScalar(FieldInt16,int16_t(0xaabb));
    checkScalar(FieldInt32,int32_t(0xaabbcc));
    checkScalar(FieldInt64,int64_t(0xaabbccddee));
    checkScalar(FieldUInt8,uint8_t(0xaa));
    checkScalar(FieldUInt16,uint16_t(0xaabb));
    checkScalar(FieldUInt32,uint32_t(0xaabbcc));
    checkScalar(FieldUInt64,uint64_t(0xaabbccddee));
}

BOOST_AUTO_TEST_CASE(FloatingPoint)
{
    plain::type o;

    auto checkScalar=[&o](const auto& field, auto val)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::set,val)));
        BOOST_TEST(o.fieldValue(field)==val, tt::tolerance(0.0001));
        BOOST_CHECK(o.field(field).isSet());
        checkOtherFields(o,field);
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::inc,3)));
        BOOST_TEST(o.fieldValue(field)==(val+3), tt::tolerance(0.001));
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(field),update::inc,-3)));
        BOOST_TEST(o.fieldValue(field)==val, tt::tolerance(0.0001));
        checkOtherFields(o,field);
        o.reset();
    };

    checkScalar(FieldFloat,float(100.103));
    checkScalar(FieldDouble,double(1000.105));
}

BOOST_AUTO_TEST_CASE(MultipleFields)
{
    plain::type o;

    auto checkScalar=[&o](const auto& field1, auto val1, const auto& field2, auto val2)
    {
        update::applyRequest(&o,update::request(
                                    update::field(field1,update::set,val1),
                                    update::field(field2,update::inc,val2)
                                )
                             );
        BOOST_CHECK_EQUAL(o.fieldValue(field1),val1);
        BOOST_CHECK(o.field(field1).isSet());
        BOOST_CHECK_EQUAL(o.fieldValue(field2),val2);
        BOOST_CHECK(o.field(field2).isSet());
        o.reset();
    };

    checkScalar(FieldInt8,int8_t(0xaa),FieldInt16,int16_t(0xaabb));
    checkScalar(FieldInt32,int32_t(0xaabbcc),FieldInt64,int64_t(0xaabbccddee));
    checkScalar(FieldUInt8,uint8_t(0xaa),FieldUInt16,uint16_t(0xaabb));
    checkScalar(FieldUInt32,uint32_t(0xaabbcc),FieldUInt64,uint64_t(0xaabbccddee));
}

BOOST_AUTO_TEST_CASE(Bytes)
{
    plain::type o;

    std::string val{"Hello world!"};

    std::vector<char> v1;
    std::copy(val.begin(),val.end(),std::back_inserter(v1));
    update::applyRequest(&o,update::request(update::field(FieldBytes,update::set,v1)));
    BOOST_CHECK(o.field(FieldBytes).isSet());
    BOOST_CHECK(o.field(FieldBytes).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();

    common::ByteArray v2;
    v2.append(val);
    update::applyRequest(&o,update::request(update::field(FieldBytes,update::set,v2)));
    BOOST_CHECK(o.field(FieldBytes).isSet());
    BOOST_CHECK(o.field(FieldBytes).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();

    common::VectorOnStack<char> v3;
    v3.append(val);
    update::applyRequest(&o,update::request(update::field(FieldBytes,update::set,v3)));
    BOOST_CHECK(o.field(FieldBytes).isSet());
    BOOST_CHECK(o.field(FieldBytes).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();

    common::pmr::string v4;
    std::copy(val.begin(),val.end(),std::back_inserter(v4));
    update::applyRequest(&o,update::request(update::field(FieldBytes,update::set,v4)));
    BOOST_CHECK(o.field(FieldBytes).isSet());
    BOOST_CHECK(o.field(FieldBytes).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();

    common::pmr::vector<char> v5;
    std::copy(val.begin(),val.end(),std::back_inserter(v5));
    update::applyRequest(&o,update::request(update::field(FieldBytes,update::set,v5)));
    BOOST_CHECK(o.field(FieldBytes).isSet());
    BOOST_CHECK(o.field(FieldBytes).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();
}

#if 0
BOOST_AUTO_TEST_CASE(Single)
{
    init();

    auto s1=initSchema(modelPlain());

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
        auto r2=client->findOne(m1_bool(),makeQuery(u1_bool_f1_idx(),query::where(u1_bool::f1,query::Operator::eq,true),topic1));
        BOOST_REQUIRE(!r2);
        BOOST_CHECK(r2.value().isNull());
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}
#endif

BOOST_AUTO_TEST_SUITE_END()
