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
std::shared_ptr<RocksdbPartition> objectPartition(
        RocksdbHandler& handler,
        const ModelT& model,
        const ObjectDateT& objectDateId,
        const DateT& date=DateT{}
    )
{
    using modelType=std::decay_t<ModelT>;

    if constexpr (modelType::isDatePartitioned())
    {
        common::DateRange range;

        // check if date is hana::false_
        using dateT=std::decay_t<DateT>;
        if constexpr (std::is_same<dateT,hana::false_>::value)
        {
            range=datePartition(objectDateId,model);
        }
        else
        {
            range=datePartition(date,model);
        }

        // return partition for range
        return handler.partition(range);
    }
    else
    {
        return handler.defaultPartition();
    }
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBOBJECTPARTITION_IPP
