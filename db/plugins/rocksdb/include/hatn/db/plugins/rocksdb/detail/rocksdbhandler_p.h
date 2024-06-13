/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbhandler_p.h
  *
  *   RocksDB database handler pimpl header.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBHANDLER_P_H
#define HATNROCKSDBHANDLER_P_H

#include <map>
#include <map>

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/common/result.h>
#include <hatn/common/datetime.h>
#include <hatn/common/stdwrappers.h>

#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN    

struct RocksdbPartition
{
    enum class CfType : int
    {
        Collection,
        Index,
        Ttl
    };

    RocksdbPartition(
            ROCKSDB_NAMESPACE::ColumnFamilyHandle* collectionCf,
            ROCKSDB_NAMESPACE::ColumnFamilyHandle* indexCf,
            ROCKSDB_NAMESPACE::ColumnFamilyHandle* ttlCf
        ) :
            collectionCf(std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle>{collectionCf}),
            indexCf(std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle>{indexCf}),
            ttlCf(std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle>{ttlCf})
    {}

    std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle> collectionCf;
    std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle> indexCf;
    std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle> ttlCf;

    static std::string columnFamilyName(CfType cfType, const common::DateRange& range)
    {
        switch (cfType)
        {
            case (CfType::Collection):
                return fmt::format("{}_collection",range.toString());
                break;

            case (CfType::Index):
                return fmt::format("{}_index",range.toString());
                break;

            case (CfType::Ttl):
                return fmt::format("{}_ttl",range.toString());
                break;
        }

        Assert(false,"Unknown column family type");
        return std::string{};
    }
};

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbHandler_p
{
    public:

        RocksdbHandler_p(ROCKSDB_NAMESPACE::DB* db, ROCKSDB_NAMESPACE::TransactionDB* transactionDb=nullptr);

        ROCKSDB_NAMESPACE::DB* db;
        ROCKSDB_NAMESPACE::TransactionDB* transactionDb;

        ROCKSDB_NAMESPACE::WriteOptions writeOptions;
        ROCKSDB_NAMESPACE::ReadOptions readOptions;
        ROCKSDB_NAMESPACE::TransactionOptions transactionOptions;

        ROCKSDB_NAMESPACE::ColumnFamilyOptions collColumnFamilyOptions;
        ROCKSDB_NAMESPACE::ColumnFamilyOptions indexColumnFamilyOptions;
        ROCKSDB_NAMESPACE::ColumnFamilyOptions ttlColumnFamilyOptions;

        bool readOnly;

        std::map<common::DateRange,std::shared_ptr<RocksdbPartition>> partitions;
        std::unique_ptr<RocksdbPartition> defaultPartition;

        bool inTransaction;
        mutable common::lib::shared_mutex partitionMutex;

        Result<std::shared_ptr<RocksdbPartition>> partition(uint32_t partitionKey) const noexcept
        {
            common::lib::shared_lock<common::lib::shared_mutex> l{partitionMutex};

            auto it=partitions.find(partitionKey);
            if (it==partitions.end())
            {
                return dbError(DbError::PARTITION_NOT_FOUND);
            }
            return it->second;
        }
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBHANDLER_P_H
