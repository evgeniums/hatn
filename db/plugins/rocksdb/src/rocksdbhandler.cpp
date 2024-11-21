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

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbtransaction.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbHandler_p **************************/

//---------------------------------------------------------------

RocksdbHandler_p::RocksdbHandler_p(
        ROCKSDB_NAMESPACE::DB* db,
        ROCKSDB_NAMESPACE::TransactionDB* transactionDb
    ) : db(db),
        transactionDb(transactionDb),
        readOnly(transactionDb==nullptr)
{}

//---------------------------------------------------------------

RocksdbHandler_p::~RocksdbHandler_p()
{
    if (db!=nullptr)
    {
        delete db;
    }
}

/********************** RocksdbHandler **************************/

//---------------------------------------------------------------

RocksdbHandler::RocksdbHandler(RocksdbHandler_p* pimpl):d(pimpl)
{}

//---------------------------------------------------------------

RocksdbHandler::~RocksdbHandler()
{}

//---------------------------------------------------------------

Error RocksdbHandler::transaction(const TransactionFn& fn, Transaction* tx, bool relaxedIfInTransaction)
{
    HATN_CTX_SCOPE("rocksdbtransaction")

    // check for nested transaction
    if (tx!=nullptr)
    {
        if (relaxedIfInTransaction)
        {
            return fn(tx);
        }
        Assert(false,"Nested transactions not allowed");
        return commonError(CommonError::UNSUPPORTED);
    }

    // create rocksdb tx
    RocksdbTransaction rocksdbTx{this};
    if (!rocksdbTx.m_native)
    {
        HATN_CTX_SCOPE_ERROR("begin-transaction")
        return dbError(DbError::TX_BEGIN_FAILED);
    }

    //! @todo Figure out what to do with transaction options
#if 0
    ROCKSDB_NAMESPACE::WriteOptions wopt;
    rocksdbTx.m_native->SetWriteOptions(wopt);
#endif

    // invoke transaction
    auto ec=fn(&rocksdbTx);
    if (ec)
    {
        auto status=rocksdbTx.m_native->Rollback();
        if (!status.ok())
        {
            HATN_CTX_ERROR(makeError(DbError::TX_ROLLBACK_FAILED,status),"");
        }
    }
    else
    {
        auto status=rocksdbTx.m_native->Commit();
        if (!status.ok())
        {
            copyRocksdbError(ec,DbError::TX_COMMIT_FAILED,status);
        }
    }
    return ec;
}

//---------------------------------------------------------------

Result<std::shared_ptr<RocksdbPartition>> RocksdbHandler::createPartition(const common::DateRange& range)
{
    HATN_CTX_SCOPE("rocksdbcreatepartition")
    HATN_CTX_SCOPE_PUSH("partition",range)

    common::lib::unique_lock<common::lib::shared_mutex> l{d->partitionMutex};

    // skip existing partition
    std::shared_ptr<RocksdbPartition> partition;
    auto it=d->partitions.find(range);
    if (it!=d->partitions.end())
    {
        partition=*it;
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
            HATN_CTX_SCOPE_ERROR("collection-column-family")
            return makeError(DbError::PARTITION_CREATE_FALIED,status);
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
            HATN_CTX_SCOPE_ERROR("index-column-family")
            return makeError(DbError::PARTITION_CREATE_FALIED,status);
        }
        if (partition)
        {
            partition->indexCf.reset(indexCf);
        }
    }

    if (!partition || !partition->ttlCf)
    {
        auto status=d->transactionDb->CreateColumnFamily(d->ttlColumnFamilyOptions,
                                                                    RocksdbPartition::columnFamilyName(RocksdbPartition::CfType::Ttl,range),
                                                                    &ttlCf
                                                                    );
        if (!status.ok())
        {
            HATN_CTX_SCOPE_ERROR("ttl-column-family")
            return makeError(DbError::PARTITION_CREATE_FALIED,status);
        }
        if (partition)
        {
            partition->ttlCf.reset(ttlCf);
        }
    }

    // keep partition
    if (!partition)
    {
        partition=std::make_shared<RocksdbPartition>(collectionCf,indexCf,ttlCf,range);
        d->partitions.insert(std::move(partition));
    }

    return partition;
}

//---------------------------------------------------------------

Error RocksdbHandler::deletePartition(const common::DateRange& range)
{
    HATN_CTX_SCOPE("rocksdbdeletepartition")
    HATN_CTX_SCOPE_PUSH("partition",range)

    common::lib::unique_lock<common::lib::shared_mutex> l{d->partitionMutex};

    // skip existing partition
    auto it=d->partitions.find(range);
    if (it==d->partitions.end())
    {
        return OK;
    }
    auto partition=*it;

    // drop column families
    if (partition->indexCf)
    {
        auto status=d->transactionDb->DropColumnFamily(partition->indexCf.get());
        if (!status.ok())
        {
            //! @todo skip non-existent cf

            HATN_CTX_SCOPE_ERROR("index-column-family")
            return makeError(DbError::PARTITION_DELETE_FALIED,status);
        }
    }

    if (partition->collectionCf)
    {
        auto status=d->transactionDb->DropColumnFamily(partition->collectionCf.get());
        if (!status.ok())
        {
            //! @todo skip non-existent cf

            HATN_CTX_SCOPE_ERROR("collection-column-family")
            return makeError(DbError::PARTITION_DELETE_FALIED,status);
        }
    }

    if (partition->ttlCf)
    {
        auto status=d->transactionDb->DropColumnFamily(partition->ttlCf.get());
        if (!status.ok())
        {
            //! @todo skip non-existent cf
            //!
            HATN_CTX_SCOPE_ERROR("ttl-column-family")
            return makeError(DbError::PARTITION_DELETE_FALIED,status);
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
        return *it;
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
    d->partitions.insert(std::move(partition));
}

//---------------------------------------------------------------

bool RocksdbHandler::readOnly() const noexcept
{
    return d->readOnly;
}

//---------------------------------------------------------------

void RocksdbHandler::resetCf()
{
    d->partitions.clear();
    d->defaultCf.reset();
    d->defaultPartition.reset();
}

//---------------------------------------------------------------

void RocksdbHandler::setSchema(std::shared_ptr<RocksdbSchema> schema) noexcept
{
    d->schema=std::move(schema);
}

//---------------------------------------------------------------

std::shared_ptr<RocksdbSchema> RocksdbHandler::schema() const noexcept
{
    return d->schema;
}

//---------------------------------------------------------------

Error RocksdbHandler::ensureModelSchema(const ModelInfo &model) const
{
    if (!d->schema)
    {
        return dbError(DbError::SCHEMA_NOT_FOUND);
    }
    auto m=d->schema->findModel(model);
    if (!m)
    {
        return dbError(DbError::MODEL_NOT_FOUND);
    }
    return OK;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
