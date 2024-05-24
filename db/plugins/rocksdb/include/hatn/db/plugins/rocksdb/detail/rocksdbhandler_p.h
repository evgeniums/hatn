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

#include <memory>

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

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
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBHANDLER_P_H
