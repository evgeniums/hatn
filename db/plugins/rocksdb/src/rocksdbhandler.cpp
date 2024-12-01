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
    HATN_CTX_SCOPE("createpartition")
    HATN_CTX_SCOPE_PUSH("partition",range)

    if (range.isNull())
    {
        HATN_CTX_INFO("create default partition")
    }
    else
    {
        HATN_CTX_INFO("create date partition")
    }

    common::lib::unique_lock<common::lib::shared_mutex> l{d->partitionMutex};

    // skip existing partition
    std::shared_ptr<RocksdbPartition> partition;
    if (!range.isNull())
    {
        auto it=d->partitions.find(range);
        if (it!=d->partitions.end())
        {
            partition=*it;
            if (partition->collectionCf && partition->indexCf && partition->ttlCf)
            {
                HATN_CTX_DEBUG_RECORDS("date partition exists",{"partition",range.toString()});
                return partition;
            }
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
    if (!partition && !range.isNull())
    {
        partition=std::make_shared<RocksdbPartition>(collectionCf,indexCf,ttlCf,range);
        d->partitions.insert(std::move(partition));
    }

    // done
    return partition;
}

//---------------------------------------------------------------

Error RocksdbHandler::deletePartition(const common::DateRange& range)
{
    HATN_CTX_SCOPE("deletepartition")
    HATN_CTX_SCOPE_PUSH("partition",range)

    if (range.isNull())
    {
        HATN_CTX_INFO("delete default partition")
    }
    else
    {
        HATN_CTX_INFO("delete date partition")
    }

    common::lib::unique_lock<common::lib::shared_mutex> l{d->partitionMutex};

    // skip existing partition
    auto it=d->partitions.find(range);
    if (it==d->partitions.end())
    {
        return OK;
    }
    auto partition=*it;

    //! @todo Drop column families in destructor for multithreaded support

    // drop column families
    if (partition->indexCf)
    {
        auto status=d->transactionDb->DropColumnFamily(partition->indexCf.get());
        if (!status.ok())
        {
            HATN_CTX_SCOPE_ERROR("index-column-family")
            return makeError(DbError::PARTITION_DELETE_FALIED,status);
        }
    }

    if (partition->collectionCf)
    {
        auto status=d->transactionDb->DropColumnFamily(partition->collectionCf.get());
        if (!status.ok())
        {
            HATN_CTX_SCOPE_ERROR("collection-column-family")
            return makeError(DbError::PARTITION_DELETE_FALIED,status);
        }
    }

    if (partition->ttlCf)
    {
        auto status=d->transactionDb->DropColumnFamily(partition->ttlCf.get());
        if (!status.ok())
        {
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

void RocksdbHandler::insertPartition(const std::shared_ptr<RocksdbPartition>& partition)
{
    if (!partition->range.isNull())
    {
        common::lib::unique_lock<common::lib::shared_mutex> l{d->partitionMutex};
        d->partitions.insert(partition);
    }
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

Error RocksdbHandler::deleteTopic(const Topic& topic)
{
    KeyBuf startB;
    startB.append(topic.topic());
    startB.append(SeparatorCharStr);
    ROCKSDB_NAMESPACE::Slice start{startB.data(),startB.size()};
    KeyBuf stopB;
    stopB.append(topic.topic());
    stopB.append(SeparatorCharPlusStr);
    ROCKSDB_NAMESPACE::Slice stop{stopB.data(),stopB.size()};

    //! @todo Refactor it when TransactionDb supports DeleteRange
    //! @note Lock any access to a topic when delete topic is called

    ROCKSDB_NAMESPACE::WriteBatch batch;

    // handler to delete from column family using writ batch
    auto deleteFromCf=[&batch,&start,&stop,this](ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf)
    {
        auto status=batch.DeleteRange(
                           cf,
                           start,stop);
        if (!status.ok())
        {
            return makeError(DbError::DELETE_TOPIC_FAILED,status);
        }
        return Error{OK};
    };

    // handler to delete from partition
    auto deleteFromPartition=[&deleteFromCf](RocksdbPartition* partition)
    {
        auto ec=deleteFromCf(partition->collectionCf.get());
        HATN_CHECK_EC(ec)
        ec=deleteFromCf(partition->indexCf.get());
        HATN_CHECK_EC(ec)

        return Error{OK};
    };

    // delete from default partiton
    auto ec=deleteFromPartition(d->defaultPartition.get());
    HATN_CHECK_EC(ec)
    {
        // delete from date partitons
        common::lib::shared_lock<common::lib::shared_mutex> l{d->partitionMutex};
        for (auto&& partition: d->partitions)
        {
            ec=deleteFromPartition(partition.get());
            HATN_CHECK_EC(ec)
        }
    }

    // send write batch to db
    ROCKSDB_NAMESPACE::TransactionDBWriteOptimizations txOpts;
    txOpts.skip_concurrency_control=true;
    txOpts.skip_duplicate_key_check=true;
    auto opts=d->writeOptions;
    auto status=d->transactionDb->Write(opts,txOpts,&batch);
    if (!status.ok())
    {
        return makeError(DbError::DELETE_TOPIC_FAILED,status);
    }

    // done
    return OK;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
