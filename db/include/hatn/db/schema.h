/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/schema.h
  *
  * Contains helpers for database schema definition.
  *
  */

/****************************************************************************/

#ifndef HATNDBSCHEMA_H
#define HATNDBSCHEMA_H

#include <set>
#include <string>

#include <boost/hana.hpp>

#include <hatn/common/meta/tupletypec.h>
#include <hatn/common/datetime.h>

#include <hatn/validator/utils/foreach_if.hpp>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>
#include <hatn/db/object.h>

HATN_DB_NAMESPACE_BEGIN

HDU_UNIT_WITH(index,(HDU_BASE(object)),
    HDU_FIELD(model,TYPE_STRING,1)
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_REPEATED_FIELD(field_names,TYPE_UINT32,3)
    HDU_FIELD(unique,TYPE_BOOL,4)
    HDU_FIELD(date_partition,TYPE_BOOL,5)
    HDU_FIELD(prefix,TYPE_UINT32,6)
    HDU_FIELD(ttl,TYPE_UINT32,7)
    HDU_FIELD(topic,TYPE_BOOL,8)
)

struct NestedFieldTag{};

template <typename FieldT, typename SubFieldT>
struct NestedField
{
    using hana_tag=NestedFieldTag;

    FieldT field;
    SubFieldT subField;
};

#define HDB_NAME_PREPARE(Name) \
    struct _hdb_t_##Name \
    { \
        constexpr static const char* name=#Name; \
    }; \
    constexpr _hdb_t_##Name _hdb_s_##Name{};

#define HDB_NAME(Name) _hdb_s_##Name
#define HDB_TTL(ttl) hana::int_<ttl>

using Unique=hana::true_;
using NotUnique=hana::false_;
using DatePartition=hana::true_;
using NotDatePartition=hana::false_;
using NotTtl=hana::int_<0>;
using Topic=hana::true_;
using NotTopic=hana::false_;

template <typename UniqueT=NotUnique,
         typename DatePartitionT=NotDatePartition, typename TtlT=NotTtl,
         typename TopicT=NotTopic>
struct IndexConfig
{
    constexpr static bool unique()
    {
        return UniqueT::value;
    }

    constexpr static bool is_date_partition()
    {
        return DatePartitionT::value;
    }

    constexpr static int ttl()
    {
        return TtlT::value;
    }

    constexpr static bool is_ttl()
    {
        return TtlT::value>0;
    }

    constexpr static bool topic()
    {
        return TopicT::value;
    }
};

template <typename ConfigT, typename ...Fields>
struct Index : public ConfigT
{
    constexpr static const boost::hana::tuple<Fields...> fields{};

    Index(std::string name):m_name(std::move(name))
    {}

    const std::string& name() const
    {
        return m_name;
    }

    constexpr static decltype(auto) date_partition_field()
    {
        return hana::front(fields);
    }

    private:

        std::string m_name;
};

struct makeIndexT
{
    template <typename ConfigT, typename ...Fields>
    auto operator()(ConfigT&& config, Fields&& ...fields) const
    {
        auto ft=hana::make_tuple(fields...);
        using isDatePartition=hana::bool_<std::decay_t<ConfigT>::is_date_partition()>;
        static_assert(!isDatePartition::value || (isDatePartition::value && (decltype(hana::size(ft))::value==1)),
                      "Date partition can be used for index with single field only");
        using firstFieldType=typename std::decay_t<decltype(hana::front(ft))>::Type::type;
        static_assert(!isDatePartition::value || (isDatePartition::value &&
                                                      (std::is_same<firstFieldType,common::DateTime>::value ||
                                                       std::is_same<firstFieldType,common::Date>::value ||
                                                       std::is_same<firstFieldType,ObjectId>::value
                                                       )
                                                  ),
                      "Type of date partition field must be one of: DateTime or Date or ObjectId");

        using isTtl=hana::bool_<std::decay_t<ConfigT>::is_ttl()>;
        static_assert(!isTtl::value || (isTtl::value && (decltype(hana::size(ft))::value==1)),
                      "Ttl mode can be used for index with single field only");
        static_assert(!isTtl::value || (isTtl::value &&
                                                  (std::is_same<firstFieldType,common::DateTime>::value
                                                   )
                                                  ),
                      "Type of ttl field must be DateTime only");

        const auto& lastArg=hana::back(ft);
        auto p=hana::eval_if(
            hana::is_a<HATN_DATAUNIT_NAMESPACE::FieldTag,decltype(lastArg)>,
            [&](auto _)
            {
                common::FmtAllocatedBufferChar buf;
                auto handler=[&buf](auto&& field, auto&& idx)
                {
                    if (idx.value==0)
                    {
                        fmt::format_to(std::back_inserter(buf),"idx_{}",field.name());
                    }
                    else
                    {
                        fmt::format_to(std::back_inserter(buf),"_{}",field.name());
                    }
                    return true;
                };
                HATN_VALIDATOR_NAMESPACE::foreach_if(_(ft),HATN_DATAUNIT_META_NAMESPACE::true_predicate,handler);
                return std::make_pair(std::move(_(ft)),common::fmtBufToString(buf));
            },
            [&](auto _)
            {
                static_assert(std::is_constructible<std::string,decltype(lastArg)>::value,"Last argument must be a name of the index");
                return std::make_pair(hana::drop_back(_(ft)),_(lastArg));
            }
        );

        auto t=hana::prepend(std::move(p.first),std::forward<ConfigT>(config));
        auto c=common::tupleToTupleC(t);
        auto ci=hana::unpack(c,hana::template_<Index>);
        using type=typename decltype(ci)::type;
        return type{std::move(p.second)};
    }
};
constexpr makeIndexT makeIndex{};

HDU_UNIT(model_field,
    HDU_FIELD(id,TYPE_UINT32,1)
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_FIELD(field_type,TYPE_STRING,3)
    HDU_FIELD(repeated,TYPE_BOOL,4)
    HDU_FIELD(default_flag,TYPE_BOOL,5)
    HDU_FIELD(default_value,TYPE_STRING,6)
)

using DatePartitionMode=common::DateRange::Type;

HDU_UNIT_WITH(model,(HDU_BASE(object)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_REPEATED_FIELD(model_fields,model_field::TYPE,2)
    HDU_FIELD(date_partition_mode,HDU_TYPE_ENUM(DatePartitionMode),3)
    HDU_FIELD(date_partition_field,TYPE_STRING,4)
)

template <typename ModelT, typename UnitT, typename ...Indexes>
struct Model
{
    boost::hana::tuple<Indexes...> indexes;
    ModelT definition;
    UnitT sample;
};

struct datePartitionFieldT
{
    template <typename IndexesT>
    constexpr decltype(auto) operator()(IndexesT&& indexes) const
    {
        hana::find_if(
            std::forward<IndexesT>(indexes),
            [](auto&& index)
            {
                return hana::not_(std::is_same<decltype(std::decay_t<decltype(index)>::datePartitionField),hana::false_>{});
            }
        );
    }
};
constexpr datePartitionFieldT datePartitionField{};

template <typename ...Models>
struct Schema
{
    std::string name;
    boost::hana::tuple<Models...> models;
    std::set<DatePartitionMode> partitionModes;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBSCHEMA_H
