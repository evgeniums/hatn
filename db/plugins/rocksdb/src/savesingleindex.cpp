/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/sacesingleindex.cpp
  *
  *   RocksDB save single index.
  *
  */

/****************************************************************************/

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/savesingleindex.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

Error HATN_ROCKSDB_SCHEMA_EXPORT SaveSingleIndex(
        const IndexKeySlice& key,
        bool unique,
        ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf,
        ROCKSDB_NAMESPACE::Transaction* tx,
        const ROCKSDB_NAMESPACE::SliceParts& indexValue,
        bool replace
    )
{
    ROCKSDB_NAMESPACE::Status status;
    ROCKSDB_NAMESPACE::SliceParts keySlices{&key[0],static_cast<int>(key.size())};

    // put index to transaction
    bool put=true;
    if (unique)
    {
        if (!replace)
        {
            HATN_CTX_SCOPE_PUSH("idx_op","merge");
            put=false;
            std::string bufk;
            ROCKSDB_NAMESPACE::Slice k{keySlices,&bufk};
            std::string bufv;
            ROCKSDB_NAMESPACE::Slice v{indexValue,&bufv};
            status=tx->Merge(cf,k,v);
        }
    }
    if (put)
    {
        HATN_CTX_SCOPE_PUSH("idx_op","put");
        status=tx->Put(cf,keySlices,indexValue);
    }
    if (!status.ok())
    {
        return makeError(DbError::SAVE_INDEX_FAILED,status);
    }

    // done
    return Error{OK};
}

HATN_ROCKSDB_NAMESPACE_END
