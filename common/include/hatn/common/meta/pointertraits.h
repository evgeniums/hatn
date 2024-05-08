/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file common/meta/pointertraits
  *
  *   Defines PointerTraits.
  *
  */

/****************************************************************************/

#ifndef HATNPOINTERTRAITS_H
#define HATNPOINTERTRAITS_H

#include <memory>
#include <type_traits>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Traits to work with pointer types
template <typename T, typename=void>
struct PointerTraits
{
    static_assert(std::is_pointer<T>::value,"Type must be either raw or smart pointer!");
};

template <typename T>
struct PointerTraits<T,std::enable_if_t<std::is_pointer<T>::value>>
{
    using Type=typename std::remove_pointer<T>::type;
    using Pointer=T;
    static inline Pointer pointer(T val) noexcept
    {
        return val;
    }
};

template <template <typename> class T1, typename T>
struct PointerTraits<T1<T>>
{
    using Type=T;
    using Pointer=T*;
    static inline Pointer pointer(const T1<T>& val) noexcept
    {
        return val.get();
    }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNPOINTERTRAITS_H
