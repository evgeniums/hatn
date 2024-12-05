/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ttlcompactionfilter.h
  *
  *   RocksDB TTL compaction filter.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBCOMPACTIONFILTER_H
#define HATNROCKSDBCOMPACTIONFILTER_H

#include <string>

#include <rocksdb/compaction_filter.h>

#include <hatn/db/plugins/rocksdb/rocksdbdriver.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class HATN_ROCKSDB_EXPORT TtlCompactionFilter : public ROCKSDB_NAMESPACE::CompactionFilter
{
    public:

        using ROCKSDB_NAMESPACE::CompactionFilter::CompactionFilter;

        bool Filter(int /*level*/, const ROCKSDB_NAMESPACE::Slice& /*key*/,
                const ROCKSDB_NAMESPACE::Slice& existing_value,
                std::string* /*new_value*/,
                bool* /*value_changed*/) const override;

        const char* Name() const override
        {
            return "TtlCompactionFilter";
        }
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCOMPACTIONFILTER_H
