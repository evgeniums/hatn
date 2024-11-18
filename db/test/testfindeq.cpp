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

namespace {
size_t MaxValIdx=250;
// size_t MaxValIdx=250;
size_t Count=MaxValIdx+1;
std::vector<size_t> CheckValueIndexes{10,20,30,150,253};
size_t Limit=0;
size_t VectorSize=5;
size_t VectorStep=5;
}

#include "findhandlers.h"

namespace {

struct eqQueryGenT
{
    template <typename PathT,typename ValT,typename ValGenT>
    auto operator ()(size_t i, PathT&& path, const ValT& val, ValGenT&&) const
    {
        std::ignore=i;
        return std::make_pair(query::where(std::forward<PathT>(path),query::eq,val),0);
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

struct ltQueryGen
{
    template <typename PathT,typename ValT,typename ValGenT>
    auto operator ()(size_t i, PathT&& path, const ValT& val, ValGenT&&) const
    {
        std::ignore=i;
        auto op = m_lte ? query::lte : query::lt;
        return std::make_pair(query::where(std::forward<PathT>(path),op,val),0);
    }

    ltQueryGen(bool lte=false) : m_lte(lte)
    {}

    bool m_lte;
};

struct ltChecker
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

        size_t resultCount=0;
        size_t lte=0;
        if (m_lte)
        {
            lte=1;
        }
        BOOST_TEST_CONTEXT(fmt::format("{}",i)){
            if (skipLast && i==(valIndexes.size()-1))
            {
                resultCount=Count;
            }
            else
            {
                resultCount=valIndexes[i]+lte;
            }
            if (Limit!=0)
            {
                resultCount=std::min(resultCount,Limit);
            }
            BOOST_REQUIRE_EQUAL(resultCount,result.size());
            for (size_t j=0;j<resultCount;j++)
            {
                auto obj=result.at(j).template unit<unitT>();
                BOOST_CHECK(valGen(j,true)==static_cast<vType>(obj->getAtPath(path)));
            }
        }
    }

    ltChecker(bool lte=false) : m_lte(lte)
    {}

    bool m_lte;
};

struct gtQueryGen
{
    template <typename PathT,typename ValT,typename ValGenT>
    auto operator ()(size_t i, PathT&& path, const ValT& val, ValGenT&&) const
    {
        std::ignore=i;
        auto op = m_gte ? query::gte : query::gt;
        return std::make_pair(query::where(std::forward<PathT>(path),op,val),0);
    }

    gtQueryGen(bool gte=false) : m_gte(gte)
    {}

    bool m_gte;
};

struct gtChecker
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

        size_t resultCount=0;
        size_t gte=0;
        if (m_gte)
        {
            gte=1;
        }
        auto startIdx=valIndexes[i]+1-gte;
        BOOST_TEST_CONTEXT(fmt::format("{}",i)){
            if (skipLast && i==(valIndexes.size()-1))
            {
                resultCount=0;
            }
            else
            {
                resultCount=Count-startIdx;
            }
            if (Limit!=0)
            {
                resultCount=std::min(resultCount,Limit);
            }
            BOOST_REQUIRE_EQUAL(resultCount,result.size());
            for (size_t j=0;j<resultCount;j++)
            {
                auto obj=result.at(j).template unit<unitT>();
                BOOST_CHECK(valGen(j+startIdx,true)==static_cast<vType>(obj->getAtPath(path)));
                // BOOST_CHECK_EQUAL(valGen(j+startIdx,true),static_cast<vType>(obj->getAtPath(path)));
            }
        }
    }

    gtChecker(bool gte=false) : m_gte(gte)
    {}

    bool m_gte;
};

template <bool Nin=false>
struct inVectorQueryGenT
{
    template <typename ValGenT, typename VecT>
    void genVector(size_t i, ValGenT&& valGen, VecT v) const
    {
        if (i<200)
        {
            for (size_t j=i;j<i+VectorSize*VectorStep;)
            {
                v->push_back(valGen(j,true));
                j+=VectorStep;
            }
        }
        else
        {
            v->push_back(valGen(i,true));
        }
    }

    template <typename PathT,typename ValT, typename ValGenT>
    auto operator ()(size_t i, PathT&& path, const ValT& val, ValGenT&& valGen) const
    {
        std::ignore=val;
        using type=decltype(valGen(i,true));
        if constexpr (std::is_enum<type>::value)
        {
            std::vector<u9::MyEnum> enums;
            enums.push_back(u9::MyEnum::Two);
            enums.push_back(u9::MyEnum::Three);

            auto v1=std::make_shared<std::vector<int32_t>>();
            query::fromEnumVector(enums,*v1);
            auto op=Nin?query::nin:query::in;
            return std::make_pair(query::where(std::forward<PathT>(path),op,*v1),v1);
        }
        else
        {
            auto vec=std::make_shared<std::vector<type>>();
            genVector(i,valGen,vec);
            auto op=Nin?query::nin:query::in;
            return std::make_pair(query::where(std::forward<PathT>(path),op,*vec),vec);
        }
    }
};
template <bool Nin=false>
constexpr inVectorQueryGenT<Nin> inVectorQueryGen{};

struct inVectorCheckerT
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
        if constexpr (std::is_same<vType,u9::MyEnum>::value)
        {            
            BOOST_TEST_CONTEXT(fmt::format("{}",i)){
                size_t resultCount=1;
                BOOST_REQUIRE_EQUAL(resultCount,result.size());
                auto obj=result.at(0).template unit<unitT>();
                BOOST_CHECK(u9::MyEnum::Two==static_cast<vType>(obj->getAtPath(path)));
            }
        }
        else
        {
            BOOST_TEST_CONTEXT(fmt::format("{}",i)){
                size_t resultCount=0;
                if (i!=(valIndexes.size()-1))
                {
                    resultCount=VectorSize;
                }
                if (Limit!=0)
                {
                    resultCount=std::min(resultCount,Limit);
                }
                BOOST_REQUIRE_EQUAL(resultCount,result.size());

                auto vec=std::make_shared<std::vector<decltype(valGen(i,true))>>();
                inVectorQueryGen<>.genVector(valIndexes[i],valGen,vec);
                const auto& v=*vec;

                for (size_t j=0;j<resultCount;j++)
                {
                    auto obj=result.at(j).template unit<unitT>();
                    BOOST_CHECK(v[j]==static_cast<vType>(obj->getAtPath(path)));
                    // BOOST_CHECK_EQUAL(v[j],static_cast<vType>(obj->getAtPath(path)));
                }
            }
        }
    }
};
constexpr inVectorCheckerT inVectorChecker{};

struct ninVectorCheckerT
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
        if constexpr (std::is_same<vType,u9::MyEnum>::value)
        {
            BOOST_TEST_CONTEXT(fmt::format("{}",i)){
                size_t resultCount=1;
                BOOST_REQUIRE_EQUAL(resultCount,result.size());
                auto obj=result.at(0).template unit<unitT>();
                BOOST_CHECK(u9::MyEnum::One==static_cast<vType>(obj->getAtPath(path)));
            }
        }
        else
        {
            BOOST_TEST_CONTEXT(fmt::format("{}",i)){
                size_t resultCount=Count;
                if (i!=(valIndexes.size()-1))
                {
                    resultCount=Count-VectorSize;
                }
                if (Limit!=0)
                {
                    resultCount=std::min(resultCount,Limit);
                }
                BOOST_REQUIRE_EQUAL(resultCount,result.size());

                // auto vec=std::make_shared<std::vector<decltype(valGen(i,true))>>();
                // inVectorQueryGen.genVector(valIndexes[i],valGen,vec);
                // const auto& v=*vec;
                // for (size_t j=0;j<resultCount;j++)
                // {
                //     auto obj=result.at(j).template unit<unitT>();
                //     BOOST_CHECK(v[j]==static_cast<vType>(obj->getAtPath(path)));
                // }
            }
        }
    }
};
constexpr ninVectorCheckerT ninVectorChecker{};

}

BOOST_AUTO_TEST_SUITE(TestFindEq, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(PlainEq)
{
    std::ignore=makeQuery(u9_f10_idx(),query::where(u9::f10,query::eq,"hi"),topic());
    std::ignore=makeQuery(u9_f10_idx(),query::where(u9::f10,query::eq,lib::string_view("hi")),topic());
    // std::ignore=makeQuery(u9_f10_idx(),query::where(u9::f10,query::eq,std::string("hi")),topic());
    std::string val("hi");
    std::ignore=makeQuery(u9_f10_idx(),query::where(u9::f10,query::eq,val),topic());
    std::ignore=makeQuery(u9_f11_idx(),query::where(u9::f11,query::eq,u9::MyEnum::One),topic());

    InvokeTestT<eqQueryGenT,eqCheckerT> testEq{eqQueryGen,eqChecker};
    runTest(testEq);
}

BOOST_AUTO_TEST_CASE(PlainLt)
{
    InvokeTestT<ltQueryGen,ltChecker> testLt{ltQueryGen(),ltChecker()};
    runTest(testLt);
}

BOOST_AUTO_TEST_CASE(PlainLte)
{
    InvokeTestT<ltQueryGen,ltChecker> testLte{ltQueryGen(true),ltChecker(true)};
    runTest(testLte);
}

BOOST_AUTO_TEST_CASE(PlainGt)
{
    InvokeTestT<gtQueryGen,gtChecker> testGt{gtQueryGen(),gtChecker()};
    runTest(testGt);
}

BOOST_AUTO_TEST_CASE(PlainGte)
{
    InvokeTestT<gtQueryGen,gtChecker> testGte{gtQueryGen(true),gtChecker(true)};
    runTest(testGte);
}

BOOST_AUTO_TEST_CASE(PlainInVector)
{
    // std::ignore=query::where(object::_id,query::in,{1,2,3,4,5});
    std::vector<int> v{1,2,3,4,5};
    std::ignore=query::where(object::_id,query::in,v);

    InvokeTestT<inVectorQueryGenT<>,inVectorCheckerT> testInVector{inVectorQueryGen<>,inVectorChecker};
    runTest(testInVector,hana::true_c);
}

BOOST_AUTO_TEST_CASE(PlainNinVector)
{
    InvokeTestT<inVectorQueryGenT<true>,ninVectorCheckerT> testNinVector{inVectorQueryGen<true>,ninVectorChecker};
    runTest(testNinVector,hana::true_c);
}

BOOST_AUTO_TEST_SUITE_END()
