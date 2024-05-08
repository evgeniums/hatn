/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/types.h
  *
  *      List of standard types of dataunit fields.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITTYPES_H
#define HATNDATAUNITTYPES_H

#include <hatn/dataunit/detail/types.ipp>

HATN_DATAUNIT_NAMESPACE_BEGIN

using namespace types;

//! List of standard types of dataunit fields
#define HDU_DATAUNIT_TYPES() \
    TYPE_BOOL,\
    TYPE_INT8,\
    TYPE_INT16,\
    TYPE_INT32,\
    TYPE_INT64,\
    TYPE_FIXED_INT32,\
    TYPE_FIXED_INT64,\
    TYPE_UINT8,\
    TYPE_UINT16,\
    TYPE_UINT32,\
    TYPE_UINT64,\
    TYPE_FIXED_UINT32,\
    TYPE_FIXED_UINT64,\
    TYPE_FLOAT,\
    TYPE_DOUBLE,\
    TYPE_BYTES, \
    TYPE_STRING

template <int Length> using FixedStringType = detail::FixedString<Length>;

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITTYPES_H
