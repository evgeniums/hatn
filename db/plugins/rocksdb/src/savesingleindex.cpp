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
#include <hatn/db/plugins/rocksdb/ttlmark.h>
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
        ROCKSDB_NAMESPACE::ColumnFamilyHandle* dataCf,
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

            // The uniqueness probe below deliberately provokes SaveUniqueKey::Merge() to fail
            // when the key already exists (that failure IS how a duplicate is detected). Without
            // a save point, the failing merge operand stays queued in the transaction's write
            // batch: if the caller treats DUPLICATE_UNIQUE_KEY as recoverable and goes on to
            // commit this same transaction (a common and intentional pattern -- catch the
            // duplicate, findOneForUpdate + update instead, return OK), that poisoned operand is
            // written into the memtable/SST. From then on the merge operator runs over
            // (existing value + operand) on every read of that key and fails permanently
            // ("Corruption: Merge operator failed"), eventually latching a RocksDB background
            // error that takes the whole database read-only. The save point guarantees the probe
            // never leaves a trace beyond this function, regardless of what the caller does with
            // the transaction afterwards.
            tx->SetSavePoint();
            status=tx->Merge(cf,k,v);
            if (status.ok())
            {
                auto ropt=handler.p()->readOptions;
                ROCKSDB_NAMESPACE::PinnableSlice sl;
                status=tx->Get(ropt,cf,k,&sl);
            }
            if (!status.ok())
            {
#if 0
                std::cout<<"Unique index " << logKey(k)  << " failed to merge" << std::endl;
#endif
                auto rollbackStatus=tx->RollbackToSavePoint();
                if (!rollbackStatus.ok())
                {
                    HATN_CTX_ERROR_RECORDS(makeError(DbError::SAVE_INDEX_FAILED,rollbackStatus),
                        "failed to rollback poisoned unique index merge probe",{"dupl_key",logKey(k)})
                }

                if (status.subcode()==ROCKSDB_NAMESPACE::Status::SubCode::kMergeOperatorFailed)
                {
                    // The existing entry is a real duplicate only if the object it points to is
                    // still alive. An entry left behind by a deleted or expired object (a stale
                    // entry of an old update, see IndexKeyUpdate::deleteKeySlice()) would block
                    // its value forever, so take it over.
                    if (dataCf!=nullptr)
                    {
                        auto ropt=handler.p()->readOptions;
                        ROCKSDB_NAMESPACE::PinnableSlice existing;
                        auto getStatus=tx->GetForUpdate(ropt,cf,k,&existing);
                        if (getStatus.ok())
                        {
                            auto objectKey=Keys::objectKeyFromIndexValue(existing.data(),existing.size());
                            bool dangling=objectKey.empty();
                            if (!dangling)
                            {
                                ROCKSDB_NAMESPACE::PinnableSlice objectValue;
                                auto objStatus=tx->Get(ropt,dataCf,objectKey,&objectValue);
                                if (objStatus.IsNotFound())
                                {
                                    dangling=true;
                                }
                                else if (objStatus.ok())
                                {
                                    TtlMark::refreshCurrentTimepoint();
                                    dangling=TtlMark::isExpired(objectValue);
                                }
                            }
                            if (dangling)
                            {
                                HATN_CTX_WARN_RECORDS("replaced dangling unique index entry",{"dupl_key",logKey(k)})
                                ROCKSDB_NAMESPACE::SliceParts keySlices{&key[0],static_cast<int>(key.size()-1)};
                                status=tx->Put(cf,keySlices,indexValue);
                                if (!status.ok())
                                {
                                    return makeError(DbError::SAVE_INDEX_FAILED,status);
                                }
                                return Error{OK};
                            }
                        }
                    }

                    auto ec=dbError(DbError::DUPLICATE_UNIQUE_KEY);
                    HATN_CTX_ERROR_RECORDS(ec,"",{"dupl_key",logKey(k)})
                    return ec;
                }

                return makeError(DbError::SAVE_INDEX_FAILED,status);
            }
            tx->PopSavePoint();
        }
    }
    if (put)
    {
        // A unique entry lives under slice[0] alone (see IndexKeyUpdate::deleteKeySlice()), also
        // when it is rewritten with a new TTL mark: writing both slices would leave the real
        // entry with the old mark and add a stray key that nothing ever deletes.
        auto slicesCount=(unique && replace)?key.size()-1:key.size();
        ROCKSDB_NAMESPACE::SliceParts keySlices{&key[0],static_cast<int>(slicesCount)};

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
