/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/valueordefault.h
  *
  *      Wrapper for variable that can have default value.
  *
  */

/****************************************************************************/

#ifndef HATNVALUEORDEFAULT_H
#define HATNVALUEORDEFAULT_H

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Wrapper for variable that can have default value.
template <typename T, T defaultValue> struct ValueOrDefault
{
    T val;
    ValueOrDefault():val(defaultValue)
    {}
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNVALUEORDEFAULT_H
