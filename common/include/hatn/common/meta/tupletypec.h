/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/tupletypec.h
  *
  * Defines tupleToTupleC.
  *
  */

/****************************************************************************/

#ifndef HATNTUPLETYPEC_H
#define HATNTUPLETYPEC_H

#include <boost/hana.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Convert tuple to tuple_t.
template <typename T>
constexpr auto tupleToTupleC(T&& t) noexcept
{
    return boost::hana::transform(std::forward<T>(t),boost::hana::make<boost::hana::type_tag>);
}

/**
 * @brief Convert tuple_t to tuple.
 * @param t hana::tuple_t value.
 * @return hana::tuple of types from t.
 *
 * Only default-constructed types in tuple are supported.
 */
template <typename T>
constexpr auto tupleCToTuple(T&& t)
{
    return boost::hana::transform(std::forward<T>(t),
        [](auto v){
            using type=typename std::decay_t<decltype(v)>::type;
            return type{};
        }
    );
}

/**
 * Convert type of tuple of types to type of tuple of type_c.
 */
template <typename T>
using tupleToTupleCType=decltype(tupleToTupleC(std::declval<T>()));

namespace detail {

template <typename T>
constexpr auto tupleCToTupleType(T tc) noexcept
{
    return boost::hana::unpack(tc,boost::hana::template_<boost::hana::tuple>);
}

template <typename T>
constexpr auto tupleCToStdTupleType(T tc) noexcept
{
    return boost::hana::unpack(tc,boost::hana::template_<std::tuple>);
}

}

template <typename T>
using tupleCToTupleTypeC=decltype(detail::tupleCToTupleType(std::declval<T>()));

template <typename T>
using tupleCToStdTupleTypeC=decltype(detail::tupleCToStdTupleType(std::declval<T>()));

/**
 * Convert type of tuple of type_c to type of hana::tuple of types.
 */
template <typename T>
using tupleCToTupleType=typename tupleCToTupleTypeC<T>::type;

/**
 * Convert type of tuple of type_c to type of hana::tuple of types.
 */
template <typename T>
using tupleCToStdTupleType=typename tupleCToStdTupleTypeC<T>::type;

template <typename TupleT>
struct TupleRefs
{
    constexpr static auto toRefs()
    {
        tupleToTupleCType<TupleT> tc;
        auto rtc=boost::hana::transform(
            tc,
            [](auto&& v)
            {
                using type=typename std::decay_t<decltype(v)>::type;
                return boost::hana::type_c<std::reference_wrapper<const type>>;
            }
            );
        return boost::hana::unpack(rtc,boost::hana::template_<boost::hana::tuple>);
    }

    using typeC=decltype(toRefs());
    using type=typename typeC::type;
};

template <typename RefsTupleT>
struct TupleFromRefs
{
    constexpr static auto fromRefs()
    {
        tupleToTupleCType<RefsTupleT> tc;
        auto rtc=boost::hana::transform(
            tc,
            [](auto&& v)
            {
                using type=typename std::decay_t<decltype(v)>::type;
                return boost::hana::type_c<type>;
            }
        );
        return boost::hana::unpack(rtc,boost::hana::template_<boost::hana::tuple>);
    }

    using typeC=decltype(fromRefs());
    using type=typename typeC::type;
};

HATN_COMMON_NAMESPACE_END

#endif // HATNTUPLETYPEC_H
