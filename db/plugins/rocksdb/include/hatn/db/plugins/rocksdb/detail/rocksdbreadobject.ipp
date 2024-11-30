/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbreadobject.ipp
  *
  *   RocksDB database template for reading single object by ID.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBREADOBJECT_IPP
#define HATNROCKSDBREADOBJECT_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/topic.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/objectpartition.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbtransaction.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename ModelT>
Result<typename ModelT::SharedPtr> readSingleObject(
    const ModelT&,
    RocksdbHandler& handler,
    RocksdbPartition* partition,
    const ROCKSDB_NAMESPACE::Slice& key,
    AllocatorFactory* factory,
    Transaction* tx,
    bool forUpdate
)
{
    using modelType=std::decay_t<ModelT>;

    // read object from db
    auto rdb=handler.p()->db;
    ROCKSDB_NAMESPACE::PinnableSlice readSlice;
    ROCKSDB_NAMESPACE::Status status;
    if (tx==nullptr)
    {
        // read directly from db
        status=rdb->Get(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
    }
    else
    {
        // read from transaction
        auto rdbTx=RocksdbTransaction::native(tx);
        if (forUpdate)
        {
            status=rdbTx->GetForUpdate(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
        }
        else
        {
            status=rdbTx->Get(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
        }
    }

    // check status
    if (!status.ok())
    {
        if (status.code()==ROCKSDB_NAMESPACE::Status::kNotFound)
        {
            return dbError(DbError::NOT_FOUND);
        }

        HATN_CTX_SCOPE_ERROR("get");
        return makeError(DbError::READ_FAILED,status);
    }

    // check if object expired
    TtlMark::refreshCurrentTimepoint();
    if (TtlMark::isExpired(readSlice))
    {
        return dbError(DbError::EXPIRED);
    }

    // create object
    auto obj=factory->createObject<typename modelType::ManagedType>(factory);

    // deserialize object
    auto objSlice=TtlMark::stripTtlMark(readSlice);
    dataunit::WireBufSolid buf{objSlice.data(),objSlice.size(),true};
    Error ec;
    if (!dataunit::io::deserialize(*obj,buf,ec))
    {
        HATN_CTX_SCOPE_ERROR("deserialize");
        return ec;
    }

    // done
    return obj;
}


struct ReadObjectT
{
    template <typename ModelT, typename DateT>
    Result<typename ModelT::SharedPtr> operator ()(const ModelT& model,
                                                  RocksdbHandler& handler,
                                                  const Topic& topic,
                                                  const ObjectId& objectId,
                                                  const DateT& date,
                                                  AllocatorFactory* allocatorFactory,
                                                  Transaction* tx,
                                                  bool forUpdate
                                                  ) const;
};
constexpr ReadObjectT ReadObject{};

template <typename ModelT, typename DateT>
Result<typename ModelT::SharedPtr> ReadObjectT::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const Topic& topic,
        const ObjectId& objectId,
        const DateT& date,
        AllocatorFactory* factory,
        Transaction* tx,
        bool forUpdate
    ) const
{
    HATN_CTX_SCOPE("read")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())
    HATN_CTX_SCOPE_PUSH("topic",topic.topic())
    auto idData=objectId.toArray();
    auto idDataStr=lib::string_view{idData.data(),idData.size()};
    HATN_CTX_SCOPE_PUSH("oid",idDataStr)

    // eval partition
    const auto partition=objectPartition(handler,model,objectId,date);
    if (!partition)
    {
        HATN_CTX_SCOPE_ERROR("find-partition");
        return dbError(DbError::PARTITION_NOT_FOUND);
    }
    if (!partition->range.isNull())
    {
        HATN_CTX_SCOPE_PUSH("partition",partition->range)
    }
    HATN_CTX_DEBUG("")

    // construct key
    ROCKSDB_NAMESPACE::Slice objectIdS{idData.data(),idData.size()};
    auto [objKeyVal,_]=Keys::makeObjectKeyValue(model.modelIdStr(),topic,objectIdS);
    KeyBuf keyBuf;
    auto key=Keys::objectKeySolid(keyBuf,objKeyVal);

#if 0
    // read object from db
    auto rdb=handler.p()->db;
    ROCKSDB_NAMESPACE::PinnableSlice readSlice;
    ROCKSDB_NAMESPACE::Status status;
    if (tx==nullptr)
    {
        // read directly from db
        status=rdb->Get(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
    }
    else
    {
        // read from transaction
        auto rdbTx=RocksdbTransaction::native(tx);
        if (forUpdate)
        {
            status=rdbTx->GetForUpdate(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
        }
        else
        {
            status=rdbTx->Get(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
        }
    }

    // check status
    if (!status.ok())
    {
        if (status.code()==ROCKSDB_NAMESPACE::Status::kNotFound)
        {
            return dbError(DbError::NOT_FOUND);
        }

        HATN_CTX_SCOPE_ERROR("get");
        return makeError(DbError::READ_FAILED,status);
    }

    // check if object expired
    TtlMark::refreshCurrentTimepoint();
    if (TtlMark::isExpired(readSlice))
    {
        return dbError(DbError::EXPIRED);
    }

    // create object
    auto obj=factory->createObject<typename modelType::ManagedType>(factory);

    // deserialize object
    auto objSlice=TtlMark::stripTtlMark(readSlice);
    dataunit::WireBufSolid buf{objSlice.data(),objSlice.size(),true};
    Error ec;
    if (!dataunit::io::deserialize(*obj,buf,ec))
    {
        HATN_CTX_SCOPE_ERROR("deserialize");
        return ec;
    }

    // done
    return obj;
#endif

    return readSingleObject(model,handler,partition.get(),key,factory,tx,forUpdate);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBREADOBJECT_IPP
