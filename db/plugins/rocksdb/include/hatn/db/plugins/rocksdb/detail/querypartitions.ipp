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
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace index_key_search
{

template <typename ModelT>
struct partitionFieldVisitor
{
    partitionFieldVisitor(
            const ModelT& model,
            RocksdbHandler& handler,
            query::Operator op,
            common::pmr::FlatSet<std::shared_ptr<RocksdbPartition>>& partitions
        )
        :model(model),
          handler(handler),
          op(op),
          partitions(partitions)
    {}

    const ModelT& model;
    RocksdbHandler& handler;
    mutable query::Operator op;
    common::pmr::FlatSet<std::shared_ptr<RocksdbPartition>>& partitions;

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
                handleRange(query::toDateRange(v,model.datePartitionMode()),false);
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
    void handleInterval(const query::Interval<T>& iv, bool raw=true) const
    {
        if constexpr (std::is_convertible_v<T,common::DateRange>)
        {
            if (iv.from.type==query::IntervalType::First)
            {
                if (iv.to.type==query::IntervalType::Last)
                {
                    std::copy(handler.p()->partitions.begin(),handler.p()->partitions.end(),partitions);
                }
                else
                {
                    handleRange(query::toDateRange(iv.to.value,model.datePartitionMode()),raw,query::Operator::lte);
                }
                return;
            }

            if (iv.to.type==query::IntervalType::Last)
            {
                handleRange(query::toDateRange(iv.from.value,model.datePartitionMode()),raw,query::Operator::gte);
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
                if (raw)
                {
                    partitions.beginRawInsert(count);
                }
                // iterate to greatest partition
                for (auto i=it.index();i<handler.p()->partitions.size();i++)
                {
                    const auto& partition=handler.p()->partitions.at(i);
                    if (partition->range>rangeTo)
                    {
                        break;
                    }
                    if (raw)
                    {
                        partitions.rawInsert(partition);
                    }
                    else
                    {
                        partitions.insert(partition);
                    }
                }
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
        partitions.beginRawSet(all.size());
        for (size_t i=0;i<partitions.size();i++)
        {
            partitions.rawSet(all.at(i),i);
        }
    }

    void operator()(const query::Last&) const
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

    void operator()(const query::First&) const
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

    void handleRange(const common::DateRange& range, bool raw=true, std::optional<query::Operator> opArg=std::optional<query::Operator>{})
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
                    if (raw)
                    {
                        partitions.beginRawInsert(count);
                    }
                    for (auto i=it.index();i<handler.p()->partitions.size();i++)
                    {
                        if (raw)
                        {
                            partitions.rawInsert(handler.p()->partitions.at(i));
                        }
                        else
                        {
                            partitions.insert(handler.p()->partitions.at(i));
                        }
                    }
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
                    if (raw)
                    {
                        partitions.beginRawInsert(count);
                    }
                    for (auto i=0;i<=it.index();i++)
                    {
                        if (raw)
                        {
                            partitions.rawInsert(handler.p()->partitions.at(i));
                        }
                        else
                        {
                            partitions.insert(handler.p()->partitions.at(i));
                        }
                    }
                }
            }
            break;

            case(query::Operator::eq):
            {
                auto partition=handler.partition(range);
                if (partition)
                {
                    if (raw)
                    {
                        partitions.rawInsert(partition);
                    }
                    else
                    {
                        partitions.insert(partition);
                    }
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
void queryPartitions(
        common::pmr::FlatSet<std::shared_ptr<RocksdbPartition>>& ps,
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& idxQuery
    )
{
    ps.clear();
    const auto& field0=idxQuery.query.field(0);
    hana::eval_if(
        hana::bool_<ModelT::isDatePartitioned()>{},
        [&](auto _)
        {
            common::lib::shared_lock<common::lib::shared_mutex> l{_(handler).p()->partitionMutex};

            const auto& allPartitions=_(handler).p()->partitions;
            _(ps).reserve(allPartitions.size());
            if (ModelT::isDatePartitionField(_(field0).fieldInfo->name())
                && (_(field0).op!=query::Operator::nin)
                && (_(field0).op!=query::Operator::neq)
                )
            {
                // collect partitions matching query expression for the first field
                partitionFieldVisitor<ModelT> visitor{
                    _(model),
                    _(handler),
                    _(field0).op,
                    _(ps)
                };
                visitor(_(field0).value);
            }
            else
            {
                //! @todo optimization: optimize nin and neq operators

                // use all partitions pre-sorted by range
                _(ps).beginRawSet(allPartitions.size());
                for (size_t i=0;i<ps.size();i++)
                {
                    _(ps).rawSet(allPartitions.at(i),i);
                }
            }
        },
        [&](auto _)
        {
            _(ps).beginRawSet(1);
            _(ps).rawSet(_(handler).defaultPartition(),0);
        }
    );
}

} // namespace index_key_search

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBQUERYPARTITIONS_IPP
