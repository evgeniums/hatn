/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/prevalidatedupdate.h
  *
  *  Defines helpers for updating data unit fields with pre-validation.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITPREVALIDATEDUPDATE_H
#define HATNDATAUNITPREVALIDATEDUPDATE_H

#include <boost/hana/ext/std/tuple.hpp>

#include <hatn/validator/prevalidation/set_validated.hpp>
#include <hatn/validator/prevalidation/unset_validated.hpp>
#include <hatn/validator/prevalidation/resize_validated.hpp>
#include <hatn/validator/prevalidation/clear_validated.hpp>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/unit.h>

HATN_VALIDATOR_NAMESPACE_BEGIN

template <typename ObjectT, typename MemberT>
struct set_member_t<
            ObjectT,
            MemberT,
            std::enable_if_t<std::is_base_of<HATN_DATAUNIT_NAMESPACE::Unit,ObjectT>::value>
        >
{
    template <typename ObjectT1, typename MemberT1, typename ValueT>
    void operator() (
            ObjectT1& obj,
            MemberT1&& member,
            ValueT&& val
        ) const
    {
        hana::if_(
            hana::is_a<hana::ext::std::tuple_tag,ValueT>,
            [](auto& obj, auto&& member, auto&& val)
            {
                hana::unpack(std::forward<decltype(val)>(val),
                             [&](auto&&... args)
                             {
                                obj.setAtPath(std::forward<decltype(member)>(member),std::forward<decltype(args)>(args)...);
                             }
                        );
            },
            [](auto& obj, auto&& member, auto&& val)
            {
                obj.setAtPath(std::forward<decltype(member)>(member),std::forward<decltype(val)>(val));
            }
        )(obj,std::forward<MemberT1>(member),std::forward<ValueT>(val));
    }
};

template <typename ObjectT, typename MemberT>
struct unset_member_t<
            ObjectT,
            MemberT,
            std::enable_if_t<std::is_base_of<HATN_DATAUNIT_NAMESPACE::Unit,ObjectT>::value>
        >
{
    template <typename ObjectT1, typename MemberT1>
    void operator() (
            ObjectT1& obj,
            MemberT1&& member
        ) const
    {
        obj.unsetAtPath(std::forward<MemberT1>(member));
    }
};

template <typename ObjectT, typename MemberT>
struct resize_member_t<
            ObjectT,
            MemberT,
            std::enable_if_t<std::is_base_of<HATN_DATAUNIT_NAMESPACE::Unit,ObjectT>::value>
        >
{
    template <typename ObjectT1, typename MemberT1>
    void operator() (
            ObjectT1& obj,
            MemberT1&& member,
            size_t size
        ) const
    {
        obj.resizeAtPath(std::forward<MemberT1>(member),size);
    }
};

template <typename ObjectT, typename MemberT>
struct clear_member_t<
            ObjectT,
            MemberT,
            std::enable_if_t<std::is_base_of<HATN_DATAUNIT_NAMESPACE::Unit,ObjectT>::value>
        >
{
    template <typename ObjectT1, typename MemberT1>
    void operator() (
            ObjectT1& obj,
            MemberT1&& member
        ) const
    {
        obj.clearAtPath(std::forward<MemberT1>(member));
    }
};

HATN_VALIDATOR_NAMESPACE_END

#endif // HATNDATAUNITPREVALIDATEDUPDATE_H
