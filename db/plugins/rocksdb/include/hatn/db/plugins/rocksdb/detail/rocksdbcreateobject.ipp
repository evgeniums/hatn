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
#include <hatn/db/topic.h>
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
#include <hatn/db/plugins/rocksdb/modeltopics.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct CreateObjectT
{
    template <typename ModelT>
    Error operator ()(const ModelT& model,
                     RocksdbHandler& handler,
                     Topic topic,
                     const typename ModelT::UnitType::type* obj,
                     const AllocatorFactory* allocatorFactory,
                     Transaction* tx
                     ) const;
};
constexpr CreateObjectT CreateObject{};

template <typename ModelT>
Error CreateObjectT::operator ()(
                               const ModelT& model,
                               RocksdbHandler& handler,
                               Topic topic,
                               const typename ModelT::UnitType::type* obj,
                               const AllocatorFactory* allocatorFactory,
                               Transaction* tx
                              ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("createobject")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())
    HATN_CTX_SCOPE_PUSH("topic",topic.topic())

    if (handler.readOnly())
    {
        return dbError(DbError::DB_READ_ONLY);
    }

    // eval partition
    const auto partition=objectPartition(handler,model,*obj);
    if (!partition)
    {
        HATN_CTX_SCOPE_ERROR("find-partition");
        return dbError(DbError::PARTITION_NOT_FOUND);
    }
    if (!partition->range.isNull())
    {
        HATN_CTX_SCOPE_PUSH("partition",partition->range)
    }

    // serialize object
    dataunit::WireBufSolid buf{allocatorFactory};
    auto ec=serializeObject(obj,buf);
    HATN_CHECK_EC(ec)
    auto ttlMark=makeTtlMark(model,obj);

    // transaction fn
    auto transactionFn=[&](Transaction* tx)
    {
        auto rdbTx=RocksdbTransaction::native(tx);
        auto objectId=obj->field(object::_id).value().toArray();
        ROCKSDB_NAMESPACE::Slice objectIdS{objectId.data(),objectId.size()};
        HATN_CTX_SCOPE_PUSH("oid",lib::string_view(objectId.data(),objectId.size()))

        // prepare
        Keys keys{allocatorFactory};        

        // put serialized object to transaction
        const auto& objectCreatedAt=obj->field(object::created_at).value();
        uint32_t timestamp=0;
        auto objectKeyFull=Keys::makeObjectKeyValue(model.modelIdStr(),topic,objectIdS,&timestamp,objectCreatedAt,ttlMark);
        auto objectKeySlices=Keys::objectKeySlices(objectKeyFull);
        auto ec=saveObject(rdbTx,partition.get(),objectKeySlices,buf,ttlMark);
        HATN_CHECK_EC(ec)

        // put indexes to transaction
        auto indexValueSlices=Keys::indexValueSlices(objectKeyFull);
        Indexes indexes{partition->indexCf.get(),keys};
        ec=indexes.saveIndexes(handler,rdbTx,model,topic,objectIdS,indexValueSlices,obj);
        HATN_CHECK_EC(ec)

        // put ttl index to transaction
        using ttlIndexesT=TtlIndexes<modelType>;
        ttlIndexesT::saveTtlIndexWithMark(ttlMark,ec,model,obj,buf,rdbTx,partition.get(),objectIdS,allocatorFactory);
        HATN_CHECK_EC(ec)

        // done
        return Error{OK};
    };

    // invoke transaction
    ec=handler.transaction(transactionFn,tx,true);
    HATN_CHECK_EC(ec)

    // save model-topic relation
    ec=ModelTopics::update(model.modelIdStr(),topic,handler,partition.get(),ModelTopics::Operator::Add);
    if (ec)
    {
        HATN_CTX_ERROR(ec,"failed to save model-topic relation")
    }

    // done
    return OK;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCREATEOBJECT_IPP
