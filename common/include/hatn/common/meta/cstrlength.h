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

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Get size of const char* at compilation time.
size_t constexpr CStrLength(const char* str) noexcept
{
    return *str ? 1 + hatn::common::CStrLength(str + 1) : 0;
}

HATN_COMMON_NAMESPACE_END

#endif // HATNCSTRLENGTH_H
