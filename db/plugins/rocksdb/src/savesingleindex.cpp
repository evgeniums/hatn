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

#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

Error HATN_ROCKSDB_SCHEMA_EXPORT SaveSingleIndex(
        RocksdbHandler& handler,
        const IndexKeySlice& key,
        bool unique,
        ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf,
        ROCKSDB_NAMESPACE::Transaction* tx,
        const ROCKSDB_NAMESPACE::SliceParts& indexValue,
        bool replace
    )
{
    ROCKSDB_NAMESPACE::Status status;

    // put index to transaction
    bool put=true;
    if (unique)
    {
        if (!replace)
        {
            ROCKSDB_NAMESPACE::SliceParts keySlices{&key[0],static_cast<int>(key.size()-1)};
            put=false;
            std::string bufk;
            ROCKSDB_NAMESPACE::Slice k{keySlices,&bufk};

//! @maybe Log debug
#if 0
            std::cout<<"Unique index " << logKey(k) << std::endl;
#endif
            std::string bufv;
            ROCKSDB_NAMESPACE::Slice v{indexValue,&bufv};
            status=tx->Merge(cf,k,v);
            if (status.ok())
            {
                auto ropt=handler.p()->readOptions;
                ROCKSDB_NAMESPACE::PinnableSlice sl;
                status=tx->Get(ropt,cf,k,&sl);
            }
            if (!status.ok())
            {
                if (status.subcode()==ROCKSDB_NAMESPACE::Status::SubCode::kMergeOperatorFailed)
                {
                    //! @todo Create native error with id/name of duplicate key
                    return dbError(DbError::DUPLICATE_UNIQUE_KEY);
                }

                return makeError(DbError::SAVE_INDEX_FAILED,status);
            }
        }
    }
    if (put)
    {
        ROCKSDB_NAMESPACE::SliceParts keySlices{&key[0],static_cast<int>(key.size())};

        std::string bufk;
        ROCKSDB_NAMESPACE::Slice k{keySlices,&bufk};
//! @maybe Log debug
#if 0
        std::cout<<"Not unique index or replace " << logKey(k) << std::endl;
#endif
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
