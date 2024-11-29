/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file db/plugins/rocksdb/src/ttlcompactionfilter.cpp
  *
  *   RocksDB TTL compaction filter.
  *
  */

/****************************************************************************/

#include <chrono>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/rocksdbkeys.h>
#include <hatn/db/plugins/rocksdb/ttlcompactionfilter.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

//---------------------------------------------------------------

bool TtlCompactionFilter::Filter(int /*level*/, const ROCKSDB_NAMESPACE::Slice& key,
                                 const ROCKSDB_NAMESPACE::Slice& existing_value,
                                 std::string* /*new_value*/,
                                 bool* /*value_changed*/) const
{    
    auto tp=static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    bool expired=TtlMark::isExpired(existing_value,tp);
#if 0
    std::cout << "TtlCompactionFilter::Filter key=" << logKey(key) << " tp="
              << fmt::format("{:016x}",static_cast<uint64_t>(tp)*uint64_t(1000)) << " ms=" <<
                 fmt::format("{:016x}",common::DateTime::millisecondsSinceEpoch()) << " expired=" << expired << std::endl;
#endif
    return expired;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
