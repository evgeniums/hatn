/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbcreateobject.ipp
  *
  *   RocksDB database template for object creating.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBCREATEOBJECT_IPP
#define HATNROCKSDBCREATEOBJECT_IPP

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/namespace.h>
#include <hatn/db/transaction.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/ttlindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/objectpartition.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbtransaction.ipp>
#include <hatn/db/plugins/rocksdb/detail/saveobject.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
struct CreateObjectT
{
    template <typename ModelT>
    Error operator ()(const ModelT& model,
                     RocksdbHandler& handler,
                     const Namespace& ns,
                     const typename ModelT::UnitType::type* obj,
                     AllocatorFactory* allocatorFactory,
                     Transaction* tx
                     ) const;
};
template <typename BufT>
constexpr CreateObjectT<BufT> CreateObject{};

template <typename BufT>
template <typename ModelT>
Error CreateObjectT<BufT>::operator ()(
                               const ModelT& model,
                               RocksdbHandler& handler,
                               const Namespace& ns,
                               const typename ModelT::UnitType::type* obj,
                               AllocatorFactory* allocatorFactory,
                               Transaction* tx
                              ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("rocksdbcreateobject")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())
    HATN_CTX_SCOPE_PUSH("topic",ns.topic())

    if (handler.readOnly())
    {
        return dbError(DbError::DB_READ_ONLY);
    }

    // eval partition
    const auto partition=objectPartition(handler,model,obj);
    if (!partition)
    {
        HATN_CTX_SCOPE_ERROR("find-partition");
        return dbError(DbError::PARTITION_NOT_FOUND);
    }

    // serialize object
    dataunit::WireBufSolid buf{allocatorFactory};
    auto ec=serializeObject(obj,buf);
    HATN_CHECK_EC(ec)

    // transaction fn
    auto transactionFn=[&](Transaction* tx)
    {
        auto rdbTx=RocksdbTransaction::native(tx);
        auto objectId=obj->field(object::_id).value().toArray();
        ROCKSDB_NAMESPACE::Slice objectIdS{objectId.data(),objectId.size()};

        // prepare
        Keys<BufT> keys{allocatorFactory->bytesAllocator()};
        auto ttlMark=makeTtlMark(model,obj);

        // put serialized object to transaction
        const auto& objectCreatedAt=obj->field(object::created_at).value();
        auto objectKeyFull=keys.makeObjectKeyValue(model,ns.topic(),objectIdS,objectCreatedAt,ttlMark);
        auto objectKeySlices=keys.objectKeySlices(objectKeyFull);
        auto ec=saveObject(rdbTx,partition.get(),objectKeySlices,buf,ttlMark);
        HATN_CHECK_EC(ec)

        // put indexes to transaction
        auto indexValueSlices=ROCKSDB_NAMESPACE::SliceParts{&objectKeyFull[0],static_cast<int>(objectKeyFull.size())};
        std::cout<<"indexValueSlices.num_parts="<<indexValueSlices.num_parts<<",ttl="<<int(objectKeyFull[7][0])<<std::endl;
        Indexes<BufT> indexes{partition->indexCf.get(),keys};
        ec=indexes.saveIndexes(rdbTx,model,ns,objectIdS,indexValueSlices,obj);
        HATN_CHECK_EC(ec)

        // put ttl index to transaction
        using ttlIndexesT=TtlIndexes<modelType>;
        ttlIndexesT::saveTtlIndexWithMark(ttlMark,ec,model,obj,buf,rdbTx,partition.get(),objectIdS,allocatorFactory);
        HATN_CHECK_EC(ec)

        // done
        return Error{OK};
    };

    // invoke transaction
    return handler.transaction(transactionFn,tx,true);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCREATEOBJECT_IPP
