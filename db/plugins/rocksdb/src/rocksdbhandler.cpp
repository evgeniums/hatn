/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbhandler.cpp
  *
  *   RocksDB database handler source.
  *
  */

/****************************************************************************/

#include <rocksdb/db.h>

#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler_p.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbHandler_p **************************/

//---------------------------------------------------------------

RocksdbHandler_p::RocksdbHandler_p(ROCKSDB_NAMESPACE::DB* db, ROCKSDB_NAMESPACE::TransactionDB* transactionDb)
    : db(db),
    transactionDb(transactionDb),
    readOnly(transactionDb==nullptr),
    inTransaction(false)
{}

/********************** RocksdbHandler **************************/

//---------------------------------------------------------------

RocksdbHandler::RocksdbHandler(RocksdbHandler_p* pimpl):d(pimpl)
{}

//---------------------------------------------------------------

RocksdbHandler::~RocksdbHandler()
{}

//---------------------------------------------------------------

Error RocksdbHandler::transaction(const TransactionFn& fn, bool relaxedIfInTransaction)
{
    if (d->inTransaction)
    {
        if (relaxedIfInTransaction)
        {
            return fn();
        }
        Assert(false,"Nested transactions not allowed");
        return commonError(CommonError::UNSUPPORTED);
    }

    auto tx=d->transactionDb->BeginTransaction(d->writeOptions,d->transactionOptions);
    if (tx==nullptr)
    {
        return dbError(DbError::TX_BEGIN_FAILED);
    }
    d->inTransaction=true;
    auto ec=fn();
    d->inTransaction=false;
    if (ec)
    {
        auto status=tx->Rollback();
        if (!status.ok())
        {
            //! @todo report error
        }
    }
    else
    {
        auto status=tx->Commit();
        if (!status.ok())
        {
            //! @todo construct error
            ec=dbError(DbError::TX_COMMIT_FAILED);
        }
    }
    return ec;
}

//---------------------------------------------------------------

Result<std::shared_ptr<RocksdbPartition>> RocksdbHandler::createPartition(const common::DateRange& range)
{
    common::lib::unique_lock<common::lib::shared_mutex> l{d->partitionMutex};

    // skip existing partition
    std::shared_ptr<RocksdbPartition> partition;
    auto it=d->partitions.find(range);
    if (it!=d->partitions.end())
    {
        partition=it->second;
        if (partition->collectionCf && partition->indexCf && partition->ttlCf)
        {
            return partition;
        }
    }

    // create column families

    ROCKSDB_NAMESPACE::ColumnFamilyHandle* collectionCf;
    ROCKSDB_NAMESPACE::ColumnFamilyHandle* indexCf;
    ROCKSDB_NAMESPACE::ColumnFamilyHandle* ttlCf;

    if (!partition || !partition->collectionCf)
    {
        auto status=d->transactionDb->CreateColumnFamily(d->collColumnFamilyOptions,
                                                                         RocksdbPartition::columnFamilyName(RocksdbPartition::CfType::Collection,range),
                                                                         &collectionCf
                                                                         );
        if (!status.ok())
        {
            //! @todo handle error
            return dbError(DbError::PARTITION_CREATE_FALIED);
        }
        if (partition)
        {
            partition->collectionCf.reset(collectionCf);
        }
    }

    if (!partition || !partition->indexCf)
    {
        auto status=d->transactionDb->CreateColumnFamily(d->indexColumnFamilyOptions,
                                                                    RocksdbPartition::columnFamilyName(RocksdbPartition::CfType::Index,range),
                                                                    &indexCf
                                                                    );
        if (!status.ok())
        {
            //! @todo handle error
            return dbError(DbError::PARTITION_CREATE_FALIED);
        }
        if (partition)
        {
            partition->indexCf.reset(indexCf);
        }
    }

    if (!partition || !partition->collectionCf)
    {
        auto status=d->transactionDb->CreateColumnFamily(d->ttlColumnFamilyOptions,
                                                                    RocksdbPartition::columnFamilyName(RocksdbPartition::CfType::Ttl,range),
                                                                    &ttlCf
                                                                    );
        if (!status.ok())
        {
            //! @todo handle error
            return dbError(DbError::PARTITION_CREATE_FALIED);
        }
        if (partition)
        {
            partition->ttlCf.reset(ttlCf);
        }
    }

    // keep partition
    if (!partition)
    {
        partition=std::make_shared<RocksdbPartition>(collectionCf,indexCf,ttlCf);
        d->partitions.emplace(range,std::move(partition));
    }

    return partition;
}

//---------------------------------------------------------------

Error RocksdbHandler::deletePartition(const common::DateRange& range)
{
    common::lib::unique_lock<common::lib::shared_mutex> l{d->partitionMutex};

    // skip existing partition
    auto it=d->partitions.find(range);
    if (it==d->partitions.end())
    {
        return OK;
    }
    auto partition=it->second;

    // drop column families
    if (partition->indexCf)
    {
        auto status=d->transactionDb->DropColumnFamily(partition->indexCf.get());
        if (!status.ok())
        {
            //! @todo handle error, skip non-existent cf
            return dbError(DbError::PARTITION_DELETE_FALIED);
        }
    }

    if (partition->collectionCf)
    {
        auto status=d->transactionDb->DropColumnFamily(partition->collectionCf.get());
        if (!status.ok())
        {
            //! @todo handle error, skip non-existent cf
            return dbError(DbError::PARTITION_DELETE_FALIED);
        }
    }

    if (partition->ttlCf)
    {
        auto status=d->transactionDb->DropColumnFamily(partition->ttlCf.get());
        if (!status.ok())
        {
            //! @todo handle error, skip non-existent cf
            return dbError(DbError::PARTITION_DELETE_FALIED);
        }
    }

    // delete partition
    d->partitions.erase(it);

    // done
    return OK;
}

//---------------------------------------------------------------

std::shared_ptr<RocksdbPartition> RocksdbHandler::partition(const common::DateRange& range) const noexcept
{
    common::lib::shared_lock<common::lib::shared_mutex> l{d->partitionMutex};

    auto it=d->partitions.find(range);
    if (it!=d->partitions.end())
    {
        return it->second;
    }
    return std::shared_ptr<RocksdbPartition>{};
}

//---------------------------------------------------------------

std::shared_ptr<RocksdbPartition> RocksdbHandler::defaultPartition() const noexcept
{
    return d->defaultPartition;
}

//---------------------------------------------------------------

void RocksdbHandler::insertPartition(const common::DateRange &range, std::shared_ptr<RocksdbPartition> partition)
{
    common::lib::unique_lock<common::lib::shared_mutex> l{d->partitionMutex};
    d->partitions.emplace(range,std::move(partition));
}

//---------------------------------------------------------------

bool RocksdbHandler::readOnly() const noexcept
{
    return d->readOnly;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
