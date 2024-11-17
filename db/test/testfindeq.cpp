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
#include <hatn/common/daterange.h>

#include <hatn/test/multithreadfixture.h>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#include "findhandlers.h"

namespace {

struct eqQueryGenT
{
    template <typename PathT,typename ValT>
    auto operator ()(size_t i, PathT&& path, const ValT& val) const
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
        ValueGeneratorT& valGen,
        const std::vector<size_t>& valIndexes,
        size_t i,
        const HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        using vType=decltype(valGen(0,true));
        bool skipLast=true;
        if constexpr (std::is_same<vType,u9::MyEnum>::value
                      ||
                      std::is_same<vType,bool>::value
                      )
        {
            skipLast=false;
        }

        BOOST_TEST_CONTEXT(fmt::format("{}",i)){
            if (skipLast && i==(valIndexes.size()-1))
            {
                BOOST_REQUIRE_EQUAL(0,result.size());
            }
            else
            {
                BOOST_REQUIRE_EQUAL(1,result.size());
                auto obj=result.at(0).template unit<unitT>();
                BOOST_CHECK(valGen(valIndexes[i],true)==static_cast<vType>(obj->getAtPath(path)));
            }
        }
    }
};
constexpr eqCheckerT eqChecker{};

}

BOOST_AUTO_TEST_SUITE(TestFindEq, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(PlainEq)
{
    std::ignore=makeQuery(u9_f10_idx(),query::where(u9::f10,query::Operator::eq,"hi"),topic());
    std::ignore=makeQuery(u9_f10_idx(),query::where(u9::f10,query::Operator::eq,lib::string_view("hi")),topic());
    // std::ignore=makeQuery(u9_f10_idx(),query::where(u9::f10,query::Operator::eq,std::string("hi")),topic());
    std::string val("hi");
    std::ignore=makeQuery(u9_f10_idx(),query::where(u9::f10,query::Operator::eq,val),topic());
    std::ignore=makeQuery(u9_f11_idx(),query::where(u9::f11,query::Operator::eq,u9::MyEnum::One),topic());

    InvokeTestT<eqQueryGenT,eqCheckerT> testEq{eqQueryGen,eqChecker};
    runTest(testEq);
}

BOOST_AUTO_TEST_SUITE_END()
