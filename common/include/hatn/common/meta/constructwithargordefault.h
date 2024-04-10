/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/constructwithargsordefault/utils.h
  *
  * Defines ConstructWithArgsOrDefault.
  *
  */

/****************************************************************************/

#ifndef HATNCONSTRUCTIWITHARGORDRFAULT_H
#define HATNCONSTRUCTIWITHARGORDRFAULT_H

#include <type_traits>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace detail
{
    template <typename T,typename Enable,typename ...Args>
    struct ConstructWithArgsOrDefault
    {
    };

    template <typename T,typename ...Args>
    struct ConstructWithArgsOrDefault<
        T,
        std::enable_if_t<std::is_constructible<T,Args...>::value>,
        Args...
        >
    {
        constexpr static T f(Args&&... args)
        {
            return T(std::forward<Args>(args)...);
        }
    };

    template <typename T,typename ...Args>
    struct ConstructWithArgsOrDefault<
        T,
        std::enable_if_t<!std::is_constructible<T,Args...>::value>,
        Args...
        >
    {
        constexpr static T f(Args&&...)
        {
            return T();
        }
    };
}

//! Helper to construct value with initializer list or without the list depending on the constructor
template <typename T,typename ...Args>
struct ConstructWithArgsOrDefault
{
    constexpr static T f(Args&&... args)
    {
        return detail::ConstructWithArgsOrDefault<T,void,Args...>::f(std::forward<Args>(args)...);
    }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNCONSTRUCTIWITHARGORDRFAULT_H

