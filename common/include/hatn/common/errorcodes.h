/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/errorcodes.h
  *
  *     Contains error codes macros.
  *
  */

/****************************************************************************/

#ifndef HATNERRORCODES_H
#define HATNERRORCODES_H

#include <hatn/common/common.h>

#define HATN_ERROR_CODE(Enum,Code,Msg) Code,

#define HATN_ERROR_STR(Enum,Code,Msg) #Code,

#define HATN_ERROR_MESSAGE(Enum,Code,Msg) \
    case (static_cast<int>(Enum::Code)): \
    result=Msg; \
    break; \

HATN_NAMESPACE_BEGIN

template <typename T, typename S>
const char* errorString(T code, const S& strings)
{
    return strings[static_cast<int>(code)];
}

HATN_NAMESPACE_END

#endif // HATNERRORCODES_H
