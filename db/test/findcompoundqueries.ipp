/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/findcompundqueries.ipp
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

struct compoundExtSetterT
{
    template <typename T>
    void fillObject(T& obj, size_t iter, size_t) const noexcept
    {
        auto str=fmt::format("comp_{:02d}",iter);
        obj.field(comp::ext1).set(str);
    }

    static const size_t iterCount() noexcept
    {
        return 5;
    }
};
using ExtSetter=compoundExtSetterT;
std::string queryExtVal{"comp_03"};
constexpr auto FirstLastNotFound=hana::true_c;

template <bool NeqT=false>
struct eqQueryGenT
{
    template <typename PathT,typename ValT,typename ValGenT>
    auto operator ()(size_t i, PathT&& path, const ValT& val, ValGenT&&) const
    {
        std::ignore=i;
        auto op=NeqT?query::neq:query::eq;
        return std::make_pair(query::where(std::forward<PathT>(path),op,val)
                                  .and_(comp::ext1,query::eq,queryExtVal)
                              ,0);
    }
};
template <bool NeqT=false>
constexpr eqQueryGenT<NeqT> eqQueryGen{};

struct ltQueryGen
{
    template <typename PathT,typename ValT,typename ValGenT>
    auto operator ()(size_t i, PathT&& path, const ValT& val, ValGenT&&) const
    {
        std::ignore=i;
        auto op = m_lte ? query::lte : query::lt;
        return std::make_pair(query::where(std::forward<PathT>(path),op,val)
                              .and_(comp::ext1,query::eq,queryExtVal)
                              ,0);
    }

    ltQueryGen(bool lte=false) : m_lte(lte)
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
        return std::make_pair(query::where(std::forward<PathT>(path),op,val)
                              .and_(comp::ext1,query::eq,queryExtVal)
                              ,0);
    }

    gtQueryGen(bool gte=false) : m_gte(gte)
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
            std::vector<plain::MyEnum> enums;
            enums.push_back(plain::MyEnum::Two);
            enums.push_back(plain::MyEnum::Three);

            auto v1=std::make_shared<std::vector<int32_t>>();
            query::fromEnumVector(enums,*v1);
            auto op=Nin?query::nin:query::in;
            return std::make_pair(query::where(std::forward<PathT>(path),op,*v1)
                                  .and_(comp::ext1,query::eq,queryExtVal)
                                  ,v1);
        }
        else
        {
            auto vec=std::make_shared<std::vector<type>>();
            genVector(i,valGen,vec);
            auto op=Nin?query::nin:query::in;
            return std::make_pair(query::where(std::forward<PathT>(path),op,*vec)
                                  .and_(comp::ext1,query::eq,queryExtVal)
                                  ,vec);
        }
    }
};
template <bool Nin=false>
constexpr inVectorQueryGenT<Nin> inVectorQueryGen{};

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
            query::Interval<type> v(plain::MyEnum::Two,fromType,plain::MyEnum::Three,toType);
            auto op=Nin?query::nin:query::in;
            return std::make_pair(query::where(std::forward<PathT>(path),op,v)
                                  .and_(comp::ext1,query::eq,queryExtVal)
                                  ,0);
        }
        else
        {
            auto p=std::make_shared<std::pair<vType,vType>>(valGen(i,true),valGen(i+IntervalWidth-1,true));
            query::Interval<type> v(p->first,fromType,p->second,toType);
            auto op=Nin?query::nin:query::in;
            return std::make_pair(query::where(std::forward<PathT>(path),op,v)
                                  .and_(comp::ext1,query::eq,queryExtVal)
                                  ,p);
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

template <bool LastT=false>
struct eqFirstLastQueryGenT
{
    template <typename PathT,typename ValT,typename ValGenT>
    auto operator ()(size_t i, PathT&& path, const ValT& val, ValGenT&&) const
    {
        std::ignore=i;
        std::ignore=val;

        if constexpr (LastT)
        {
            return std::make_pair(query::where(std::forward<PathT>(path),query::eq,query::Last)
                                  .and_(comp::ext1,query::eq,queryExtVal)
                                  ,0);
        }
        else
        {
            return std::make_pair(query::where(std::forward<PathT>(path),query::eq,query::First)
                                  .and_(comp::ext1,query::eq,queryExtVal)
                                  ,0);
        }
    }
};
template <bool LastT=false>
constexpr eqFirstLastQueryGenT<LastT> eqFirstLastQueryGen{};

}
