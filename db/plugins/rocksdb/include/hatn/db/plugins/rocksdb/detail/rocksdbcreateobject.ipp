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

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/namespace.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler_p.h>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbindexes.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
struct CreateObjectT
{
    template <typename ModelT, typename UnitT,
             typename AllocatorT=common::FmtAllocator
             >
    Error operator ()(const ModelT& model, RocksdbHandler& handler, const Namespace& ns, UnitT* object, const AllocatorT& alloc=AllocatorT{}) const;
};
template <typename BufT>
constexpr CreateObjectT<BufT> CreateObject{};

template <typename BufT>
template <typename ModelT, typename UnitT, typename AllocatorT>
Error CreateObjectT<BufT>::operator ()(
                               const ModelT& model,
                               RocksdbHandler& handler,
                               const Namespace& ns,
                               UnitT* object,
                               const AllocatorT& alloc
                               ) const
{
    using modelType=std::decay_t<ModelT>;
    using unitType=typename modelType::UnitType;
    static_assert(
        std::is_same<
            std::decay_t<UnitT>,
            typename unitType::type
            >::value,
        "Invalid type of object (UnitT)"
    );

    HATN_CTX_SCOPE("rocksdbcreateobject")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())
    HATN_CTX_SCOPE_PUSH("topic",ns.topic())

    if (handler.readOnly())
    {
        return dbError(DbError::DB_READ_ONLY);
    }

    // handle partition
    auto&& partition=hana::eval_if(
        hana::bool_<modelType::isDatePartitioned()>{},
        [&](auto _)
        {
            auto partitionRange=datePartition(*_(object),_(model));
            HATN_CTX_SCOPE_PUSH_("partition",partitionRange)
            return _(handler).partition(partitionRange);
        },
        [&](auto _)
        {
            return _(handler).defaultPartition();
        }
    );
    if (!partition)
    {
        HATN_CTX_SCOPE_ERROR("find-partition");
        return dbError(DbError::PARTITION_NOT_FOUND);
    }

    // serialize object
    dataunit::WireBufSolid buf;
    if (!object->wireDataKeeper())
    {
        Error ec;
        dataunit::io::serialize(*object,buf,ec);
        if(ec)
        {
            HATN_CTX_SCOPE_ERROR("serialize");
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
        Keys<BufT> keys{alloc};

        //! @todo check unique indexes

        // put serialized object to batch
        auto objectKey=keys.makeObjectKey(model,ns,objectIdS);
        ROCKSDB_NAMESPACE::SliceParts keySlices{&objectKey[0],static_cast<int>(objectKey.size())};
        ROCKSDB_NAMESPACE::Slice value{buf.mainContainer()->data(),buf.mainContainer()->size()};
        ROCKSDB_NAMESPACE::SliceParts valueSlices{&value,1};
        auto status=batch.Put(partition->collectionCf.get(),keySlices,valueSlices);
        if (!status.ok())
        {
            HATN_CTX_SCOPE_ERROR("batch-object");
            return makeError(DbError::WRITE_OBJECT_FAILED,status);
        }

        // put indexes to batch
        Indexes<BufT> indexes{partition->indexCf.get(),keys};
        auto ec=indexes.saveIndexes(batch,model,ns,objectIdS,keySlices,object);
        HATN_CHECK_EC(ec)

        //! @todo put ttl indexes to batch

        // write batch to rocksdb
        status=rdb->Write(handler.p()->writeOptions,&batch);
        if (!status.ok())
        {
            HATN_CTX_SCOPE_ERROR("write-batch");
            return makeError(DbError::WRITE_OBJECT_FAILED,status);
        }

        // done
        return Error{OK};
    };

    // invoke transaction
    return handler.transaction(transactionFn,false);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCREATEOBJECT_IPP
