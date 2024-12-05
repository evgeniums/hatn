/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file db/plugins/rocksdb/saveuniquekey.cpp
  *
  *   RocksDB merge operator for unique keys.
  *
  */

/****************************************************************************/

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/rocksdboperror.h>
#include <hatn/db/plugins/rocksdb/saveuniquekey.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

//---------------------------------------------------------------

bool SaveUniqueKey::Merge(
    const ROCKSDB_NAMESPACE::Slice& key,
    const ROCKSDB_NAMESPACE::Slice* existing_value,
    const ROCKSDB_NAMESPACE::Slice& value,
    std::string* new_value,
    ROCKSDB_NAMESPACE::Logger*) const
{
    if (existing_value!=nullptr && !TtlMark::isExpired(*existing_value))
    {
        return false;
    }
    *new_value=std::string{value.data(),value.size()};
    return true;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
