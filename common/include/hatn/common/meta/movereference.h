/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/movereference.h
  *
  * Defines MoveReference.
  *
  */

/****************************************************************************/

#ifndef HATNMOVEREFERENCE_H
#define HATNMOVEREFERENCE_H

#include <type_traits>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Helper for moving references.
template <typename T, typename T1=void>
struct MoveReference
{
    template <typename T2>
    constexpr static auto f(T2&& v) -> decltype(auto)
    {
        return static_cast<T2&&>(v);
    }
};

template <typename T>
struct MoveReference<T,std::enable_if_t<std::is_lvalue_reference<T>::value>>
{
    template <typename T2>
    constexpr static T f(T2&& v)
    {
        return v;
    }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNMOVEREFERENCE_H
