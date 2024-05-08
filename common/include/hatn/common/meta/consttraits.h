/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/consttraits.h
  *
  *   Defines ConstTraits and ReverseCOnst.
  *
  */

/****************************************************************************/

#ifndef HATNCONSTTRAITS_H
#define HATNCONSTTRAITS_H

#include <memory>
#include <type_traits>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Helper to extract const or non const type.
template <typename T, bool isConst>
struct ConstTraits
{};

template <typename T>
struct ConstTraits<T,true>
{
    using type=const T;
};
template <typename T> struct
    ConstTraits<T,false>
{
    using type=T;
};

//! Helper to reverse constness.
template <typename T, typename=void>
struct ReverseConst
{
};

template <typename T>
struct ReverseConst<T,
                std::enable_if_t<std::is_const<typename std::pointer_traits<T>::element_type>::value>>
{
    using type=typename std::remove_const<typename std::pointer_traits<T>::element_type>::type*;
};

template <typename T>
struct ReverseConst<T,
                std::enable_if_t<!std::is_const<typename std::pointer_traits<T>::element_type>::value>>
{
    using type=const typename std::pointer_traits<T>::element_type*;
};

HATN_COMMON_NAMESPACE_END

#endif // HATNCONSTTRAITS_H
