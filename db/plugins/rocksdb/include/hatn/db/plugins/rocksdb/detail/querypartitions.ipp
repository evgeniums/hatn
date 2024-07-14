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
struct partitionFieldVisitorT
{

    partitionFieldVisitorT(
        const ModelT& model,
        RocksdbHandler& handler,
        query::Operator op
        ):model(model),
          handler(handler),
          op(op)
    {}

    const ModelT& model;
    RocksdbHandler& handler;
    query::Operator op;

    template <typename T>
    void operator()(const common::pmr::vector<T>& vec) const
    {
        if constexpr (std::is_convertible_v<T,common::DateRange>)
        {
            if (op==query::Operator::in)
            {
                op=query::Operator::eq;
            }
            for (const auto& v: vec)
            {
                common::DateRange range=v;
                handleRange(range);
            }
        }
    }

    template <typename T>
    void operator()(const common::pmr::vector<query::Interval<T>>& vec) const
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
            common::DateRange rangeFrom=iv.from.value;
            common::DateRange rangeTo=iv.to.value;

            //! @todo find lowest partition
            //! @todo iterate to greatest partition
        }
        //! @todo Special cases for First and Last
    }

    template <typename T>
    void operator()(const query::Interval<T>& iv) const
    {
        handleInterval(iv);
    }

    template <typename T>
    void operator()(const T& v) const
    {
        if constexpr (std::is_convertible_v<T,common::DateRange>)
        {
            common::DateRange range=v;
            handleRange(range);
        }
    }

    void handleRange(const common::DateRange& range)
    {
        //! @todo Special cases for First and Last

        //! @todo Implement
        switch(op)
        {
            case(query::Operator::lt):
            {}
            break;

            case(query::Operator::lte):
            {}
            break;

            case(query::Operator::gt):
            {}
            break;

            case(query::Operator::gte):
            {}
            break;

            case(query::Operator::eq):
            {}
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
common::pmr::vector<std::shared_ptr<RocksdbPartition>> partitions(
        const ModelT& model,
        RocksdbHandler& handler,
        const IndexQuery& idxQuery,
        AllocatorFactory* factory
    )
{
    common::pmr::vector<std::shared_ptr<RocksdbPartition>> ps{1,factory->dataAllocator<std::shared_ptr<RocksdbPartition>>()};
    const auto& field0=idxQuery.field(0);
    hana::eval_if(
        hana::bool_<ModelT::isDatePartitioned()>{},
        [&](auto _)
        {
            const auto& allPartitions=_(handler).p()->partitions;
            _(ps).reserve(allPartitions.size());
            if (ModelT::isDatePartitionField(_(field0).fieldInfo->name()))
            {
                // collect partitions matching query expression for the first field
#if 0
                // extract range from query field
                common::DateRange range{};
                switch (_(field0).value.typeId())
                {
                    case(query::Value::Type::DateTime):
                    {
                        range=datePartition(_(field0).value.as<common::DateTime>(),_(model));
                    }
                    break;

                    case(query::Value::Type::Date):
                    {
                        range=datePartition(_(field0).value.as<common::Date>(),_(model));
                    }
                    break;

                    case(query::Value::Type::DateRange):
                    {
                        range=_(field0).value.as<common::DateRange>();
                    }
                    break;

                    case(query::Value::Type::ObjectId):
                    {
                        range=datePartition(_(field0).value.as<ObjectId>(),_(model));
                    }
                    break;

                    default:
                    {
                        //! @todo Handle vectors and intervals
                        Assert(false,"Invalid partition field");
                    }
                    break;
                }
#endif
                //! @todo eq - one exact partition

                //! @todo lt/lte - partitions before

                //! @todo gt/gte - partitions after

                //! @todo in interval - partitions in interval

                //! @todo in vector - exact partitions in vector

                //! @todo in vector of intervals - partitions in intervals of vector elements

                //! @todo for neq and nin use all partitions
            }
            else
            {
                // use all partitions pre-sorted by range
                _(ps).resize(allPartitions.size());
                for (size_t i=0;i<ps.size();i++)
                {
                    _(ps)[i]=allPartitions.at(i);
                }
            }
        },
        [&](auto _)
        {
            _(ps)[0]=_(handler).defaultPartition();
        }
    );
    return ps;
}

} // namespace index_key_search

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBQUERYPARTITIONS_IPP
