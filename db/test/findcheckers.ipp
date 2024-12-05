/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/findcheckers.ipp
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

struct eqCheckerT
{
    template <typename ValueGeneratorT, typename ModelT, typename ...Fields>
    void operator ()(
        const ModelT&,
        ValueGeneratorT& valGen,
        const std::vector<size_t>& valIndexes,
        size_t i,
        const HATN_COMMON_NAMESPACE::pmr::vector<DbObject>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        using vType=decltype(valGen(0,true));
        bool skipLast=true;
        if constexpr (std::is_same<vType,plain::MyEnum>::value
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
        const HATN_COMMON_NAMESPACE::pmr::vector<DbObject>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        using vType=decltype(valGen(0,true));
        bool skipLast=true;
        if constexpr (std::is_same<vType,plain::MyEnum>::value
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

struct ltChecker
{
    template <typename ValueGeneratorT, typename ModelT, typename ...Fields>
    void operator ()(
        const ModelT&,
        ValueGeneratorT& valGen,
        const std::vector<size_t>& valIndexes,
        size_t i,
        const HATN_COMMON_NAMESPACE::pmr::vector<DbObject>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        using vType=decltype(valGen(0,true));
        bool skipLast=true;
        if constexpr (std::is_same<vType,plain::MyEnum>::value
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

struct gtChecker
{
    template <typename ValueGeneratorT, typename ModelT, typename ...Fields>
    void operator ()(
        const ModelT&,
        ValueGeneratorT& valGen,
        const std::vector<size_t>& valIndexes,
        size_t i,
        const HATN_COMMON_NAMESPACE::pmr::vector<DbObject>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        using vType=decltype(valGen(0,true));
        bool skipLast=true;
        if constexpr (std::is_same<vType,plain::MyEnum>::value
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

struct inVectorCheckerT
{
    template <typename ValueGeneratorT, typename ModelT, typename ...Fields>
    void operator ()(
        const ModelT&,
        ValueGeneratorT& valGen,
        const std::vector<size_t>& valIndexes,
        size_t i,
        const HATN_COMMON_NAMESPACE::pmr::vector<DbObject>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        using vType=decltype(valGen(0,true));
        if constexpr (std::is_same<vType,plain::MyEnum>::value)
        {            
            BOOST_TEST_CONTEXT(fmt::format("{}",i)){
                size_t resultCount=1;
                BOOST_REQUIRE_EQUAL(resultCount,result.size());
                auto obj=result.at(0).template unit<unitT>();
                BOOST_CHECK(plain::MyEnum::Two==static_cast<vType>(obj->getAtPath(path)));
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
        const HATN_COMMON_NAMESPACE::pmr::vector<DbObject>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        using vType=decltype(valGen(0,true));
        if constexpr (std::is_same<vType,plain::MyEnum>::value)
        {
            BOOST_TEST_CONTEXT(fmt::format("{}",i)){
                size_t resultCount=1;
                BOOST_REQUIRE_EQUAL(resultCount,result.size());
                auto obj=result.at(0).template unit<unitT>();
                BOOST_CHECK(plain::MyEnum::One==static_cast<vType>(obj->getAtPath(path)));
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

                //! @todo Check result values for nin vector
            }
        }
    }
};
constexpr ninVectorCheckerT ninVectorChecker{};

template <bool Nin=false>
struct inIntervalCheckerT
{
    template <typename ValueGeneratorT, typename ModelT, typename ...Fields>
    void operator ()(
        const ModelT&,
        ValueGeneratorT& valGen,
        const std::vector<size_t>& valIndexes,
        size_t i,
        const HATN_COMMON_NAMESPACE::pmr::vector<DbObject>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        using vType=decltype(valGen(0,true));
        if constexpr (std::is_same<vType,plain::MyEnum>::value)
        {
            BOOST_TEST_CONTEXT(fmt::format("[{},{}]",query::intervalTypeToString(fromType),query::intervalTypeToString(toType))){

                if (Nin)
                {
                    // enums not checked yet
                    return;
                }

                if (fromType==query::IntervalType::Closed)
                {
                    size_t resultCount=1;
                    if (fromType==query::IntervalType::First)
                    {
                        resultCount=2;
                    }
                    BOOST_REQUIRE_EQUAL(resultCount,result.size());
                    auto obj=result.at(0).template unit<unitT>();
                    BOOST_CHECK(plain::MyEnum::Two==static_cast<vType>(obj->getAtPath(path)));
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
                            BOOST_CHECK(plain::MyEnum::One==static_cast<vType>(obj0->getAtPath(path)));
                            if (toType!=query::IntervalType::First)
                            {
                                auto obj1=result.at(1).template unit<unitT>();
                                BOOST_CHECK(plain::MyEnum::Two==static_cast<vType>(obj1->getAtPath(path)));
                            }
                        }
                        else
                        {
                            auto obj=result.at(0).template unit<unitT>();
                            BOOST_CHECK(plain::MyEnum::Two==static_cast<vType>(obj->getAtPath(path)));
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

                if (Nin)
                {
                    resultCount=Count-resultCount;
                }
                if (Limit!=0)
                {
                    resultCount=std::min(resultCount,Limit);
                }
                BOOST_REQUIRE_EQUAL(resultCount,result.size());

                if (!Nin)
                {
                    for (size_t j=0;j<resultCount;j++)
                    {
                        auto obj=result.at(j).template unit<unitT>();
                        BOOST_CHECK(valGen(j+startIdx,true)==static_cast<vType>(obj->getAtPath(path)));
                        // BOOST_CHECK_EQUAL(valGen(j+startIdx,true),static_cast<vType>(obj->getAtPath(path)));
                    }
                }
                else
                {
                    //! @todo Check values for nin interval
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

template <bool LastT=false>
struct eqFirstLastCheckerT
{
    template <typename ValueGeneratorT, typename ModelT, typename ...Fields>
    void operator ()(
        const ModelT&,
        ValueGeneratorT& valGen,
        const std::vector<size_t>&,
        size_t,
        const HATN_COMMON_NAMESPACE::pmr::vector<DbObject>& result,
        Fields&&... fields
        ) const
    {
        using unitT=typename std::decay_t<typename ModelT::element_type>::Type;
        auto path=du::path(std::forward<Fields>(fields)...);

        using vType=decltype(valGen(0,true));

        if constexpr (decltype(FirstLastNotFound)::value)
        {
            BOOST_REQUIRE_EQUAL(0,result.size());
        }
        else
        {
            if (LastT)
            {
                BOOST_REQUIRE_EQUAL(1,result.size());
                auto obj=result.at(0).template unit<unitT>();
                BOOST_CHECK(valGen(MaxValIdx,true)==static_cast<vType>(obj->getAtPath(path)));
            }
            else
            {
                BOOST_REQUIRE_EQUAL(1,result.size());
                auto obj=result.at(0).template unit<unitT>();
                BOOST_CHECK(valGen(0,true)==static_cast<vType>(obj->getAtPath(path)));
            }
        }
    }
};
template <bool LastT=false>
constexpr eqFirstLastCheckerT<LastT> eqFirstLastChecker{};

}
