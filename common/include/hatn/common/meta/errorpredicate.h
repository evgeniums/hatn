/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/errorpredocate.h
  *
  * Defines errorPredicate.
  *
  */

/****************************************************************************/

#ifndef HATNERRORPREDICATE_H
#define HATNERRORPREDICATE_H

#include <boost/hana.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

struct true_predicate_t
{
    template <typename T>
    constexpr bool operator()(T&&) const
    {
        return true;
    }
};
constexpr true_predicate_t true_predicate{};

struct error_predicate_t
{
    template <typename T>
    constexpr bool operator()(T&& v) const
    {
        return !v;
    }
};
constexpr error_predicate_t error_predicate{};

HATN_COMMON_NAMESPACE_END

#endif // HATNERRORPREDICATE_H
