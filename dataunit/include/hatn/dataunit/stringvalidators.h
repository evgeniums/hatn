/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/stringvalidators.h
  *
  *  Difinitions of elementary string validators
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSTRINGVALIDATORS_H
#define HATNDATAUNITSTRINGVALIDATORS_H

#include <boost/algorithm/string/predicate.hpp>

#include <hatn/common/translate.h>

#include <hatn/dataunit/validators.h>

HATN_NAMESPACE_BEGIN
using HATN_COMMON_NS::_TR;
namespace HATN_DATAUNIT_NS {
namespace validators {

template <typename ... Validators>
using BufValidator=FieldValidator<const char*,Validators...>;

/********************** Elementary string validators **************************/

//---------------------------------------------------------------
struct buf_eq
{
    template <typename T>
    struct type
    {
        type(common::ByteArray val):m_val(std::move(val))
        {}
        bool operator() (const common::lib::string_view& val) const noexcept
        {
            return m_val.isEqual(val);
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be equal to {}","validator"),m_val.c_str());
        }
        common::ByteArray m_val;
    };
};

//---------------------------------------------------------------
struct buf_ne
{
    template <typename T>
    struct type
    {
        type(common::ByteArray val):m_val(std::move(val))
        {}
        bool operator() (const common::lib::string_view& val) const noexcept
        {
            return !m_val.isEqual(val);
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be equal to {}","validator"),m_val.c_str());
        }
        common::ByteArray m_val;
    };
};

//---------------------------------------------------------------
struct str_eq
{
    template <typename T>
    struct type
    {
        type(common::ByteArray val):m_val(std::move(val))
        {}
        bool operator() (const common::lib::string_view& val) const noexcept
        {
            return boost::algorithm::equals(val,m_val.stringView());
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be equal to {}","validator"),m_val.c_str());
        }
        common::ByteArray m_val;
    };
};

//---------------------------------------------------------------
struct str_eq_i
{
    template <typename T>
    struct type
    {
        type(common::ByteArray val):m_val(std::move(val))
        {}
        bool operator() (const common::lib::string_view& val) const noexcept
        {
            return boost::algorithm::iequals(val,m_val.stringView());
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be case insensitively equal to {}","validator"),m_val.c_str());
        }
        common::ByteArray m_val;
    };
};

//---------------------------------------------------------------
struct str_ne
{
    template <typename T>
    struct type
    {
        type(common::ByteArray val):m_val(std::move(val))
        {}
        bool operator() (const common::lib::string_view& val) const noexcept
        {
            return !boost::algorithm::equals(val,m_val.stringView());
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must not be equal to {}","validator"),m_val.c_str());
        }
        common::ByteArray m_val;
    };
};

//---------------------------------------------------------------
struct str_ne_i
{
    template <typename T>
    struct type
    {
        type(common::ByteArray val):m_val(std::move(val))
        {}
        bool operator() (const common::lib::string_view& val) const noexcept
        {
            return !boost::algorithm::iequals(val,m_val.stringView());
        }
        std::string errorStr() const
        {
            return fmt::format(_TR("must be case insensitively not equal to {}","validator"),m_val.c_str());
        }
        common::ByteArray m_val;
    };
};

//! \todo
//! str_gt, str_gt_i, str_gte, str_gte_i, str_lt, str_lt_i, str_lte, str_lte_i,
//! str_starts_with, str_starts_with_i, str_ends_with, str_ends_with_i, str_nstarts_with, str_nstarts_with_i, str_nends_with, str_nends_with_i,
//! str_contains, str_contains_i, str_ncontains, str_ncontains_i,
//! str_regex_match, str_regex_nmatch

} // namespace validators

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSTRINGVALIDATORS_H
