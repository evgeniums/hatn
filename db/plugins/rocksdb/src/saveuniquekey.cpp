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
#include <hatn/db/plugins/rocksdb/saveuniquekey.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace {
static thread_local Error Ec;
}

//---------------------------------------------------------------

const Error& RocksdbOpError::ec()
{
    return Ec;
}

//---------------------------------------------------------------

void RocksdbOpError::setEc(const Error& ec)
{
    Ec=ec;
}

//---------------------------------------------------------------

void RocksdbOpError::resetEc()
{
    Ec.reset();
}

//---------------------------------------------------------------

bool SaveUniqueKey::Merge(
    const ROCKSDB_NAMESPACE::Slice& key,
    const ROCKSDB_NAMESPACE::Slice* existing_value,
    const ROCKSDB_NAMESPACE::Slice& value,
    std::string* new_value,
    ROCKSDB_NAMESPACE::Logger*) const
{
    RocksdbOpError::resetEc();
    if (existing_value!=nullptr && !TtlMark::isExpired(existing_value))
    {        
        HATN_CTX_SCOPE("saveuniquekey")
        auto k=common::lib::string_view{key.data(),key.size()};
        HATN_CTX_SCOPE_PUSH("unique_key",k);
        HATN_CTX_SCOPE_ERROR("duplicate-key");
        RocksdbOpError::setEc(dbError(DbError::DUPLICATE_UNIQUE_KEY));
        return false;
    }

    *new_value=std::string{value.data(),value.size()};
    return true;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
