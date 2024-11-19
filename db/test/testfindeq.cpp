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
size_t MaxValIdx=230;
// size_t MaxValIdx=250;
size_t Count=MaxValIdx+1;
std::vector<size_t> CheckValueIndexes{10,20,30,150,233};
size_t Limit=0;
size_t VectorSize=5;
size_t VectorStep=5;
size_t IntervalWidth=7;
}

#include "findhandlers.h"

namespace {

template <bool NeqT=false>
struct eqQueryGenT
{
    template <typename PathT,typename ValT,typename ValGenT>
    auto operator ()(size_t i, PathT&& path, const ValT& val, ValGenT&&) const
    {
        std::ignore=i;
        auto op=NeqT?query::neq:query::eq;
        return std::make_pair(query::where(std::forward<PathT>(path),op,val),0);
    }
};
template <bool NeqT=false>
constexpr eqQueryGenT<NeqT> eqQueryGen{};

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

struct neqCheckerT
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
                BOOST_REQUIRE_EQUAL(Count,result.size());
            }
            else
            {
                BOOST_REQUIRE_EQUAL(Count-1,result.size());
                auto skipIdx=valIndexes[i];
                for (size_t j=0;j<Count;j++)
                {
                    if (j==result.size())
                    {
                        break;
                    }
                    auto obj=result.at(j).template unit<unitT>();
                    if (j>=skipIdx)
                    {
                        BOOST_CHECK(valGen(j+1,true)==static_cast<vType>(obj->getAtPath(path)));
                    }
                    else
                    {
                        BOOST_CHECK(valGen(j,true)==static_cast<vType>(obj->getAtPath(path)));
                    }
                }
            }
        }
    }
};
constexpr neqCheckerT neqChecker{};

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

                //! @todo check result
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

template <bool Nin=false>
struct inIntervalQueryGenT
{
    template <typename PathT, typename ValT, typename ValGenT>
    auto operator ()(size_t i, PathT&& path, const ValT& val, ValGenT&& valGen) const
    {
        std::ignore=val;
        using vType=decltype(valGen(i,true));
        using type=typename query::ValueTypeTraits<vType>::type;
        if constexpr (std::is_enum<vType>::value)
        {
            query::Interval<type> v(u9::MyEnum::Two,fromType,u9::MyEnum::Three,toType);
            auto op=Nin?query::nin:query::in;
            return std::make_pair(query::where(std::forward<PathT>(path),op,v),0);
        }
        else
        {
            auto p=std::make_shared<std::pair<vType,vType>>(valGen(i,true),valGen(i+IntervalWidth-1,true));
            query::Interval<type> v(p->first,fromType,p->second,toType);
            auto op=Nin?query::nin:query::in;
            return std::make_pair(query::where(std::forward<PathT>(path),op,v),p);
        }
    }

    inIntervalQueryGenT(
        query::IntervalType fromType,
        query::IntervalType toType
    ) : fromType(fromType),
        toType(toType)
    {}

    query::IntervalType fromType;
    query::IntervalType toType;
};

struct inIntervalCheckerT
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
            BOOST_TEST_CONTEXT(fmt::format("[{},{}]",query::intervalTypeToString(fromType),query::intervalTypeToString(toType))){
                if (fromType==query::IntervalType::Closed)
                {
                    size_t resultCount=1;
                    if (fromType==query::IntervalType::First)
                    {
                        resultCount=2;
                    }
                    BOOST_REQUIRE_EQUAL(resultCount,result.size());
                    auto obj=result.at(0).template unit<unitT>();
                    BOOST_CHECK(u9::MyEnum::Two==static_cast<vType>(obj->getAtPath(path)));
                }
                else
                {
                    size_t resultCount=0;
                    if (fromType==query::IntervalType::First)
                    {
                        if (toType==query::IntervalType::First)
                        {
                            resultCount=1;
                        }
                        else
                        {
                            resultCount=2;
                        }
                    }
                    else if (fromType==query::IntervalType::Last)
                    {
                        resultCount=1;
                    }
                    BOOST_REQUIRE_EQUAL(resultCount,result.size());
                    if (resultCount>0)
                    {
                        if (fromType==query::IntervalType::First)
                        {
                            auto obj0=result.at(0).template unit<unitT>();
                            BOOST_CHECK(u9::MyEnum::One==static_cast<vType>(obj0->getAtPath(path)));
                            if (toType!=query::IntervalType::First)
                            {
                                auto obj1=result.at(1).template unit<unitT>();
                                BOOST_CHECK(u9::MyEnum::Two==static_cast<vType>(obj1->getAtPath(path)));
                            }
                        }
                        else
                        {
                            auto obj=result.at(0).template unit<unitT>();
                            BOOST_CHECK(u9::MyEnum::Two==static_cast<vType>(obj->getAtPath(path)));
                        }
                    }
                }
            }
        }
        else
        {
            BOOST_TEST_CONTEXT(fmt::format("[{},{}]",query::intervalTypeToString(fromType),query::intervalTypeToString(toType))){
                size_t resultCount=0;
                size_t startIdx=0;
                if (
                    fromType==query::IntervalType::First
                    &&
                    toType==query::IntervalType::Last
                    )
                {
                    resultCount=Count;
                }
                else if (
                        fromType==query::IntervalType::First
                        &&
                        toType==query::IntervalType::First
                    )
                {
                    resultCount=1;
                }
                else if (
                    fromType==query::IntervalType::Last
                    &&
                    toType==query::IntervalType::Last
                    )
                {
                    resultCount=1;
                    startIdx=MaxValIdx;
                }
                else if (i<(valIndexes.size()-1))
                {
                    if (
                        fromType==query::IntervalType::Closed
                        &&
                        toType==query::IntervalType::Closed
                       )
                    {
                        resultCount=IntervalWidth;
                        startIdx=valIndexes[i];
                    }
                    else if (
                        fromType==query::IntervalType::Open
                        &&
                        toType==query::IntervalType::Open
                        )
                    {
                        resultCount=IntervalWidth-2;
                        startIdx=valIndexes[i]+1;
                    }
                    else if (
                            fromType==query::IntervalType::First
                            &&
                            toType==query::IntervalType::Open
                        )
                    {
                        resultCount=valIndexes[i]+IntervalWidth-1;
                        startIdx=0;
                    }
                    else if (
                        fromType==query::IntervalType::First
                        &&
                        toType==query::IntervalType::Closed
                        )
                    {
                        resultCount=valIndexes[i]+IntervalWidth;
                        startIdx=0;
                    }
                    else if (
                        fromType==query::IntervalType::Open
                        &&
                        toType==query::IntervalType::Last
                        )
                    {
                        resultCount=Count-valIndexes[i]-1;
                        startIdx=valIndexes[i]+1;
                    }
                    else if (
                        fromType==query::IntervalType::Closed
                        &&
                        toType==query::IntervalType::Last
                        )
                    {
                        resultCount=Count-valIndexes[i];
                        startIdx=valIndexes[i];
                    }
                    else
                    {
                        resultCount=IntervalWidth-1;
                        if (fromType==query::IntervalType::Closed)
                        {
                            startIdx=valIndexes[i];
                        }
                        else
                        {
                            startIdx=valIndexes[i]+1;
                        }
                    }
                }
                else
                {
                    resultCount=0;
                    if (fromType==query::IntervalType::First)
                    {
                        resultCount=Count;
                    }
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
    }

    inIntervalCheckerT(
        query::IntervalType fromType,
        query::IntervalType toType
        ) : fromType(fromType),
        toType(toType)
    {}

    query::IntervalType fromType;
    query::IntervalType toType;
};

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

    InvokeTestT<eqQueryGenT<>,eqCheckerT> testEq{eqQueryGen<>,eqChecker};
    runTest(testEq);
}

BOOST_AUTO_TEST_CASE(PlainNeq)
{
    InvokeTestT<eqQueryGenT<true>,neqCheckerT> testNeq{eqQueryGen<true>,neqChecker};
    runTest(testNeq);
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

BOOST_AUTO_TEST_CASE(PlainInInterval)
{
    query::IntervalType fromType{query::IntervalType::Open};
    query::IntervalType toType{query::IntervalType::Open};

    std::ignore=query::Interval<query::String>{"1000",fromType,"9000",toType};
    // std::ignore=query::Interval<query::String>{std::string("1000"),fromType,"9000",toType};
    // std::ignore=query::Interval<query::String>{"1000",fromType,std::string("9000"),toType};
    std::string from("1000");
    std::string to("9000");
    std::ignore=query::Interval<query::String>{from,fromType,to,toType};

    auto run=[&]()
    {
        BOOST_TEST_MESSAGE(fmt::format("[{},{}]",query::intervalTypeToString(fromType),query::intervalTypeToString(toType)));

        InvokeTestT<inIntervalQueryGenT<>,inIntervalCheckerT> testInInterval{inIntervalQueryGenT<>{
                fromType,
                toType
            },
            inIntervalCheckerT{
                fromType,
                toType
            }
        };
        runTest(testInInterval,hana::true_c);
    };

    run();

    fromType=query::IntervalType{query::IntervalType::Closed};
    toType=query::IntervalType{query::IntervalType::Closed};
    run();

    fromType=query::IntervalType{query::IntervalType::Open};
    toType=query::IntervalType{query::IntervalType::Closed};
    run();

    fromType=query::IntervalType{query::IntervalType::Closed};
    toType=query::IntervalType{query::IntervalType::Open};
    run();

    fromType=query::IntervalType{query::IntervalType::First};
    toType=query::IntervalType{query::IntervalType::Open};
    run();

    fromType=query::IntervalType{query::IntervalType::First};
    toType=query::IntervalType{query::IntervalType::Closed};
    run();

    fromType=query::IntervalType{query::IntervalType::Open};
    toType=query::IntervalType{query::IntervalType::Last};
    run();

    fromType=query::IntervalType{query::IntervalType::Closed};
    toType=query::IntervalType{query::IntervalType::Last};
    run();

    fromType=query::IntervalType{query::IntervalType::First};
    toType=query::IntervalType{query::IntervalType::Last};
    run();

    fromType=query::IntervalType{query::IntervalType::First};
    toType=query::IntervalType{query::IntervalType::First};
    run();

    fromType=query::IntervalType{query::IntervalType::Last};
    toType=query::IntervalType{query::IntervalType::Last};
    run();
}

BOOST_AUTO_TEST_SUITE_END()
