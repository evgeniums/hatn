/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/savesingleindex.ipp
  *
  *   RocksDB single index saving.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBSAVESINGLEINDEX_H
#define HATNROCKSDBSAVESINGLEINDEX_H

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbkeys.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

Error HATN_ROCKSDB_SCHEMA_EXPORT SaveSingleIndex(
        const IndexKeySlice& key,
        bool unique,
        ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf,
        ROCKSDB_NAMESPACE::Transaction* tx,
        const ROCKSDB_NAMESPACE::SliceParts& indexValue,
        bool replace=false
    );

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSAVESINGLEINDEX_H
