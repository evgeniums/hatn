/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/hasmethod.h
  *
  */

/****************************************************************************/

#ifndef HATNHASMETHOD_H
#define HATNHASMETHOD_H

#include <boost/hana.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

HATN_COMMON_NAMESPACE_END

#define HATN_PREPARE_HAS_METHOD(Method) \
    template <typename Type, typename ...Args> \
    constexpr auto has_##Method(Args&& ...args) \
    { \
            return boost::hana::is_valid([](auto v, auto&& ...arg) -> decltype((void)boost::hana::traits::declval(v). Method (std::forward<decltype(arg)>(arg)...)){})(boost::hana::type_c<Type>, std::forward<Args>(args)...); \
    }

#endif // HATNHASMETHOD_H
