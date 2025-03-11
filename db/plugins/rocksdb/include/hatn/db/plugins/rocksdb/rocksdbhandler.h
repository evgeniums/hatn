/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbhandler.h
  *
  *   RocksDB database handler header.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBHANDLER_H
#define HATNROCKSDBHANDLER_H

#include <memory>

#include <hatn/common/datetime.h>

#include <hatn/db/dberror.h>
#include <hatn/db/model.h>
#include <hatn/db/transaction.h>
#include <hatn/db/topic.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class RocksdbHandler_p;
struct RocksdbPartition;
class RocksdbSchema;

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbHandler
{
    public:

        RocksdbHandler(RocksdbHandler_p* pimpl);
        ~RocksdbHandler();

        RocksdbHandler(const RocksdbHandler&)=delete;
        RocksdbHandler& operator=(const RocksdbHandler&)=delete;
        RocksdbHandler(RocksdbHandler&&)=default;
        RocksdbHandler& operator=(RocksdbHandler&&)=default;

        RocksdbHandler_p* p() noexcept
        {
            return d.get();
        }

        Error transaction(const TransactionFn& fn, Transaction* tx=nullptr, bool relaxedIfInTransaction=false);

        Result<std::shared_ptr<RocksdbPartition>> createPartition(const common::DateRange& range=common::DateRange{});

        Error deletePartition(const common::DateRange& range);

        std::shared_ptr<RocksdbPartition> partition(const common::DateRange& range) const noexcept;
        void insertPartition(const std::shared_ptr<RocksdbPartition>& partition);

        std::shared_ptr<RocksdbPartition> defaultPartition() const noexcept;

        bool readOnly() const noexcept;

        void resetCf();

        void setSchema(std::shared_ptr<RocksdbSchema> schema) noexcept;

        std::shared_ptr<RocksdbSchema> schema() const noexcept;

        Error ensureModelSchema(const ModelInfo &model) const;

        Error deleteTopic(Topic topic);

        common::FlatSet<common::DateRange> partitionRanges() const;

    private:

        std::unique_ptr<RocksdbHandler_p> d;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBHANDLER_H
