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
constexpr size_t CStrLength(const char* str) noexcept
{
    return *str ? 1 + hatn::common::CStrLength(str + 1) : 0;
}

//! Check if const char* is empty.
template <typename T>
constexpr bool CStrEmpty(T str) noexcept
{
    return boost::hana::eval_if(
        std::is_same<T,std::nullptr_t>{},
        []()
        {
            return true;
        },
        [&](auto _)
        {
            return CStrLength(_(str))==0;
        }
    );
}

HATN_COMMON_NAMESPACE_END

#endif // HATNCSTRLENGTH_H
