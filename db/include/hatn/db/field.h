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
#include <hatn/common/allocatoronstack.h>

#include <hatn/dataunit/unitmeta.h>

#include <hatn/validator/utils/foreach_if.hpp>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

struct FieldPathElement
{
    int fieldId;
    const char* name;
    int id;

    FieldPathElement(
            int fieldId,
            const char* name,
            int id
        ) : fieldId(fieldId),
            name(name),
            id(id)
    {}
};

constexpr const size_t FieldPathDepth=4;
using FieldPath=common::VectorOnStack<FieldPathElement,FieldPathDepth>;

struct FieldPathCompare
{
    inline bool operator ()(const FieldPath& l, const FieldPath& r) const noexcept
    {
        for (size_t i=0;i<l.size();i++)
        {
            if (i>=r.size())
            {
                return false;
            }

            if (l[i].id<r[i].id)
            {
                return true;
            }

            if (l[i].id>r[i].id)
            {
                return false;
            }
        }
        return false;
    }
};

struct ArrayFieldTag
{};

template <typename T>
struct ArrayField
{
    using hana_tag=ArrayFieldTag;

    const T& field;
    int id;
};

struct arrayT
{
    template <typename FieldT, typename T>
    auto operator ()(const FieldT& field, T&& id) const noexcept
    {
        return ArrayField{field,static_cast<int>(id)};
    }
};
constexpr arrayT array{};

struct makePathT
{
    template <typename ...Args>
    FieldPath operator()(Args&&... args) const
    {
        FieldPath path;

        auto make=[&path](const auto& ts)
        {
            path.reserve(hana::size(ts).value);
            hana::for_each(
                ts,
                [&](auto&& arg)
                {
                    auto&& val=vld::unwrap_object(vld::extract_ref(arg));
                    using type=std::decay_t<decltype(val)>;
                    if constexpr (hana::is_a<du::FieldTag,type>)
                    {
                        path.emplace_back(val.id(),val.name(),val.id());
                    }
                    else
                    {
                        static_assert(hana::is_a<ArrayFieldTag,type>,"Invalid path element");
                        path.emplace_back(val.field.id(),val.field.name(),val.id);
                    }
                }
            );
        };

        auto ts=vld::make_cref_tuple(std::forward<Args>(args)...);
        const auto& arg0=hana::front(ts).get();
        if constexpr (hana::is_a<vld::member_tag,decltype(arg0)>)
        {
            make(arg0.path());
        }
        else
        {
            make(ts);
        }

        return path;
    }
};
constexpr makePathT makePath{};

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

    static FieldPath fieldPath()
    {
        static auto p=hana::unpack(path,makePath);
        return p;
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

    static FieldPath fieldPath()
    {
        static auto p=makePath(field);
        return p;
    }
};

struct fieldPathT
{
    template <typename T>
    FieldPath operator()(T&& field) const
    {
        if constexpr (hana::is_a<FieldTag,T> || hana::is_a<NestedFieldTag,T>)
        {
            return field.fieldPath();
        }
        else
        {
            return makePath(field);
        }
    }
};
constexpr fieldPathT fieldPath{};

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
        if constexpr (hana::is_a<vld::member_tag,FieldT>)
        {
            return hana::unpack(hana::transform(field.path(),vld::unwrap_object),fieldT{});
        }
        else
        {
            return hana::id(std::forward<FieldT>(field));
        }
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
