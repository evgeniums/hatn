/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbupdate.ipp
  *
  *   RocksDB database template for updating object.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBUPDATE_IPP
#define HATNROCKSDBUPDATE_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/topic.h>
#include <hatn/db/update.h>
#include <hatn/db/ipp/updateunit.ipp>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/savesingleindex.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/objectpartition.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbtransaction.ipp>
#include <hatn/db/plugins/rocksdb/detail/ttlindexes.ipp>
#include <hatn/db/plugins/rocksdb/rocksdbmodelt.h>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodelt.ipp>
#include <hatn/db/plugins/rocksdb/detail/saveobject.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct UpdateObjectT
{
    template <typename ModelT, typename DateT>
    Result<typename ModelT::SharedPtr> operator ()(const ModelT& model,
                                                  RocksdbHandler& handler,
                                                  const Topic& topic,
                                                  const ObjectId& objectId,
                                                  const update::Request& request,
                                                  const DateT& date,
                                                  db::update::ModifyReturn modifyReturn,
                                                  AllocatorFactory* allocatorFactory,
                                                  Transaction* tx
                                                  ) const;
};
constexpr UpdateObjectT UpdateObject{};

template <typename ModelT>
Result<typename ModelT::SharedPtr> updateSingle(
    Keys& keys,
    ROCKSDB_NAMESPACE::Slice objectIdS,
    const ROCKSDB_NAMESPACE::Slice& key,
    const ModelT& model,
    RocksdbHandler& handler,
    RocksdbPartition* partition,
    const lib::string_view& topic,
    const update::Request& request,
    db::update::ModifyReturn modifyReturn,
    AllocatorFactory* factory,
    Transaction* intx,
    bool* warnBrokenIndex=nullptr
    )
{
    using modelType=std::decay_t<ModelT>;

    // create object
    auto obj=factory->createObject<typename modelType::ManagedType>(factory);
    decltype(obj) objBefore;

    // transaction fn
    auto transactionFn=[&](Transaction* tx)
    {
        Error ec;
        auto rdbTx=RocksdbTransaction::native(tx);

        // get for update
        ROCKSDB_NAMESPACE::PinnableSlice readSlice;
        ROCKSDB_NAMESPACE::Slice objSlice;
        auto getForUpdate=[&]()
        {
            auto status=rdbTx->GetForUpdate(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
            if (!status.ok())
            {
                if (status.code()==ROCKSDB_NAMESPACE::Status::kNotFound)
                {
                    return false;
                }

                HATN_CTX_SCOPE_ERROR("get");
                ec=makeError(DbError::READ_FAILED,status);
                return false;
            }

            // check if object expired
            TtlMark::refreshCurrentTimepoint();
            if (TtlMark::isExpired(readSlice))
            {
                return false;
            }

            // deserialize object
            objSlice=TtlMark::stripTtlMark(readSlice);
            dataunit::WireBufSolid buf{objSlice.data(),objSlice.size(),true};
            if (!dataunit::io::deserialize(*obj,buf,ec))
            {
                HATN_CTX_SCOPE_ERROR("deserialize");
                return false;
            }

            // done
            return true;
        };
        auto found=getForUpdate();
        HATN_CHECK_EC(ec)

        // if not found then return
        if (!found)
        {
            if (warnBrokenIndex!=nullptr)
            {
                *warnBrokenIndex=true;
            }
            return Error{OK};
        }
        if (modifyReturn==db::update::ModifyReturn::Before)
        {
            objBefore=factory->createObject<typename modelType::ManagedType>(factory);
            dataunit::WireBufSolid buf{objSlice.data(),objSlice.size(),true};
            dataunit::io::deserialize(*objBefore,buf,ec);
            // ignore ec because this object was already deserialized
        }

        // extract old keys for updated fields
        const auto& k=key;
        IndexKeyUpdateSet oldKeys;
        RocksdbModelT<modelType>::updatingKeys(keys,request,topic,objectIdS,obj.get(),oldKeys);

        // apply request to object
        update::ApplyRequest(obj.get(),request);
        obj->field(object::updated_at).set(common::DateTime::currentUtc());
        auto ttlUpdated=RocksdbModelT<modelType>::checkTtlFieldUpdated(request);

        // serialize object
        dataunit::WireBufSolid buf{factory};
        ec=serializeObject(obj.get(),buf);
        HATN_CHECK_EC(ec)

        // save object
        ROCKSDB_NAMESPACE::SliceParts keySlices{&k,1};
        TtlMark ttlMark;
        auto oldTtlMarkSlice=TtlMark::ttlMark(readSlice);
        auto ttlMarkSlice=oldTtlMarkSlice;
        if (ttlUpdated)
        {
            ttlMark.fill(model,obj.get());
            ttlMarkSlice=ttlMark.slice();
        }
        ec=saveObject(rdbTx,partition,keySlices,buf,ttlMarkSlice);
        HATN_CHECK_EC(ec)

        // extract new keys for updated fields
        IndexKeyUpdateSet newKeys;
        RocksdbModelT<modelType>::updatingKeys(keys,request,topic,objectIdS,obj.get(),newKeys);

        // find keys difference
        for (auto& newKey : newKeys)
        {
            auto oldKeyIt=oldKeys.find(newKey);
            if (oldKeyIt!=oldKeys.end())
            {
                newKey.exists=true;
                oldKeyIt->exists=true;
            }
        }

        auto indexCf=partition->indexCf.get();

        // delete old keys
        for (auto&& oldKey : oldKeys)
        {
            if (!oldKey.exists)
            {
                // delete old key
                ROCKSDB_NAMESPACE::SliceParts keySlices{oldKey.key.data(),static_cast<int>(oldKey.key.size())};
                auto status=rdbTx->Delete(indexCf,keySlices);
                if (!status.ok())
                {
                    HATN_CTX_SCOPE_PUSH("idx_name",oldKey.indexName);
                    return makeError(DbError::DELETE_INDEX_FAILED,status);
                }
            }
        }

        // save new keys
        if (!newKeys.empty())
        {
            const auto& objectCreatedAt=obj->field(object::created_at).value();
            auto [objectKeyFull,_]=Keys::makeObjectKeyValue(model.modelIdStr(),topic,objectIdS,objectCreatedAt,ttlMarkSlice);
            auto indexValue=Keys::indexValueSlices(objectKeyFull);
            for (auto&& newKey : newKeys)
            {
                if (!newKey.exists)
                {
                    // save new key
                    ec=SaveSingleIndex(handler,newKey.key,newKey.unique,indexCf,rdbTx,indexValue);
                    if (ec)
                    {
                        HATN_CTX_SCOPE_PUSH("idx_name",newKey.indexName)
                        HATN_CTX_SCOPE_LOCK()
                        return ec;
                    }
                }
            }
        }

        // update ttl index if actual
        if (ttlUpdated)
        {
            using ttlIndexesT=TtlIndexes<modelType>;

            // delete old ttl index
            ttlIndexesT::deleteTtlIndex(ec,rdbTx,partition,objectIdS,oldTtlMarkSlice);
            HATN_CHECK_EC(ec)

            // save new ttl index
            ttlIndexesT::saveTtlIndexWithMark(ttlMarkSlice,ec,model,obj.get(),buf,rdbTx,partition,objectIdS,factory);
            HATN_CHECK_EC(ec)
        }

        // done
        return Error{OK};
    };

    // invoke transaction
    auto ec=handler.transaction(transactionFn,intx,true);
    HATN_CHECK_EC(ec)

    // done
    if (modifyReturn==db::update::ModifyReturn::Before)
    {
        return objBefore;
    }
    return obj;
}

template <typename ModelT, typename DateT>
Result<typename ModelT::SharedPtr> UpdateObjectT::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const Topic& topic,
        const ObjectId& objectId,
        const update::Request& request,
        const DateT& date,
        db::update::ModifyReturn modifyReturn,
        AllocatorFactory* factory,
        Transaction* intx
    ) const
{
    HATN_CTX_SCOPE("rocksdbupdateobject")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())
    HATN_CTX_SCOPE_PUSH("topic",topic.topic())
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

    // construct key
    Keys keys{factory};
    ROCKSDB_NAMESPACE::Slice objectIdS{idData.data(),idData.size()};
    auto [objKeyVal,_]=keys.makeObjectKeyValue(model.modelIdStr(),topic.topic(),objectIdS);
    auto key=keys.objectKeySolid(objKeyVal);

    // do
    auto r=updateSingle(keys,objectIdS,key,model,handler,partition.get(),topic.topic(),request,modifyReturn,factory,intx);
    return r;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBUPDATE_IPP
