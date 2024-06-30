/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/index.h
  *
  * Contains helpers for definition od database indexes.
  *
  */

/****************************************************************************/

#ifndef HATNDBINDEX_H
#define HATNDBINDEX_H

#include <string>

#include <boost/hana.hpp>

#include <hatn/common/crc32.h>
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
    HDU_REPEATED_FIELD(field_names,TYPE_STRING,3)
    HDU_FIELD(unique,TYPE_BOOL,4)
    HDU_FIELD(date_partition,TYPE_BOOL,5)
    HDU_FIELD(prefix,TYPE_UINT32,6)
    HDU_FIELD(ttl,TYPE_UINT32,7)
    HDU_FIELD(topic,TYPE_BOOL,8)
)

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
                if (idx.value==0)
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
};

struct nestedIndexFieldT
{
    template <typename ...PathT>
    constexpr auto operator()(PathT ...path) const
    {
        return NestedField<decltype(hana::make_tuple(path...))>{};
    }
};
constexpr nestedIndexFieldT nestedIndexField{};

struct AutoSizeTag{};

struct AutoSizeT
{
    using hana_tag=AutoSizeTag;
};
constexpr AutoSizeT AutoSize{};

struct IndexFieldTag{};

using NotNullT=hana::false_;
constexpr NotNullT NotNull{};

using NullableT=hana::true_;
constexpr NullableT Nullable{};

namespace detail {

template <typename T, typename LengthT>
struct IndexFieldHelper
{
    static constexpr size_t size=LengthT::value;
};

template <typename T>
struct IndexFieldHelper<T,AutoSizeT>
{
    using maxSize=typename T::maxSize;
    static constexpr size_t size=maxSize::value;
};

}

template <typename FieldT, typename LengthT=AutoSizeT, typename NullableT=NotNullT>
struct IndexField
{
    using hana_tag=IndexFieldTag;

    using field_type=FieldT;
    using Type=typename std::decay_t<field_type>::Type;

    constexpr static const field_type field{};

    constexpr static size_t length() noexcept
    {
        return detail::IndexFieldHelper<Type,LengthT>::size;
    }

    constexpr static bool nullable() noexcept
    {
        return NullableT::value;
    }

    static auto name()
    {
        return std::decay_t<field_type>::name();
    }
};

struct indexFieldT
{
    template <typename FieldT, typename LengthT, typename NullableT>
    constexpr static auto create(FieldT, LengthT, NullableT)
    {
        return IndexField<FieldT,std::decay_t<LengthT>,std::decay_t<NullableT>>{};
    }

    template <typename FieldT, typename LengthT, typename NullableT>
    constexpr auto operator()(FieldT f, LengthT l, NullableT n) const
    {
        return create(f,l,n);
    }

    template <typename FieldT, typename LengthT>
    constexpr auto operator()(FieldT f, LengthT l) const
    {
        return create(f,l,NotNull);
    }

    template <typename FieldT>
    constexpr auto operator()(FieldT f) const
    {
        return create(f,AutoSize,NotNull);
    }
};
constexpr indexFieldT indexField{};

#define HDB_LENGTH(l) hana::size_t<l>{}
#define HDB_TTL(ttl) hana::uint<ttl>

using Unique=hana::true_;
using NotUnique=hana::false_;
using DatePartition=hana::true_;
using NotDatePartition=hana::false_;
using NotTtl=hana::uint<0>;
using Topic=hana::true_;
using NotTopic=hana::false_;

struct IndexConfigTag{};

template <typename UniqueT=NotUnique,
         typename DatePartitionT=NotDatePartition, typename TtlT=NotTtl,
         typename TopicT=NotTopic>
struct IndexConfig
{
    using hana_tag=IndexConfigTag;

    constexpr static bool unique()
    {
        return UniqueT::value;
    }

    constexpr static bool isDatePartitioned()
    {
        return DatePartitionT::value;
    }

    constexpr static uint32_t ttl()
    {
        return TtlT::value;
    }

    constexpr static bool isTtl()
    {
        return TtlT::value>0;
    }

    constexpr static bool topic()
    {
        return TopicT::value;
    }
};
constexpr IndexConfig<> DefaultIndexConfig{};

template <typename ConfigT, typename ...Fields>
struct Index : public ConfigT
{
    //! @todo Disallow floaing point indexes.

    constexpr static const hana::tuple<std::decay_t<Fields>...> fields{};

    Index(std::string name):m_name(std::move(name))
    {}

    const std::string& name() const
    {
        return m_name;
    }

    const std::string& id() const
    {
        return m_id;
    }

    const std::string& collection() const
    {
        return m_collection;
    }

    constexpr static decltype(auto) datePartitionField()
    {
        return hana::front(fields);
    }

    void setCollection(std::string val)
    {
        m_collection=std::move(val);
        m_id=common::Crc32Hex(m_collection,m_name);
    }

    private:

        std::string m_name;
        std::string m_collection;
        std::string m_id;
};

struct makeIndexT
{
    template <typename ConfigT, typename ...Fields>
    auto operator()(ConfigT&& cfg, Fields&& ...fields) const
    {
        auto ft1=hana::make_tuple(fields...);

        auto args=hana::eval_if(
            hana::is_a<IndexConfigTag,ConfigT>,
            [&](auto _)
            {
                return std::make_pair(_(cfg),_(ft1));
            },
            [&](auto _)
            {
                return std::make_pair(DefaultIndexConfig,hana::prepend(_(ft1),_(cfg)));
            }
        );
        auto&& config=args.first;
        using configT=std::decay_t<decltype(config)>;
        auto&& ft=args.second;

        using isDatePartition=hana::bool_<configT::isDatePartitioned()>;
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

        using isTtl=hana::bool_<configT::isTtl()>;
        static_assert(!isTtl::value || (isTtl::value && (decltype(hana::size(ft))::value==1)),
                      "Ttl mode can be used for index with single field only");
        static_assert(!isTtl::value || (isTtl::value &&
                                                  (std::is_same<firstFieldType,common::DateTime>::value
                                                   )
                                                  ),
                      "Type of ttl field must be of DateTime only");

        const auto& lastArg=hana::back(ft);
        auto p=hana::eval_if(
            hana::or_(
                hana::is_a<HATN_DATAUNIT_NAMESPACE::FieldTag,decltype(lastArg)>,
                hana::is_a<NestedFieldTag,decltype(lastArg)>,
                hana::is_a<IndexFieldTag,decltype(lastArg)>
            ),
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
                static_assert(std::is_constructible<std::string,decltype(_(lastArg))>::value,"Last argument must be a name of the index");
                return std::make_pair(hana::drop_back(_(ft)),_(lastArg));
            }
        );

        auto t=hana::prepend(std::move(p.first),config);
        auto c=common::tupleToTupleC(t);
        auto ci=hana::unpack(c,hana::template_<Index>);
        using type=typename decltype(ci)::type;
        return type{std::move(p.second)};
    }
};
constexpr makeIndexT makeIndex{};

struct getIndexFieldT
{
    template <typename UnitT, typename FieldT>
    static decltype(auto) invoke(UnitT&& unit, FieldT&& field)
    {
        return hana::eval_if(
            hana::is_a<NestedFieldTag,FieldT>,
            [&](auto _)
            {
                auto handler=[](auto&& currentUnit, auto&& currentField)
                {
                    return currentUnit.field(currentField);
                };
                return hana::fold(_(field).path,_(unit),handler);
            },
            [&](auto _)
            {
                return hana::eval_if(
                    hana::is_a<IndexFieldTag,FieldT>,
                    [&](auto _)
                    {
                        return getIndexFieldT::invoke(_(unit),_(field).field);
                    },
                    [&](auto _)
                    {
                        return _(unit).field(_(field));
                    }
                );
            }
        );
    }

    template <typename UnitT, typename FieldT>
    decltype(auto) operator()(UnitT&& unit, FieldT&& field) const
    {
        return getIndexFieldT::invoke(std::forward<UnitT>(unit),std::forward<FieldT>(field));
    }
};
constexpr getIndexFieldT getIndexField{};

struct getPlainIndexFieldT
{
    template <typename UnitT, typename IndexT>
    decltype(auto) operator()(UnitT&& unit, IndexT&& idx) const
    {
        return hana::eval_if(
            hana::is_a<hana::type_tag,IndexT>,
            [&](auto _)
            {
                const auto& _idx=_(idx);
                using typeC=std::decay_t<decltype(_idx)>;
                using idxT=typename typeC::type;
                return getIndexField(_(unit),
                                     hana::front(idxT::fields)
                                     );
            },
            [&](auto _)
            {
                const auto& _idx=_(idx);
                using idxT=std::decay_t<decltype(_idx)>;
                return getIndexField(_(unit),
                                     hana::front(idxT::fields)
                                     );
            }
        );
    }
};
constexpr getPlainIndexFieldT getPlainIndexField{};

HATN_DB_NAMESPACE_END

#endif // HATNDBINDEX_H
