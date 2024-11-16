/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/querypartitions.ipp
  *
  *   RocksDB partitions collect.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBQUERYPARTITIONS_IPP
#define HATNROCKSDBQUERYPARTITIONS_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/dberror.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/indexquery.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/indexkeysearch.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace index_key_search
{

template <typename CompT>
using FilteredPartitions=common::FlatSetOnStack<std::shared_ptr<RocksdbPartition>,PresetPartitionsCount,CompT>;

template <typename ModelT, typename CompT>
struct partitionFieldVisitor
{
    partitionFieldVisitor(
            const ModelT& model,
            RocksdbHandler& handler,
            query::Operator op,
            FilteredPartitions<CompT>& partitions
        )
        :model(model),
          handler(handler),
          op(op),
          partitions(partitions)
    {}

    const ModelT& model;
    RocksdbHandler& handler;
    mutable query::Operator op;
    FilteredPartitions<CompT>& partitions;

    template <typename T>
    void operator()(const query::Vector<T>& vec) const
    {
        if constexpr (std::is_convertible_v<T,common::DateRange>)
        {
            if (op==query::Operator::in)
            {
                op=query::Operator::eq;
            }
            for (const auto& v: vec)
            {
                handleRange(query::toDateRange(v,model.datePartitionMode()));
            }
        }
    }

    template <typename T>
    void operator()(const query::Vector<query::Interval<T>>& vec) const
    {
        if constexpr (std::is_convertible_v<T,common::DateRange>)
        {
            for (const auto& iv: vec)
            {
                handleInterval(iv);
            }
        }
    }

    template <typename T>
    void handleInterval(const query::Interval<T>& iv) const
    {
        if constexpr (std::is_convertible_v<T,common::DateRange>)
        {
            if (iv.from.type==query::IntervalType::First)
            {
                if (iv.to.type==query::IntervalType::Last)
                {
                    allPartitions();
                }
                else
                {
                    handleRange(query::toDateRange(iv.to.value,model.datePartitionMode()),query::Operator::lte);
                }
                return;
            }

            if (iv.to.type==query::IntervalType::Last)
            {
                handleRange(query::toDateRange(iv.from.value,model.datePartitionMode()),query::Operator::gte);
                return;
            }

            common::DateRange rangeFrom=iv.from.value;
            common::DateRange rangeTo=iv.to.value;

            // find lowest partition
            auto it=std::lower_bound(handler.p()->partitions.begin(),handler.p()->partitions.end(),
                                       rangeFrom,
                                       std::less<std::shared_ptr<RocksdbPartition>>{}
                                       );
            if (it!=handler.p()->partitions.end())
            {
                size_t count=handler.p()->partitions.size()-it.index();
                partitions.beginRawInsert(count);

                // iterate to greatest partition
                for (auto i=it.index();i<handler.p()->partitions.size();i++)
                {
                    const auto& partition=handler.p()->partitions.at(i);
                    if (partition->range>rangeTo)
                    {
                        break;
                    }
                    partitions.rawInsert(partition);
                }

                partitions.endRawInsert();
            }
        }
    }

    template <typename T>
    void operator()(const query::Interval<T>& iv) const
    {
        handleInterval(iv);
    }

    void allPartitions()
    {
        const auto& all=handler.p()->partitions;
        partitions.beginRawInsert(all.size());
        for (size_t i=0;i<all.size();i++)
        {
            partitions.rawInsert(all.at(i));
        }
        partitions.endRawInsert();
    }

    void operator()(const query::LastT&) const
    {
        if (op==query::Operator::lt || op==query::Operator::lte)
        {
            allPartitions();
        }
        else
        {
            Assert(false,"Invalid query operator for Last");
        }
    }

    void operator()(const query::FirstT&) const
    {
        if (op==query::Operator::gt || op==query::Operator::gte)
        {
            allPartitions();
        }
        else
        {
            Assert(false,"Invalid query operator for First");
        }
    }

    template <typename T>
    void operator()(const T& v) const
    {
        if constexpr (std::is_convertible_v<T,common::DateRange>)
        {
            handleRange(query::toDateRange(v,model.datePartitionMode()));
        }
        else
        {
            Assert(false,"Invalid partition field");
        }
    }

    void handleRange(const common::DateRange& range, std::optional<query::Operator> opArg=std::optional<query::Operator>{})
    {
        auto op_=opArg.value_or(op);
        switch(op_)
        {
            case(query::Operator::lt):
            HATN_FALLTHROUGH
            case(query::Operator::lte):
            {
                auto it=std::lower_bound(handler.p()->partitions.begin(),handler.p()->partitions.end(),
                            range,
                            std::less<std::shared_ptr<RocksdbPartition>>{}
                        );
                if (it!=handler.p()->partitions.end())
                {
                    size_t count=handler.p()->partitions.size()-it.index();
                    partitions.beginRawInsert(count);
                    for (auto i=it.index();i<handler.p()->partitions.size();i++)
                    {
                        partitions.rawInsert(handler.p()->partitions.at(i));
                    }
                    partitions.endRawInsert();
                }
            }
            break;

            case(query::Operator::gt):
            HATN_FALLTHROUGH
            case(query::Operator::gte):
            {
                auto it=std::lower_bound(handler.p()->partitions.begin(),handler.p()->partitions.end(),
                                           range,
                                           std::less<std::shared_ptr<RocksdbPartition>>{}
                                           );
                if (it!=handler.p()->partitions.end())
                {
                    size_t count=it.index()+1;
                    partitions.beginRawInsert(count);
                    for (auto i=0;i<=it.index();i++)
                    {
                        partitions.rawInsert(handler.p()->partitions.at(i));
                    }
                    partitions.endRawInsert();
                }
            }
            break;

            case(query::Operator::eq):
            {
                auto partition=handler.partition(range);
                if (partition)
                {
                    partitions.insert(partition);
                }
            }
            break;

            default:
            {
                Assert(false,"Unsupported query operator for partition field");
            }
            break;
        }
    }
};

template <typename ModelT>
bool queryPartitions(
        index_key_search::Partitions& ps,
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& idxQuery
    )
{
    ps.clear();

    const auto& field0=idxQuery.query.field(0);
    return hana::eval_if(
        hana::bool_<ModelT::isDatePartitioned()>{},
        [&](auto _)
        {
            bool firtsFieldPartitioned=ModelT::isDatePartitionField(_(field0).fieldInfo->name())
                                       && (_(field0).op!=query::Operator::nin)
                                       && (_(field0).op!=query::Operator::neq);
            if (firtsFieldPartitioned)
            {
                // partition sets must be ordered by first field
                auto comp=[&](const std::shared_ptr<RocksdbPartition>& l,
                            const std::shared_ptr<RocksdbPartition>& r
                            )
                {
                    if (_(field0).order==query::Order::Desc)
                    {
                        return l->range > r->range;
                    }
                    l->range < r->range;
                };
                using compT=decltype(comp);
                FilteredPartitions<compT> filteredPs(comp);

                // filter partitions
                {
                    common::lib::shared_lock<common::lib::shared_mutex> l{_(handler).p()->partitionMutex};
                    filteredPs.reserve(_(handler).p()->partitions.size());

                    // collect partitions matching query expression for the first field
                    partitionFieldVisitor<ModelT,compT> visitor{
                        _(model),
                        _(handler),
                        _(field0).op,
                        filteredPs
                    };
                    visitor(_(field0).value);
                }

                // copy filtered partitions to result
                _(ps).reserve(filteredPs.size());
                for (size_t i=0;i<filteredPs.size();i++)
                {
                    _(ps).push_back(filteredPs.at(i));
                }
            }
            else
            {                
                common::lib::shared_lock<common::lib::shared_mutex> l{_(handler).p()->partitionMutex};
                _(ps).reserve(_(handler).p()->partitions.size());

                //! @todo optimization: optimize nin and neq operators

                // use all partitions pre-sorted by range
                for (size_t i=0;i<ps.size();i++)
                {
                    _(ps).push_back(_(handler).p()->partitions.at(i));
                }
            }
            return firtsFieldPartitioned;
        },
        [&](auto _)
        {
            _(ps).push_back(_(handler).defaultPartition());
            return false;
        }
    );
}

} // namespace index_key_search

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBQUERYPARTITIONS_IPP
