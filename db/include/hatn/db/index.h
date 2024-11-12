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
#include <boost/algorithm/string.hpp>

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
    HDU_FIELD(ttl,TYPE_UINT32,7)
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

    static auto id()
    {
        return std::decay_t<field_type>::id();
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

class IndexFieldInfo
{
    public:

        IndexFieldInfo(std::string name, int id, bool nullable=false)
            : m_name(std::move(name)),
              m_id(id),
              m_nested(false),
              m_nullable(nullable)
        {
            boost::split(m_path,name,boost::is_any_of("."));
        }

        // IndexFieldInfo(std::string nestedName, bool nullable=false)
        //     : IndexFieldInfo(std::move(nestedName),-1,nullable)
        // {}

        template <typename FieldT>
        explicit IndexFieldInfo(const FieldT& field, bool nullable=false)
            : IndexFieldInfo(field.name(),field.id(),nullable)
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

        bool nullable() const noexcept
        {
            return m_nullable;
        }

        const std::vector<std::string>& path() const noexcept
        {
            return m_path;
        }

    private:

        std::string m_name;
        int m_id;
        bool m_nested;
        bool m_nullable;
        std::vector<std::string> m_path;
};

#define HDB_LENGTH(l) hana::size_t<l>{}
#define HDB_TTL(ttl) hana::uint<ttl>

using Unique=hana::true_;
using NotUnique=hana::false_;
using DatePartition=hana::true_;
using NotDatePartition=hana::false_;
using NotTtl=hana::uint<0>;

struct IndexConfigTag{};

template <typename UniqueT=NotUnique,
         typename DatePartitionT=NotDatePartition, typename TtlT=NotTtl>
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
};
constexpr IndexConfig<> DefaultIndexConfig{};

class IndexBase
{
    public:

        IndexBase(std::string name):m_name(std::move(name))
        {}

        const std::string& name() const noexcept
        {
            return m_name;
        }

        const std::string& id() const noexcept
        {
            return m_id;
        }

        const std::string& collection() const noexcept
        {
            return m_collection;
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

template <typename Fields>
auto makeIndexFieldInfos(Fields&& fs)
{
    return hana::fold(
        fs,
        hana::make_tuple(),
        [](auto&& finfs, auto&& field)
        {
            bool nullable=hana::eval_if(
                hana::is_a<IndexFieldTag,decltype(field)>,
                [&](auto _)
                {
                    return _(field).nullable();
                },
                [](auto)
                {
                    return false;
                }
                );
            return hana::append(finfs,IndexFieldInfo{field,nullable});
        }
    );
}

template <typename ...Fields>
auto makeFieldInfos(Fields&& ...fields)
{
    return makeIndexFieldInfos(hana::make_tuple(std::forward<Fields>(fields)...));
}

template <typename Fields>
constexpr auto makeIndexFieldInfoIdx(Fields&& fs)
{
    return hana::first(hana::fold(
        fs,
        hana::make_pair(hana::make_map(),hana::int_c<0>),
            [](auto&& state, auto&& field)
            {
                return hana::make_pair(hana::insert(    hana::first(state),
                                                        hana::make_pair(
                                                            hana::type_c<std::decay_t<decltype(field)>>,
                                                            hana::second(state)
                                                        )
                                                    ),
                                       hana::plus(hana::second(state),hana::int_c<1>)
                                       );
            }
        ));
}

class IndexInfo : public IndexBase
{
    public:

        template <typename IndexT>
        IndexInfo(
            const IndexT& idx
            ) : IndexInfo(idx,idx.isDatePartitioned(),idx.unique(),idx.ttl())
        {
            auto eachField=[this](const auto& field)
            {
                bool nullable=hana::eval_if(
                    hana::is_a<IndexFieldTag,decltype(field)>,
                    [&](auto _)
                    {
                        return _(field).nullable();
                    },
                    [&](auto)
                    {
                        return false;
                    }
                    );
                auto f=IndexFieldInfo{field,nullable};
                m_fields.push_back(std::move(f));
            };
            hana::for_each(idx.fields,eachField);
        }

        IndexInfo(
            const IndexBase& base,
            bool datePartitioned,
            bool unique,
            uint32_t ttl
            ) : IndexBase(base),
            m_datePartitioned(datePartitioned),
            m_unique(unique),
            m_ttl(ttl)
        {}

        bool unique() const noexcept
        {
            return m_unique;
        }

        bool isDatePartitioned() const noexcept
        {
            return m_datePartitioned;
        }

        uint32_t ttl() const noexcept
        {
            return m_ttl;
        }

        bool isTtl() const noexcept
        {
            return m_ttl>0;
        }

        const std::vector<IndexFieldInfo>& fields() const noexcept
        {
            return m_fields;
        }

        void setFields(std::vector<IndexFieldInfo> fields)
        {
            m_fields=std::move(fields);
        }

    private:

        std::vector<IndexFieldInfo> m_fields;
        bool m_datePartitioned;
        bool m_unique;
        uint32_t m_ttl;
};

template <typename ConfigT, typename ...Fields>
struct Index : public IndexBase, public ConfigT
{
    //! @note Floaing point indexes are not allowed.

    Index(std::string name)
        : IndexBase(std::move(name)),
          m_indexInfo(*this)
    {}

    constexpr static const hana::tuple<std::decay_t<Fields>...> fields{};
    constexpr static const auto fieldIdx=makeIndexFieldInfoIdx(fields);

    static const auto& fieldInfos()
    {
        static const auto fieldInfos_=makeIndexFieldInfos(fields);
        return fieldInfos_;
    }

    template <typename FieldT>
    static const IndexFieldInfo* fieldInfo(const FieldT&)
    {
        constexpr static const auto idx=hana::find(fieldIdx,hana::type_c<std::decay_t<FieldT>>);
        const auto& finfs=fieldInfos();
        return &(hana::at(finfs,idx.value()));
    }

    constexpr static decltype(auto) frontField()
    {
        return hana::front(fields);
    }

    const IndexInfo* indexInfo() const noexcept
    {
        return &m_indexInfo;
    }

    private:

        const IndexInfo m_indexInfo;
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

struct makePlainIndexesT
{
    template <typename ConfigT, typename ...Fields>
    auto operator()(ConfigT&& cfg, Fields&& ...fields) const
    {
        return hana::fold(
            hana::make_tuple(fields...),
            hana::make_tuple(),
            [&cfg](auto&& ts, auto&& field)
            {
                return hana::append(ts,makeIndex(cfg,field));
            }
        );
    }
};
constexpr makePlainIndexesT makePlainIndexes{};

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
            [&](auto)
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

inline auto objectIndexes()
{
    return hana::make_tuple(makeIndex(IndexConfig<Unique>{},object::_id),
                            makeIndex(DefaultIndexConfig,object::created_at),
                            makeIndex(DefaultIndexConfig,object::updated_at)
                            );
}

HATN_DB_NAMESPACE_END

#endif // HATNDBINDEX_H
