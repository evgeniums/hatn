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
         typename extSetterT,
         typename ...FieldsT>
size_t fillDbForFind(
    size_t Count,
    ClientT& client,
    Topic topic,
    const ModelT& model,
    ValueGeneratorT& valGen,
    extSetterT extSetter,
    FieldsT&&... fields
    )
{
    using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
    auto path=du::path(std::forward<FieldsT>(fields)...);

    // fill db with objects
    for (size_t iter=0;iter<extSetter.iterCount();iter++)
    {
        auto valGenProxy=[&valGen,iter](size_t i)
        {
            // generate only once, use already generated for the rest iterations
            if (iter==0)
            {
                return valGen(i,false);
            }
            return valGen(i,true);
        };

        for (size_t i=0;i<Count;i++)
        {
            // create and fill object
            auto obj=makeInitObject<unitT>();
            auto val=valGenProxy(i);
            obj.setAtPath(path,val);
            extSetter.fillObject(obj,iter,i);

            // save object in db
            auto ec=client->create(topic,model,&obj);
            BOOST_REQUIRE(!ec);
        }
    }

    auto totalCount=Count*extSetter.iterCount();

#if 1

    HATN_CTX_INFO("check if all objects are written, using less than Last")
    auto q1=makeQuery(oidIdx(),query::where(object::_id,query::Operator::lte,query::Last),topic);
    q1.setLimit(0);
    auto r1=client->find(model,q1);
    BOOST_REQUIRE(!r1);
    BOOST_REQUIRE_EQUAL(r1.value().size(),totalCount);

    HATN_CTX_INFO("check if all objects are written, using gt than First and reverse order")
    auto q2=makeQuery(oidIdx(),query::where(object::_id,query::Operator::gte,query::First,query::Order::Desc),topic);
    q2.setLimit(0);
    auto r2=client->find(model,q2);
    BOOST_REQUIRE(!r2);
    BOOST_REQUIRE_EQUAL(r2.value().size(),totalCount);

    // check ordering
    for (size_t i=0;i<totalCount;i++)
    {
        auto obj1=r1.value().at(i).template unit<unitT>();
        auto obj2=r2.value().at(totalCount-i-1).template unit<unitT>();
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
        if ((i%Count)<(Count-1))
        {
            // ordering of ASC
            auto obj4=r1.value().at(i+1).template unit<unitT>();
            BOOST_CHECK(obj1->getAtPath(path)<obj4->getAtPath(path));
            BOOST_CHECK(obj1->fieldValue(object::_id)<obj4->fieldValue(object::_id));
        }
        if ((i%Count)>0)
        {
            // ordering of DESC
            auto obj5=r2.value().at(i-1).template unit<unitT>();
            BOOST_CHECK(obj3->getAtPath(path)<obj5->getAtPath(path));
        }
    }
#endif

    return totalCount;
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
    Topic topic,
    ValueGeneratorT& valGen,
    QueryGenT&& queryGen,
    ResultCheckerT&& checker,
    FieldsT&&... fields
    )
{
    HATN_CTX_INFO("invoke db find")

    auto qField=field(std::forward<FieldsT>(fields)...);

    // find objects
    for (size_t i=0;i<valIndexes.size();i++)
    {
        auto idx=valIndexes[i];
        const auto val=valGen(idx,true);
        BOOST_TEST_CONTEXT(fmt::format("i={}, idx={}",i,idx)){
            auto [genQ,_]=queryGen(idx,qField,val,valGen);
            auto q=makeQuery(index,genQ,topic);
            q.setLimit(Limit);

            auto r=client->find(model,q);
            if (r)
            {
                BOOST_TEST_MESSAGE(r.error().message());
            }
            BOOST_REQUIRE(!r);
            checker(model,valGen,valIndexes,i,r.value(),fields...);
        }
    }
}

Topic topic()
{
    return "topic1";
}

template <typename ModelT>
void clearTopic(std::shared_ptr<Client> client, const ModelT& m, size_t count)
{
    HATN_CTX_INFO("clear topic")

    auto q=makeQuery(oidIdx(),query::where(object::_id,query::Operator::gte,query::First),topic());
    q.setLimit(0);
    auto rd=client->deleteMany(m,q);
    BOOST_CHECK(!rd);
    BOOST_CHECK_EQUAL(rd.value(),count);

    HATN_CTX_INFO("check find after clear")

    auto r=client->find(m,q);
    BOOST_REQUIRE(!r);
    BOOST_REQUIRE_EQUAL(r.value().size(),0);

    HATN_CTX_INFO("clear topic done")
}

template <typename QueryGenT, typename CheckerT, typename ExtSetterT=ExtSetter>
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
        BOOST_TEST_MESSAGE("Begin test");

        auto count=fillDbForFind(Count,client,topic(),model,valGen,extSetter,fields...);

        std::vector<size_t> valIndexes=CheckValueIndexes;
        if constexpr (std::is_same<bool,decltype(valGen(0,true))>::value || std::is_same<TestEnum,decltype(valGen(0,true))>::value)
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

        clearTopic(client,model,count);

        BOOST_TEST_MESSAGE("End test");
    }

    template <typename QueryGenT1, typename CheckerT1, typename ExtSetterT1>
    InvokeTestT(
        QueryGenT1&& queryGen,
        CheckerT1&& checker,
        ExtSetterT1&& extSetter
        ) : queryGen(std::forward<QueryGenT1>(queryGen)),
        checker(std::forward<CheckerT1>(checker)),
        extSetter(std::forward<ExtSetterT1>(extSetter))
    {}

    template <typename QueryGenT1, typename CheckerT1>
    InvokeTestT(
        QueryGenT1&& queryGen,
        CheckerT1&& checker
        ) : queryGen(std::forward<QueryGenT1>(queryGen)),
        checker(std::forward<CheckerT1>(checker))
    {}

    QueryGenT queryGen;
    CheckerT checker;
    ExtSetterT extSetter;
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

TestEnum genEnum(size_t i, bool)
{
    if (i==0)
    {
        return TestEnum::One;
    }
    return TestEnum::Two;
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

template <typename InvokerT, typename SkipBoolT=hana::false_>
void invokeTests(InvokerT&& invoker, std::shared_ptr<Client> client, SkipBoolT=SkipBoolT{})
{
    BOOST_TEST_CONTEXT("uint32"){invoker(client,ModelRef,genUInt32,IdxUInt32,FieldUInt32);}

#if 1
    BOOST_TEST_CONTEXT("int8"){invoker(client,ModelRef,genInt8,IdxInt8,FieldInt8);}
    BOOST_TEST_CONTEXT("int16"){invoker(client,ModelRef,genInt16,IdxInt16,FieldInt16);}
    BOOST_TEST_CONTEXT("int32"){invoker(client,ModelRef,genInt32,IdxInt32,FieldInt32);}
    BOOST_TEST_CONTEXT("int64"){invoker(client,ModelRef,genInt64,IdxInt64,FieldInt64);}
    BOOST_TEST_CONTEXT("uint8"){invoker(client,ModelRef,genUInt8,IdxUInt8,FieldUInt8);}
    BOOST_TEST_CONTEXT("uint16"){invoker(client,ModelRef,genUInt16,IdxUInt16,FieldUInt16);}
    BOOST_TEST_CONTEXT("uint64"){invoker(client,ModelRef,genUInt64,IdxUInt64,FieldUInt64);}

    BOOST_TEST_CONTEXT("string"){invoker(client,ModelRef,genString,IdxString,FieldString);}
    BOOST_TEST_CONTEXT("fixed_string"){invoker(client,ModelRef,genString,IdxFixedString,FieldFixedString);}

    BOOST_TEST_CONTEXT("datetime"){invoker(client,ModelRef,genDateTime,IdxDateTime,FieldDateTime);}
    BOOST_TEST_CONTEXT("date"){invoker(client,ModelRef,genDate,IdxDate,FieldDate);}
    BOOST_TEST_CONTEXT("time"){invoker(client,ModelRef,genTime,IdxTime,FieldTime);}
    BOOST_TEST_CONTEXT("daterange"){invoker(client,ModelRef,genDateRange,IdxDateRange,FieldDateRange);}

    BOOST_TEST_CONTEXT("object_id"){invoker(client,ModelRef,genObjectId,IdxObjectId,FieldObjectId);}

    auto cc=Count;
    Count=2;
    if constexpr (!std::decay_t<SkipBoolT>::value)
    {
        BOOST_TEST_CONTEXT("bool"){invoker(client,ModelRef,genBool,IdxBool,FieldBool);}
    }
    BOOST_TEST_CONTEXT("enum"){invoker(client,ModelRef,genEnum,IdxEnum,FieldEnum);}
    Count=cc;
#endif
}

void init()
{
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));

    ModelRegistry::free();
    initRocksDb();
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

template <typename InvokerT, typename SkipBoolT=hana::false_>
void runTest(InvokerT&& invoker, SkipBoolT skipBool=SkipBoolT{})
{
    init();
    registerModels();
    auto s1=initSchema(ModelRef);

    auto handler=[&s1,&invoker,&skipBool](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        invokeTests(invoker,client,skipBool);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

}

HATN_TEST_NAMESPACE_END

#endif // HATNDBTESTFINDS_H
