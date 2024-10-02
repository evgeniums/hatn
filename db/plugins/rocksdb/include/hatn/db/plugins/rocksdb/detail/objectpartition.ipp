/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/objectpartition.ipp
  *
  *   Selection of object partition in RocksDB database.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBOBJECTPARTITION_IPP
#define HATNROCKSDBOBJECTPARTITION_IPP

#include <hatn/db/dberror.h>
#include <hatn/db/objectid.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename ModelT, typename ObjectDateT, typename DateT=hana::false_>
HATN_COMMON_NAMESPACE::DateRange objectPartition(
        RocksdbHandler& handler,
        const ModelT& model,
        const ObjectDateT& objectId,
        const DateT& date=DateT{}
    ) const
{
    using modelType=std::decay_t<ModelT>;

    auto partitionR=hana::eval_if(
        hana::bool_<modelType::isDatePartitioned()>{},
        [&](auto _)
        {
            using dateT=std::decay_t<decltype(_(date))>;
            std::pair<std::shared_ptr<RocksdbPartition>,common::DateRange> r;
            r.second=hana::eval_if(
                std::is_same<dateT,hana::false_>{},
                [&](auto _)
                {
                    // check if partition field is _id
                    using modelType=std::decay_t<decltype(_(model))>;
                    using eqT=std::is_same<
                        std::decay_t<decltype(object::_id)>,
                        std::decay_t<decltype(modelType::datePartitionField())>
                        >;
                    if constexpr (eqT::value)
                    {
                        return datePartition(_(objectId),_(model));
                    }
                    else
                    {
                        Assert(eqT::value,"Object ID must be a date partition index field");
                        return HATN_COMMON_NAMESPACE::DateRange{};
                    }
                },
                [&](auto _)
                {
                    return datePartition(_(date),_(model));
                }
                );
            HATN_CTX_SCOPE_PUSH_("partition",r.second)
            r.first=_(handler).partition(r.second);
            return r;
        },
        [&](auto _)
        {
            return std::make_pair(_(handler).defaultPartition(),common::DateRange{});
        }
    );
    return partitionR.first;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBOBJECTPARTITION_IPP
