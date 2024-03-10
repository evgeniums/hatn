/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/keyrange.h
  *
  *      Key range struct.
  *
  */

/****************************************************************************/

#ifndef HATNKEYRANGE_H
#define HATNKEYRANGE_H

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * @brief The KeyRange struct to be used as a key in containers of ranges.
 */
struct KeyRange
{
    size_t from=0;
    size_t to=0;

    friend inline bool operator ==(const KeyRange& left,const KeyRange& right) noexcept
    {
        return (left.from>=right.from&&left.to<=right.to)
                ||
               (right.from>=left.from&&right.to<=left.to)
              ;
    }
    friend inline bool operator <(const KeyRange& left,const KeyRange& right) noexcept
    {
        return left.to<right.from;
    }
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNKEYRANGE_H
