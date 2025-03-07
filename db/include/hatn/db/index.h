/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/index.h
  *
  * Contains helpers for definition of database model indexes.
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

#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>
#include <hatn/db/object.h>
#include <hatn/db/field.h>

HATN_DB_NAMESPACE_BEGIN

HDU_UNIT_WITH(index,(HDU_BASE(object)),
    HDU_FIELD(model,TYPE_STRING,1)
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_REPEATED_FIELD(field_names,TYPE_STRING,3)
    HDU_FIELD(unique,TYPE_BOOL,4)
    HDU_FIELD(date_partition,TYPE_BOOL,5)
    HDU_FIELD(ttl,TYPE_UINT32,7)
)

using IndexFieldInfo=FieldInfo;

#define HDB_TTL(ttl) hana::uint<ttl>

struct Unique : public hana::true_
{};

struct UniqueInPartition : public hana::true_
{};

using NotUnique=hana::false_;
using DatePartition=hana::true_;
using NotDatePartition=hana::false_;
using NotTtl=hana::uint<0>;

struct IndexTag{};

template <typename UniqueT=NotUnique,
         typename DatePartitionT=NotDatePartition, typename TtlT=NotTtl>
struct IndexConfig
{
    using hana_tag=IndexTag;

    static_assert(!DatePartitionT::value || !(UniqueT::value || TtlT::value),
                  "Partition index must not have any other attributes"
                  );

    constexpr static bool unique()
    {
        return UniqueT::value;
    }

    constexpr static auto uniqueC()
    {
        return UniqueT{};
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
constexpr IndexConfig<Unique> UniqueIndexConfig{};
constexpr IndexConfig<UniqueInPartition> UniqueInPartitionIndexConfig{};
constexpr IndexConfig<NotUnique,DatePartition> PartitionIndexConfig{};
template <typename TtlT>
constexpr IndexConfig<NotUnique,NotDatePartition,TtlT> TtlIndexConfig{};

class IndexBase
{
    public:

        IndexBase(std::string name):m_name(std::move(name))
        {}

        const std::string& name() const noexcept
        {
            return m_name;
        }

        //! @note ID of index outside model is empty
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
            return hana::append(finfs,IndexFieldInfo{field});
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
                m_fields.emplace_back(field);
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

        //! @todo Hold UTF-8 comparator for UTF-8 indexes
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
        static_assert(decltype(!hana::is_nothing(idx))::value,"Index does not include provided field");
        const auto& finfs=fieldInfos();
        return &(hana::at(finfs,idx.value()));
    }

    template <typename FieldT, typename PosT>
    static const IndexFieldInfo* fieldInfoPos(const FieldT&, const PosT&)
    {
        constexpr static const auto idx=hana::find(fieldIdx,hana::type_c<std::decay_t<FieldT>>);
        static_assert(decltype(!hana::is_nothing(idx))::value,"Index does not include provided field");
        static_assert(std::decay_t<decltype(idx.value())>::value==std::decay_t<PosT>::value,"Wrong field order in the index");
        return &(hana::at(fieldInfos(),idx.value()));
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
            hana::is_a<IndexTag,ConfigT>,
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
                      "Date partition index can contain only single field");

        using firstFieldType=typename std::decay_t<decltype(hana::front(ft))>::Type::type;
        static_assert(!isDatePartition::value || (isDatePartition::value &&
                                                      (std::is_same<firstFieldType,common::DateTime>::value ||
                                                       std::is_same<firstFieldType,common::Date>::value ||
                                                       std::is_same<firstFieldType,common::DateRange>::value ||
                                                       std::is_same<firstFieldType,ObjectId>::value
                                                       )
                                                  ),
                      "Type of date partition field must be one of: DateTime or Date or DateRange or ObjectId");

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
                hana::is_a<FieldTag,decltype(lastArg)>
            ),
            [&](auto _)
            {
                common::FmtAllocatedBufferChar buf;
                auto handler=[&buf](auto&& field, auto&& idx)
                {
                    if (idx.value==0)
                    {
                        if constexpr (isDatePartition::value)
                        {
                            fmt::format_to(std::back_inserter(buf),"pidx_{}",field.name());
                        }
                        else
                        {
                            fmt::format_to(std::back_inserter(buf),"idx_{}",field.name());
                        }
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
    static const auto& invoke(const UnitT& unit, FieldT&& field)
    {
        if constexpr (hana::is_a<NestedFieldTag,FieldT>)
        {
            const auto& f=unit.fieldAtPath(vld::make_member(field.path));
            return f;
        }
        else if constexpr (hana::is_a<FieldTag,FieldT>)
        {
            const auto& f=unit.field(field.field);
            return f;
        }
        else
        {
            const auto& f=unit.field(field);
            return f;
        }
    }

    template <typename UnitT, typename FieldT>
    const auto& operator()(const UnitT& unit, FieldT&& field) const
    {
        return getIndexFieldT::invoke(unit,std::forward<FieldT>(field));
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

#define HATN_DB_INDEX(idx,...) \
    struct _index_##idx { \
        const auto& operator()() const \
        { \
            static auto idx=HATN_DB_NAMESPACE::makeIndex(HATN_DB_NAMESPACE::DefaultIndexConfig,__VA_ARGS__); \
            return idx; \
        } \
    }; \
    constexpr _index_##idx idx{};

#define HATN_DB_UNIQUE_INDEX(idx,...) \
    struct _index_##idx { \
        const auto& operator()() const \
        { \
            static auto idx=HATN_DB_NAMESPACE::makeIndex(HATN_DB_NAMESPACE::UniqueIndexConfig,__VA_ARGS__); \
            return idx; \
        } \
    }; \
    constexpr _index_##idx idx{};

#define HATN_DB_UNIQUE_IN_PARTITION_INDEX(idx,...) \
    struct _index_##idx { \
            const auto& operator()() const \
        { \
                static auto idx=HATN_DB_NAMESPACE::makeIndex(HATN_DB_NAMESPACE::UniqueInPartitionIndexConfig,__VA_ARGS__); \
                return idx; \
        } \
    }; \
    constexpr _index_##idx idx{};


#define HATN_DB_PARTITION_INDEX(idx,...) \
    struct _index_##idx { \
            const auto& operator()() const \
        { \
                static auto idx=HATN_DB_NAMESPACE::makeIndex(HATN_DB_NAMESPACE::PartitionIndexConfig,__VA_ARGS__); \
                return idx; \
        } \
    }; \
    constexpr _index_##idx idx{};

#define HATN_DB_TTL_INDEX(idx,Ttl,...) \
    struct _index_##idx { \
    const auto& operator()() const \
        { \
            static auto idx=HATN_DB_NAMESPACE::makeIndex(HATN_DB_NAMESPACE::TtlIndexConfig<hana::int_<Ttl>>,__VA_ARGS__); \
            return idx; \
        } \
    }; \
    constexpr _index_##idx idx{};

#endif // HATNDBINDEX_H
