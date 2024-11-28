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

    //! @todo Implement vector of intervals
    template <typename T>
    void operator()(const query::Vector<query::Interval<T>>& vec)
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
    void handleInterval(const query::Interval<T>& iv)
    {
        if (iv.from.type==query::IntervalType::First && iv.to.type==query::IntervalType::Last)
        {
            allPartitions();
            return;
        }

        if constexpr (std::is_convertible_v<T,common::DateRange> || std::is_same_v<T,ObjectId>)
        {
            if (iv.from.type==query::IntervalType::First)
            {
                handleRange(query::toDateRange(iv.to.value,model.datePartitionMode()),query::Operator::lte);
                return;
            }

            if (iv.to.type==query::IntervalType::Last)
            {
                handleRange(query::toDateRange(iv.from.value,model.datePartitionMode()),query::Operator::gte);
                return;
            }

            common::DateRange rangeFrom=query::toDateRange(iv.from.value,model.datePartitionMode());
            common::DateRange rangeTo=query::toDateRange(iv.to.value,model.datePartitionMode());

            // find lowest partition
            auto it=handler.p()->partitions.lower_bound(rangeFrom);
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
        else
        {
            Assert(false,"Invalid type for date partition interval");
        }
    }

    template <typename T>
    void operator()(const query::Interval<T>& iv)
    {
        handleInterval(iv);
    }

    void allPartitions()
    {
        common::lib::shared_lock<common::lib::shared_mutex> l{handler.p()->partitionMutex};

        const auto& all=handler.p()->partitions;
        partitions.beginRawInsert(all.size());
        for (size_t i=0;i<all.size();i++)
        {
            partitions.rawInsert(all.at(i));
        }
        partitions.endRawInsert();
    }

    void operator()(const query::LastT&)
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

    void operator()(const query::FirstT&)
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
    void operator()(const T& v)
    {
        handleRange(query::toDateRange(v,model.datePartitionMode()));
    }

    void handleRange(const common::DateRange& range, std::optional<query::Operator> opArg=std::optional<query::Operator>{}) const
    {
        auto op_=opArg.value_or(op);
        switch(op_)
        {
            case(query::Operator::gt):
            HATN_FALLTHROUGH
            case(query::Operator::gte):
            {
                auto it=handler.p()->partitions.lower_bound(range);
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

            case(query::Operator::lt):
            HATN_FALLTHROUGH
            case(query::Operator::lte):
            {
                auto it=handler.p()->partitions.lower_bound(range);
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
                    return l->range < r->range;
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
                    if (_(field0).value.isVectorType())
                    {
                        if (visitor.op==query::Operator::in)
                        {
                            visitor.op=query::Operator::eq;
                        }
                        const auto& m=_(model);
                        std::ignore=_(field0).value.eachVectorItem(
                            [&visitor,&m](const auto& v)
                            {
                                visitor.handleRange(query::toDateRange(v,m.datePartitionMode()));
                                return Error{OK};
                            }
                        );
                    }
                    else
                    {
                        lib::variantVisit(visitor,_(field0).value.value());
                    }
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
                for (size_t i=0;i<_(handler).p()->partitions.size();i++)
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
