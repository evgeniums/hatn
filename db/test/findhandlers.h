/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/findhandlers.h
 *
 *     Handlers for tests of find.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTFINDS_H
#define HATNDBTESTFINDS_H

#include <boost/test/unit_test.hpp>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include <hatn/db/object.h>
#include <hatn/db/model.h>
#include <hatn/db/topic.h>
#include <hatn/db/query.h>
#include <hatn/db/indexquery.h>

#include "hatn_test_config.h"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

HATN_TEST_NAMESPACE_BEGIN

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void init()
{
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));

    ModelRegistry::free();
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::free();
    rdb::RocksdbModels::free();
#endif
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

template <typename ClientT,
         typename ModelT,
         typename ValueGeneratorT,
         typename PartitionFieldSetterT,
         typename ...FieldsT>
void fillDbForFind(
            size_t count,
            ClientT& client,
            const Topic& topic,
            const ModelT& model,
            ValueGeneratorT valGen,
            PartitionFieldSetterT partitionSetter,
            FieldsT&&... fields
        )
{
    using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
    auto path=du::path(std::forward<FieldsT>(fields)...);

    // fill db with objects
    for (size_t i=0;i<count;i++)
    {
        // create and fill object
        auto obj=makeInitObject<unitT>();
        auto val=valGen(i);
        obj.setAtPath(path,val);
        partitionSetter(obj,i);

        // save object in db
        auto ec=client->create(topic,model,&obj);
        BOOST_REQUIRE(!ec);
    }

#if 0
    // check if all objects are written, using less than Last
    auto q1=makeQuery(oidIdx(),query::where(object::_id,query::Operator::lt,query::Last),topic);
    q1.setLimit(0);
    auto r1=client->find(model,q1);
    BOOST_REQUIRE(!r1);
    BOOST_REQUIRE_EQUAL(r1.value().size(),count);

    // check if all objects are written, using gt than First
    auto q2=makeQuery(oidIdx(),query::where(object::_id,query::Operator::gt,query::First,query::Order::Desc),topic);
    q2.setLimit(0);
    auto r2=client->find(model,q2);
    BOOST_REQUIRE(!r2);
    BOOST_REQUIRE_EQUAL(r2.value().size(),count);

    // check ordering
    for (size_t i=0;i<count;i++)
    {
        auto obj1=r1.value().at(i).template unit<unitT>();
        auto obj2=r2.value().at(count-i-1).template unit<unitT>();
#if 0
        BOOST_TEST_MESSAGE(fmt::format("Obj1 {}",i));
        BOOST_TEST_MESSAGE(obj1->toString(true));
#endif
        BOOST_CHECK(obj1->fieldValue(object::_id)==obj2->fieldValue(object::_id));

        auto obj3=r2.value().at(i).template unit<unitT>();
#if 0
        BOOST_TEST_MESSAGE(fmt::format("Obj3 {}",i));
        BOOST_TEST_MESSAGE(obj3->toString(true));
#endif
        // exclude multiples of 10 due to possible invalid string comparison
        constexpr bool exclude10=std::is_same<std::string,decltype(valGen(i))>::value;
        if (i<(count-1) && (!exclude10 || (i+1)%10!=0))
        {
            // ordering of ASC
            auto obj4=r1.value().at(i+1).template unit<unitT>();                        
            BOOST_CHECK_LT(obj1->getAtPath(path),obj4->getAtPath(path));
            BOOST_CHECK(obj1->fieldValue(object::_id)<obj4->fieldValue(object::_id));
        }
        if (i>0 && (!exclude10 || i%10!=0))
        {
            // ordering of DESC
            auto obj5=r2.value().at(i-1).template unit<unitT>();
            BOOST_CHECK_LT(obj3->getAtPath(path),obj5->getAtPath(path));
        }
    }
#endif
}

#if 1
template <typename ClientT,
         typename ModelT,
         typename IndexT,
         typename ValueGeneratorT,
         typename QueryGenT,
         typename ResultCheckerT,
         typename ...FieldsT>
void invokeDbFind(
    const std::vector<size_t>& valIndexes,
    ClientT& client,
    const ModelT& model,
    const IndexT& index,
    const Topic& topic,
    ValueGeneratorT&& valGen,
    QueryGenT&& queryGen,
    ResultCheckerT&& checker,
    FieldsT&&... fields
    )
{    
    auto qField=field(fields...);

    // fill db with objects
    for (size_t i=0;i<valIndexes.size();i++)
    {
        auto val=valGen(valIndexes[i]);
        auto q=makeQuery(index,queryGen(i,qField,val),topic);

        auto r=client->find(model,q);
        BOOST_REQUIRE(!r);
        checker(model,valGen,valIndexes,i,r.value(),fields...);
    }
}
#endif

HATN_TEST_NAMESPACE_END

#endif // HATNDBTESTFINDS_H
