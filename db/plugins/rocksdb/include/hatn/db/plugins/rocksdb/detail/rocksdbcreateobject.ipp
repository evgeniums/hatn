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

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/ttlindexes.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
struct CreateObjectT
{
    template <typename ModelT>
    Error operator ()(const ModelT& model,
                     RocksdbHandler& handler,
                     const Namespace& ns,
                     const typename ModelT::UnitType::type* object,
                     AllocatorFactory* allocatorFactory
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
                               const typename ModelT::UnitType::type* object,
                               AllocatorFactory* allocatorFactory
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

    // handle partition
    auto partitionR=hana::eval_if(
        hana::bool_<modelType::isDatePartitioned()>{},
        [&](auto _)
        {
            auto partitionRange=datePartition(*_(object),_(model));
            HATN_CTX_SCOPE_PUSH_("partition",partitionRange)
            return std::make_pair(_(handler).partition(partitionRange),partitionRange);
        },
        [&](auto _)
        {
            return std::make_pair(_(handler).defaultPartition(),common::DateRange{});
        }
    );
    const auto& partition=partitionR.first;
    if (!partition)
    {
        HATN_CTX_SCOPE_ERROR("find-partition");
        return dbError(DbError::PARTITION_NOT_FOUND);
    }
    const auto& dateRange=partitionR.second;

    // serialize object
    dataunit::WireBufSolid buf{allocatorFactory};
    if (!object->wireDataKeeper())
    {
        Error ec;
        dataunit::io::serialize(*object,buf,ec);
        if(ec)
        {
            HATN_CTX_SCOPE_ERROR("serialize object");
            return ec;
        }
    }
    else
    {
        buf=object->wireDataKeeper()->toSolidWireBuf();
    }

    // transaction fn
    auto transactionFn=[&]()
    {
        auto rdb=handler.p()->transactionDb;
        auto objectId=object->field(object::_id).value().toString();
        ROCKSDB_NAMESPACE::Slice objectIdS{objectId.data(),objectId.size()};

        // prepare
        ROCKSDB_NAMESPACE::WriteBatch batch;
        Keys<BufT> keys{allocatorFactory->bytesAllocator()};
        TtlMark::refreshCurrentTimepoint();
        TtlMark ttlMarkObj{model,object};
        auto ttlMark=ttlMarkObj.slice();

        // put serialized object to batch
        auto objectKeyFull=keys.makeObjectKeyValue(model,ns,objectIdS,ttlMark);
        auto objectKeySlices=keys.objectKeySlices(objectKeyFull);
        std::array<ROCKSDB_NAMESPACE::Slice,2> objectValueParts{
            ROCKSDB_NAMESPACE::Slice{buf.mainContainer()->data(),buf.mainContainer()->size()},
            ttlMark
        };
        ROCKSDB_NAMESPACE::SliceParts objectValueSlices{&objectValueParts[0],static_cast<int>(objectValueParts.size())};
        auto status=batch.Put(partition->collectionCf.get(),objectKeySlices,objectValueSlices);
        if (!status.ok())
        {
            HATN_CTX_SCOPE_ERROR("batch-object");
            return makeError(DbError::WRITE_OBJECT_FAILED,status);
        }

        // prepare ttl index
        using ttlIndexesT=TtlIndexes<modelType>;
        using ttlIndexT=typename ttlIndexesT::ttlT;
        ttlIndexT ttlIndex{allocatorFactory};
        ttlIndexesT::prepareTtl(ttlIndex,model,object->field(object::_id).value(),dateRange);

        // put indexes to batch
        auto indexValueSlices=ROCKSDB_NAMESPACE::SliceParts{&objectKeyFull[0],static_cast<int>(objectKeyFull.size())};
        Indexes<BufT> indexes{partition->indexCf.get(),keys};
        auto ec=indexes.saveIndexes(batch,model,ns,objectIdS,indexValueSlices,object,
                                    ttlIndexesT::makeIndexKeyCb(ttlIndex)
        );
        HATN_CHECK_EC(ec)

        // put ttl index to batch
        ttlIndexesT::putTtlToBatch(ec,buf,batch,ttlIndex,partition,objectIdS,ttlMark);
        HATN_CHECK_EC(ec)

        // write batch to rocksdb
        status=rdb->Write(handler.p()->writeOptions,&batch);
        if (!status.ok())
        {
            HATN_CTX_SCOPE_ERROR("write-batch");
            ec=makeError(DbError::WRITE_OBJECT_FAILED,status);
            if (RocksdbOpError::ec())
            {
                auto prevEc=RocksdbOpError::ec();
                ec.setPrevError(std::move(prevEc));
            }
            return ec;
        }

        // done
        return Error{OK};
    };

    // invoke transaction
    return handler.transaction(transactionFn,false);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCREATEOBJECT_IPP
