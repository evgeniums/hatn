/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbdelete.ipp
  *
  *   RocksDB database template for deleting objects.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBDELETE_IPP
#define HATNROCKSDBDELETE_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/common/runonscopeexit.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/namespace.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/indexquery.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/ttlindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/objectpartition.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbtransaction.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
struct DeleteObjectT
{
    template <typename ModelT, typename ttlIndexesT>
    Error doDelete(
            const ModelT& model,
            RocksdbHandler& handler,
            RocksdbPartition* partition,
            const lib::string_view& topic,
            const ROCKSDB_NAMESPACE::Slice& objectKey,
            Keys<BufT>& keys,
            ttlIndexesT& ttlIndexes,
            Transaction* tx
        ) const
    {
        using modelType=std::decay_t<ModelT>;
        using unitType=typename modelType::Type;

        HATN_CTX_SCOPE("deleteobject")
        HATN_CTX_SCOPE_PUSH("coll",model.collection())
        HATN_CTX_SCOPE_PUSH("topic",topic)

        // read object from db
        auto rdbTx=RocksdbTransaction::native(tx);
        ROCKSDB_NAMESPACE::PinnableSlice readSlice;
        auto status=rdbTx->Get(handler.p()->readOptions,partition->collectionCf.get(),objectKey,&readSlice);
        if (!status.ok())
        {
            if (status.code()==ROCKSDB_NAMESPACE::Status::kNotFound)
            {
                // object not found, nothing to do
                return OK;
            }

            HATN_CTX_SCOPE_ERROR("get-object");
            return makeError(DbError::READ_FAILED,status);
        }

        // deserialize object
        thread_local static unitType unit;
        auto objSlice=TtlMark::stripTtlMark(readSlice);
        dataunit::WireBufSolid buf{objSlice.data(),objSlice.size(),true};
        Error ec;
        if (!dataunit::io::deserialize(unit,buf,ec))
        {
            HATN_CTX_SCOPE_ERROR("deserialize");
            return ec;
        }
        auto objectId=unit.field(object::_id).value().toArray();
        ROCKSDB_NAMESPACE::Slice objectIdS{objectId.data(),objectId.size()};

        // delete object
        status=rdbTx->Delete(partition->collectionCf.get(),objectKey);
        if (!status.ok())
        {
            HATN_CTX_SCOPE_ERROR("delete-object");
            return makeError(DbError::DELETE_OBJECT_FAILED,status);
        }

        // append index deletions to transaction
        Indexes<BufT> indexes{partition->indexCf.get(),keys};
        ec=indexes.deleteIndexes(rdbTx,model,topic,objectIdS,&unit);
        HATN_CHECK_EC(ec)

        // append ttl index deletion to transaction
        TtlMark ttlMarkObj{model,&unit};
        auto ttlMark=ttlMarkObj.slice();
        ttlIndexes.deleteTtlIndex(ec,rdbTx,partition,objectIdS,ttlMark);
        HATN_CHECK_EC(ec)

        // done
        return OK;
    }

    template <typename ModelT, typename DateT>
    Error operator ()(const ModelT& model,
                      RocksdbHandler& handler,
                      const Namespace& ns,
                      const ObjectId& objectId,
                      const DateT& date,
                      AllocatorFactory* allocatorFactory,
                      Transaction* tx) const;
};
template <typename BufT>
constexpr DeleteObjectT<BufT> DeleteObject{};

template <typename BufT>
template <typename ModelT, typename DateT>
Error DeleteObjectT<BufT>::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const Namespace& ns,
        const ObjectId& objectId,
        const DateT& date,
        AllocatorFactory* factory,
        Transaction* tx
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("rocksdbdeleteobject")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())
    HATN_CTX_SCOPE_PUSH("topic",ns.topic())
    auto idData=objectId.toArray();
    auto idDataStr=lib::string_view{idData.data(),idData.size()};
    HATN_CTX_SCOPE_PUSH("object",idDataStr)

    // eval partition
    const auto partition=objectPartition(handler,model,objectId,date);
    if (!partition)
    {
        HATN_CTX_SCOPE_ERROR("find-partition");
        return dbError(DbError::PARTITION_NOT_FOUND);
    }

    // construct object key
    Keys<BufT> keys{factory->bytesAllocator()};
    ROCKSDB_NAMESPACE::Slice objectIdS{idData.data(),idData.size()};
    auto key=keys.objectKeySolid(keys.makeObjectKeyValue(model,ns.topic(),objectIdS));

    // delete object
    using ttlIndexesT=TtlIndexes<modelType>;
    static ttlIndexesT ttlIndexes{};

    // transaction fn
    auto transactionFn=[&](Transaction* tx)
    {
        return doDelete(model,handler,partition.get(),ns.topic(),key,keys,ttlIndexes,tx);
    };

    // invoke transaction
    return handler.transaction(transactionFn,tx,true);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBDELETE_IPP
