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

#include "models9.h"

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

HATN_TEST_NAMESPACE_BEGIN

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

namespace
{

template <typename ClientT,
         typename ModelT,
         typename ValueGeneratorT,
         typename PartitionFieldSetterT,
         typename ...FieldsT>
void fillDbForFind(
    size_t Count,
    ClientT& client,
    const Topic& topic,
    const ModelT& model,
    ValueGeneratorT& valGen,
    PartitionFieldSetterT partitionSetter,
    FieldsT&&... fields
    )
{
    using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
    auto path=du::path(std::forward<FieldsT>(fields)...);

    // fill db with objects
    for (size_t i=0;i<Count;i++)
    {
        // create and fill object
        auto obj=makeInitObject<unitT>();
        auto val=valGen(i,false);
        obj.setAtPath(path,val);
        partitionSetter(obj,i);

        // save object in db
        auto ec=client->create(topic,model,&obj);
        BOOST_REQUIRE(!ec);
    }

#if 1
    // check if all objects are written, using less than Last
    auto q1=makeQuery(oidIdx(),query::where(object::_id,query::Operator::lt,query::Last),topic);
    q1.setLimit(0);
    auto r1=client->find(model,q1);
    BOOST_REQUIRE(!r1);
    BOOST_REQUIRE_EQUAL(r1.value().size(),Count);

    // check if all objects are written, using gt than First
    auto q2=makeQuery(oidIdx(),query::where(object::_id,query::Operator::gt,query::First,query::Order::Desc),topic);
    q2.setLimit(0);
    auto r2=client->find(model,q2);
    BOOST_REQUIRE(!r2);
    BOOST_REQUIRE_EQUAL(r2.value().size(),Count);

    // check ordering
    for (size_t i=0;i<Count;i++)
    {
        auto obj1=r1.value().at(i).template unit<unitT>();
        auto obj2=r2.value().at(Count-i-1).template unit<unitT>();
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
        if (i<(Count-1))
        {
            // ordering of ASC
            auto obj4=r1.value().at(i+1).template unit<unitT>();
            BOOST_CHECK(obj1->getAtPath(path)<obj4->getAtPath(path));
            BOOST_CHECK(obj1->fieldValue(object::_id)<obj4->fieldValue(object::_id));
        }
        if (i>0)
        {
            // ordering of DESC
            auto obj5=r2.value().at(i-1).template unit<unitT>();
            BOOST_CHECK(obj3->getAtPath(path)<obj5->getAtPath(path));
        }
    }
#endif
}

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
    ValueGeneratorT& valGen,
    QueryGenT&& queryGen,
    ResultCheckerT&& checker,
    FieldsT&&... fields
    )
{
    auto qField=field(fields...);

    // fill db with objects
    for (size_t i=0;i<valIndexes.size();i++)
    {
        const auto val=valGen(valIndexes[i],true);
        auto q=makeQuery(index,queryGen(i,qField,val),topic);
        q.setLimit(Limit);

        auto r=client->find(model,q);
        BOOST_REQUIRE(!r);
        checker(model,valGen,valIndexes,i,r.value(),fields...);
    }
}

struct noPartitionT
{
    template <typename T>
    void operator()(T&&, size_t) const noexcept
    {
    }
};
constexpr noPartitionT noPartition{};

Topic topic()
{
    return "topic1";
}

template <typename ModelT>
void clearTopic(std::shared_ptr<Client> client, const ModelT& m)
{
    auto q=makeQuery(oidIdx(),query::where(object::_id,query::Operator::gt,query::First),topic());
    q.setLimit(0);
    auto ec=client->deleteMany(m,q);
    BOOST_CHECK(!ec);
    auto r=client->find(m,q);
    BOOST_REQUIRE(!r);
    BOOST_REQUIRE_EQUAL(r.value().size(),0);
}

template <typename QueryGenT, typename CheckerT, typename PartitionFnT=noPartitionT>
struct InvokeTestT
{
    template <typename ModelT, typename ValGenT, typename IndexT, typename ...FieldsT>
    void operator()(
        std::shared_ptr<Client> client,
        const ModelT& model,
        ValGenT& valGen,
        const IndexT& idx,
        const FieldsT&... fields
        )
    {
        fillDbForFind(Count,client,topic(),model,valGen,partitionFn,fields...);

        std::vector<size_t> valIndexes=CheckValueIndexes;
        if constexpr (std::is_same<bool,decltype(valGen(0,true))>::value || std::is_same<u9::MyEnum,decltype(valGen(0,true))>::value)
        {
            valIndexes.clear();
            valIndexes.push_back(0);
            valIndexes.push_back(1);
        }
        invokeDbFind(valIndexes,
                     client,
                     model,
                     idx,
                     topic(),
                     valGen,
                     queryGen,
                     checker,
                     fields...);
        clearTopic(client,model);
    }

    template <typename QueryGenT1, typename CheckerT1, typename PartitionFnT1>
    InvokeTestT(
        QueryGenT1&& queryGen,
        CheckerT1&& checker,
        PartitionFnT1&& partitionFn
        ) : queryGen(std::forward<QueryGenT1>(queryGen)),
        checker(std::forward<CheckerT1>(checker)),
        partitionFn(std::forward<PartitionFnT>(partitionFn))
    {}

    template <typename QueryGenT1, typename CheckerT1>
    InvokeTestT(
        QueryGenT1&& queryGen,
        CheckerT1&& checker
        ) : queryGen(std::forward<QueryGenT1>(queryGen)),
        checker(std::forward<CheckerT1>(checker)),
        partitionFn(noPartition)
    {}

    QueryGenT queryGen;
    CheckerT checker;
    PartitionFnT partitionFn;
};

auto genInt8(size_t i, bool)
{
    return static_cast<int8_t>(i-127);
}

auto genInt16(size_t i, bool)
{
    return static_cast<int16_t>(i-127);
}

auto genInt32(size_t i, bool)
{
    return static_cast<int32_t>(i-127);
}

auto genInt64(size_t i, bool)
{
    return static_cast<int64_t>(i-127);
}

auto genUInt8(size_t i, bool)
{
    return static_cast<uint8_t>(i);
}

auto genUInt16(size_t i, bool)
{
    return static_cast<uint16_t>(i);
}

auto genUInt32(size_t i, bool)
{
    return static_cast<uint32_t>(i);
}

auto genUInt64(size_t i, bool)
{
    return static_cast<uint64_t>(i);
}

std::string genString(size_t i, bool)
{
    return fmt::format("value_{:04d}",i);
}

bool genBool(size_t i, bool)
{
    if (i==0)
    {
        return false;
    }
    return true;
}

u9::MyEnum genEnum(size_t i, bool)
{
    if (i==0)
    {
        return u9::MyEnum::One;
    }
    return u9::MyEnum::Two;
}

template <typename T>
struct genAndKeep
{
    auto operator ()(size_t i, bool forCheck)
    {
        auto val=m_gen(i);
        if (!forCheck)
        {
            m_vals.resize(std::max(i+1,m_vals.size()));
            m_vals[i]=val;
            return val;
        }
        if (i<m_vals.size())
        {
            return m_vals[i];
        }
        return val;
    }

    std::vector<T> m_vals;
    std::function<T (size_t)> m_gen;
};

struct genDateTimeT : public genAndKeep<common::DateTime>
{
    genDateTimeT()
    {
        m_gen=[](size_t i)
        {
            auto dt=common::DateTime::currentUtc();
            dt.addMinutes(static_cast<int>(i));
            return dt;
        };
    }
};
genDateTimeT genDateTime{};

struct genDateT : public genAndKeep<common::Date>
{
    genDateT()
    {
        m_gen=[](size_t i)
        {
            auto dt=common::Date::currentUtc();
            dt.addDays(static_cast<int>(i));
            return dt;
        };
    }
};
genDateT genDate{};

struct genTimeT : public genAndKeep<common::Time>
{
    genTimeT()
    {
        m_gen=[](size_t i)
        {
            common::Time time{1,1,1};
            time.addSeconds(static_cast<int>(i));
            return time;
        };
    }
};
genTimeT genTime{};

struct genDateRangeT : public genAndKeep<common::DateRange>
{
    genDateRangeT()
    {
        m_gen=[](size_t i)
        {
            auto dt=common::Date::currentUtc();
            dt.addDays(static_cast<int>(i));
            common::DateRange range{dt,common::DateRange::Type::Day};
            return range;
        };
    }
};
genDateRangeT genDateRange{};

struct genObjectIdT : public genAndKeep<ObjectId>
{
    genObjectIdT()
    {
        m_gen=[](size_t)
        {
            return ObjectId::generateId();
        };
    }
};
genObjectIdT genObjectId{};

template <typename InvokerT>
void invokeTests(InvokerT&& invoker, std::shared_ptr<Client> client)
{
    BOOST_TEST_CONTEXT("int8"){invoker(client,m9(),genInt8,u9_f2_idx(),u9::f2);}
    BOOST_TEST_CONTEXT("int16"){invoker(client,m9(),genInt16,u9_f3_idx(),u9::f3);}
    BOOST_TEST_CONTEXT("int32"){invoker(client,m9(),genInt32,u9_f4_idx(),u9::f4);}
    BOOST_TEST_CONTEXT("int64"){invoker(client,m9(),genInt64,u9_f5_idx(),u9::f5);}
    BOOST_TEST_CONTEXT("uint8"){invoker(client,m9(),genUInt8,u9_f6_idx(),u9::f6);}
    BOOST_TEST_CONTEXT("uint16"){invoker(client,m9(),genUInt16,u9_f7_idx(),u9::f7);}
    BOOST_TEST_CONTEXT("uint32"){invoker(client,m9(),genUInt32,u9_f8_idx(),u9::f8);}
    BOOST_TEST_CONTEXT("uint64"){invoker(client,m9(),genUInt64,u9_f9_idx(),u9::f9);}

    BOOST_TEST_CONTEXT("string"){invoker(client,m9(),genString,u9_f10_idx(),u9::f10);}
    BOOST_TEST_CONTEXT("fixed_string"){invoker(client,m9(),genString,u9_f12_idx(),u9::f12);}

    BOOST_TEST_CONTEXT("datetime"){invoker(client,m9(),genDateTime,u9_f13_idx(),u9::f13);}
    BOOST_TEST_CONTEXT("date"){invoker(client,m9(),genDate,u9_f14_idx(),u9::f14);}
    BOOST_TEST_CONTEXT("time"){invoker(client,m9(),genTime,u9_f15_idx(),u9::f15);}
    BOOST_TEST_CONTEXT("daterange"){invoker(client,m9(),genDateRange,u9_f17_idx(),u9::f17);}

    BOOST_TEST_CONTEXT("object_id"){invoker(client,m9(),genObjectId,u9_f16_idx(),u9::f16);}

    auto cc=Count;
    Count=2;
    BOOST_TEST_CONTEXT("bool"){invoker(client,m9(),genBool,u9_f1_idx(),u9::f1);}
    BOOST_TEST_CONTEXT("enum"){invoker(client,m9(),genEnum,u9_f11_idx(),u9::f11);}
    Count=cc;
}

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

template <typename InvokerT>
void runTest(InvokerT&& invoker)
{
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));
    auto ctx=HATN_COMMON_NAMESPACE::makeTaskContext<HATN_LOGCONTEXT_NAMESPACE::ContextWrapper>();
    ctx->beforeThreadProcessing();

    init();
    registerModels9();
    auto s1=initSchema(m9());

    auto handler=[&s1,&invoker](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        invokeTests(invoker,client);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");

    ctx->afterThreadProcessing();
}

}

HATN_TEST_NAMESPACE_END

#endif // HATNDBTESTFINDS_H
