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

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/common/result.h>

#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN    

struct RocksdbPartition
{
    std::map<std::string,std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle>> collections;
    std::map<std::string,std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle>> indexes;
    std::map<std::string,std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle>> ttlIndexes;

    ROCKSDB_NAMESPACE::ColumnFamilyHandle* collectionCF(const std::string_view& name) const
    {
        auto it=collections.find(name);
        Assert(it!=collections.end(),"collection not found");
        return it->second.get();
    }

    ROCKSDB_NAMESPACE::ColumnFamilyHandle* indexCF(const std::string_view& name) const
    {
        auto it=indexes.find(name);
        Assert(it!=indexes.end(),"index not found");
        return it->second.get();
    }

    ROCKSDB_NAMESPACE::ColumnFamilyHandle* ttlIndexCF(const std::string_view& name) const
    {
        auto it=ttlIndexes.find(name);
        Assert(it!=ttlIndexes.end(),"ttl index not found");
        return it->second.get();
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

        bool readOnly;

        std::map<uint32_t,std::unique_ptr<RocksdbPartition>> partitions;
        std::unique_ptr<RocksdbPartition> defaultPartition;

        Result<RocksdbPartition*> partition(uint32_t partitionKey) const noexcept
        {
            auto it=partitions.find(partitionKey);
            if (it==partitions.end())
            {
                return dbError(DbError::PARTITION_NOT_FOUND);
            }
            return it->second.get();
        }
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBHANDLER_P_H
