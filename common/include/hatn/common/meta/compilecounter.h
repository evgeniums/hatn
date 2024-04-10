/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/compiletcounter.h
  *
  * Defines compilation time counter.
  */

/****************************************************************************/

#ifndef HATNCOMPILECOUNTER_H
#define HATNCOMPILECOUNTER_H

#include <hatn/thirdparty/fameta/counter.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

#define HATN_COUNTER_PREPARE(Id) template <int> struct c_foo_##Id{}; struct cc_##Id{}; constexpr static fameta::counter<cc_##Id,0> c_##Id{};
#define HATN_COUNTER_INC(Id) template <> struct c_foo_##Id<c_##Id.next<__COUNTER__>()>{};
#define HATN_COUNTER_MAKE(Id) HATN_COUNTER_PREPARE(Id) \
HATN_COUNTER_INC(Id)
#define HATN_COUNTER_GET(Id) c_##Id.current<__COUNTER__>()

HATN_COMMON_NAMESPACE_END

#endif // HATNCOMPILECOUNTER_H
