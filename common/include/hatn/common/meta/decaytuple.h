/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/decaytuple.h
  *
  * Defines decayTuple.
  *
  */

/****************************************************************************/

#ifndef HATNDECAYTUPLE_H
#define HATNDECAYTUPLE_H

#include <boost/hana.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename ...Ts>
constexpr auto decayTupleFn(std::tuple<Ts...>) -> std::tuple<std::decay_t<Ts>...>;

template <typename ...Ts>
constexpr auto decayTupleFn(boost::hana::tuple<Ts...>) -> boost::hana::tuple<std::decay_t<Ts>...>;

template <typename T>
using decayTuple = decltype(decayTupleFn(std::declval<std::decay_t<T>>()));

template <typename ...Ts>
using decayStdTupleT = std::tuple<std::decay_t<Ts>...>;

template <typename ...Ts>
using decayHanaTupleT = boost::hana::tuple<std::decay_t<Ts>...>;

HATN_COMMON_NAMESPACE_END

#endif // HATNDECAYTUPLE_H
