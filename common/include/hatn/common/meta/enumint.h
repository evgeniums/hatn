/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/enumint.h
  *
  * Defines EnumInt.
  *
  */

/****************************************************************************/

#ifndef HATNENUMINT_H
#define HATNENUMINT_H

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Cast enum to int
template <typename T>
constexpr static int EnumInt(T t) noexcept
{
    return static_cast<int>(t);
}

HATN_COMMON_NAMESPACE_END

#endif // HATNENUMINT_H
