/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/field.h
  *
  * Contains helpers for definition od database model fields.
  *
  */

/****************************************************************************/

#ifndef HATNDBFIELD_H
#define HATNDBFIELD_H

#include <string>

#include <hatn/common/format.h>

#include <hatn/dataunit/unitmeta.h>

#include <hatn/validator/utils/foreach_if.hpp>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

struct NestedFieldTag{};

template <typename PathT>
struct NestedField
{
    using hana_tag=NestedFieldTag;

    constexpr static const PathT path{};
    using Type=typename std::decay_t<decltype(hana::back(path))>::Type;

    static std::string name()
    {
        auto fillName=[]()
        {
            common::FmtAllocatedBufferChar buf;
            auto handler=[&buf](auto&& field, auto&& idx)
            {
                if constexpr (std::decay_t<decltype(idx)>::value==0)
                {
                    fmt::format_to(std::back_inserter(buf),"{}",field.name());
                }
                else
                {
                    fmt::format_to(std::back_inserter(buf),"__{}",field.name());
                }
                return true;
            };
            HATN_VALIDATOR_NAMESPACE::foreach_if(path,HATN_DATAUNIT_META_NAMESPACE::true_predicate,handler);
            return common::fmtBufToString(buf);
        };

        static std::string str=fillName();
        return str;
    }

    constexpr static int id()
    {
        return -1;
    }
};

struct nestedFieldT
{
    template <typename ...PathT>
    constexpr auto operator()(PathT ...path) const
    {
        return NestedField<decltype(hana::make_tuple(path...))>{};
    }
};
constexpr nestedFieldT nestedField{};
constexpr nestedFieldT nested{};

struct FieldTag{};

template <typename FieldT>
struct Field
{
    using hana_tag=FieldTag;

    using field_type=FieldT;
    using Type=typename std::decay_t<field_type>::Type;

    constexpr static const field_type field{};

    static auto name()
    {
        return std::decay_t<field_type>::name();
    }

    static auto id()
    {
        return std::decay_t<field_type>::id();
    }
};

class FieldInfo
{
    public:

        FieldInfo(std::string name, int id)
            : m_name(std::move(name)),
              m_id(id),
              m_nested(false)
        {
            boost::split(m_path,name,boost::is_any_of("."));
        }

        template <typename FieldT>
        explicit FieldInfo(const FieldT& field)
            : FieldInfo(field.name(),field.id())
        {}

        const std::string& name() const noexcept
        {
            return m_name;
        }

        int id() const noexcept
        {
            return m_id;
        }

        bool nested() const noexcept
        {
            return m_nested;
        }

        const std::vector<std::string>& path() const noexcept
        {
            return m_path;
        }

    private:

        std::string m_name;
        int m_id;
        bool m_nested;
        std::vector<std::string> m_path;
};

struct fieldT
{
    template <typename FieldT>
    auto operator() (FieldT&& field) const -> decltype(auto)
    {
        return hana::id(std::forward<FieldT>(field));
    }

    template <typename ...FieldsT>
    auto operator() (FieldsT&&... fields) const
    {
        return nestedField(std::forward<FieldsT>(fields)...);
    }
};
constexpr fieldT field{};

HATN_DB_NAMESPACE_END

#endif // HATNDBFIELD_H
