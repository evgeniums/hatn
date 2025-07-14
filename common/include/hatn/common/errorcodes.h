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

#include <cstddef>
#include <hatn/common/common.h>

#define HATN_ERROR_CODE(Enum,Code,Msg) Code,

#define HATN_ERROR_STR(Enum,Code,Msg) #Enum"-"#Code,

#define HATN_PLAIN_ERROR_STR(Enum,Code,Msg) #Code,

#define HATN_ERROR_MESSAGE(Enum,Code,Msg) \
    case (static_cast<int>(Enum::Code)): \
    result=Msg; \
    break;

HATN_NAMESPACE_BEGIN

template<class T, size_t N>
constexpr size_t arraySize(T (&)[N]) { return N; }

template <typename T, typename S>
const char* errorString(T code, const S& strings)
{
    if (static_cast<int>(code)==0)
    {
        return "OK";
    }

    auto c=static_cast<size_t>(code);
    if (c<arraySize(strings))
    {
        return strings[c];
    }
    return "";
}

HATN_NAMESPACE_END

#endif // HATNERRORCODES_H
