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

//! Concert tuple to tuple_t
template <typename T>
constexpr auto tupleToTupleC(T&& t) noexcept
{
    return boost::hana::transform(std::forward<T>(t),boost::hana::make<boost::hana::type_tag>);
}
//! Concert tuple_t to tuple
template <typename T>
constexpr auto tupleCToTuple(T&& t) noexcept
{
    return boost::hana::transform(std::forward<T>(t),
        [](auto v){
            using type=typename std::decay_t<decltype(v)>::type;
            return type{};
        }
    );
}

template <typename T>
using tupleToTupleCType=decltype(tupleToTupleC(std::declval<T>()));

/**
 * Convert type of tuple of type_c to type tuple of types.
 * Currently supported only default-constructed types in tuple.
 */
template <typename T>
using tupleCToTupleType=decltype(tupleCToTuple(std::declval<T>()));

HATN_COMMON_NAMESPACE_END

#endif // HATNTUPLETYPEC_H
