/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/cstrlength.h
  *
  * Defines CStrLength.
  *
  */

/****************************************************************************/

#ifndef HATNCSTRLENGTH_H
#define HATNCSTRLENGTH_H

#include <cstddef>

#include <boost/hana.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Get size of const char* at compilation time.
constexpr inline size_t CStrLength(const char* str) noexcept
{
    return *str ? 1 + CStrLength(str + 1) : 0;
}

constexpr inline size_t CStrLength(std::nullptr_t) noexcept
{
    return 0;
}

//! Check if const char* is empty at compilation time.
constexpr inline bool CStrEmpty(std::nullptr_t) noexcept
{
    return true;
}

//! Check if const char* is empty at compilation time.
constexpr inline bool CStrEmpty(const char* str) noexcept
{
    return *str==0;
}

//! Check if const char* is empty at runtime time.
inline bool StrEmpty(const char* str) noexcept
{
    return str==nullptr || *str==0;
}

HATN_COMMON_NAMESPACE_END

#endif // HATNCSTRLENGTH_H
