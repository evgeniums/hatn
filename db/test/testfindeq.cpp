/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testfindeq.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

#include <hatn/test/multithreadfixture.h>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#include "models9.h"
#include "findhandlers.h"

namespace {

constexpr const size_t count=250;

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

struct eqQueryGenT
{
    template <typename PathT,typename ValT>
    auto operator ()(size_t i, PathT&& path, ValT&& val) const
    {
        std::ignore=i;
        return query::where(std::forward<PathT>(path),query::Operator::eq,val);
    }
};
constexpr eqQueryGenT eqQueryGen{};

struct eqCheckerT
{
    template <typename ValueGeneratorT, typename ModelT, typename ...Fields>
    void operator ()(
        const ModelT&,
        const ValueGeneratorT& valGen,
        const std::vector<size_t>& valIndexes,
        size_t i,
        const HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        if (i==(valIndexes.size()-1))
        {
            BOOST_REQUIRE_EQUAL(0,result.size());
        }
        else
        {
            BOOST_REQUIRE_EQUAL(1,result.size());
            auto obj=result.at(0).template unit<unitT>();
            BOOST_CHECK(valGen(valIndexes[i])==obj->getAtPath(path));
        }
    }
};
constexpr eqCheckerT eqChecker{};

template <typename ModelT>
void clearTopic(std::shared_ptr<Client> client, const ModelT& m)
{
    auto q=makeQuery(oidIdx(),query::where(object::_id,query::Operator::gt,query::First),topic());
    q.setLimit(0);
    auto ec=client->deleteMany(m,q);
    BOOST_CHECK(!ec);
    // auto r=client->find(m,q);
    // BOOST_REQUIRE(!r);
    // BOOST_REQUIRE_EQUAL(r.value().size(),0);
}

template <typename ModelT, typename ValGenT, typename IndexT, typename ...FieldsT>
void testEq(std::shared_ptr<Client> client,
            const ModelT& model,
            const ValGenT& valGen,
            const IndexT& idx,
            const FieldsT&... fields
            )
{
    fillDbForFind(count,client,topic(),model,valGen,noPartition,fields...);

    std::vector<size_t> valIndexes{10,20,30,50,253};
    invokeDbFind(valIndexes,
                 client,
                 model,
                 idx,
                 topic(),
                 valGen,
                 eqQueryGen,
                 eqChecker,
                 fields...);
    clearTopic(client,model);
}

auto genInt8(size_t i)
{
    return static_cast<int8_t>(i-127);
}

auto genInt16(size_t i)
{
    return static_cast<int16_t>(i-127);
}

auto genInt32(size_t i)
{
    return static_cast<int32_t>(i-127);
}

auto genInt64(size_t i)
{
    return static_cast<int64_t>(i-127);
}

auto genUInt8(size_t i)
{
    return static_cast<uint8_t>(i);
}

auto genUInt16(size_t i)
{
    return static_cast<uint16_t>(i);
}

auto genUInt32(size_t i)
{
    return static_cast<uint32_t>(i);
}

auto genUInt64(size_t i)
{
    return static_cast<uint64_t>(i);
}

}

BOOST_AUTO_TEST_SUITE(TestFindEq, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(CheckEqInt)
{
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));
    auto ctx=HATN_COMMON_NAMESPACE::makeTaskContext<HATN_LOGCONTEXT_NAMESPACE::ContextWrapper>();
    ctx->beforeThreadProcessing();

    init();
    registerModels9();
    auto s1=initSchema(m9());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        testEq(client,m9(),genInt8,u9_f2_idx(),u9::f2);
        testEq(client,m9(),genInt16,u9_f3_idx(),u9::f3);
        testEq(client,m9(),genInt32,u9_f4_idx(),u9::f4);
        testEq(client,m9(),genInt64,u9_f5_idx(),u9::f5);
        testEq(client,m9(),genUInt8,u9_f6_idx(),u9::f6);
        testEq(client,m9(),genUInt16,u9_f7_idx(),u9::f7);
        testEq(client,m9(),genUInt32,u9_f8_idx(),u9::f8);
        testEq(client,m9(),genUInt64,u9_f9_idx(),u9::f9);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");

    ctx->afterThreadProcessing();
}

BOOST_AUTO_TEST_SUITE_END()
