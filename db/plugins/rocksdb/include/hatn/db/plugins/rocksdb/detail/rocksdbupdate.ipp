/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbupdate.ipp
  *
  *   RocksDB database template for updating objects.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBUPDATE_IPP
#define HATNROCKSDBUPDATE_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/namespace.h>
#include <hatn/db/update.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/objectpartition.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbtransaction.ipp>
#include <hatn/db/plugins/rocksdb/detail/ttlindexes.ipp>
#include <hatn/db/plugins/rocksdb/rocksdbmodelt.h>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodelt.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
struct UpdateObjectT
{
    template <typename ModelT, typename DateT>
    Result<typename ModelT::SharedPtr> operator ()(const ModelT& model,
                                                  RocksdbHandler& handler,
                                                  const Namespace& ns,
                                                  const ObjectId& objectId,
                                                  const update::Request& request,
                                                  const DateT& date,
                                                  AllocatorFactory* allocatorFactory) const;
};
template <typename BufT>
constexpr UpdateObjectT<BufT> UpdateObject{};

template <typename BufT>
template <typename ModelT, typename DateT>
Result<typename ModelT::SharedPtr> UpdateObjectT<BufT>::operator ()(
    const ModelT& model,
    RocksdbHandler& handler,
    const Namespace& ns,
    const ObjectId& objectId,
    const update::Request& request,
    const DateT& date,
    AllocatorFactory* factory
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("rocksdbupdateobject")
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

    // construct key
    Keys<BufT> keys{factory->bytesAllocator()};
    ROCKSDB_NAMESPACE::Slice objectIdS{idData.data(),idData.size()};
    auto key=keys.objectKeySolid(keys.makeObjectKeyValue(model,ns,objectIdS));

    // transaction fn
    auto transactionFn=[&](Transaction* tx)
    {
        Error ec;
        auto rdbTx=RocksdbTransaction::native(tx);

        // get for update
        ROCKSDB_NAMESPACE::PinnableSlice readSlice;
        typename modelType::Type obj{factory};
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
            auto objSlice=TtlMark::stripTtlMark(readSlice);
            dataunit::WireBufSolid buf{objSlice.data(),objSlice.size(),true};
            if (!dataunit::io::deserialize(&obj,buf,ec))
            {
                HATN_CTX_SCOPE_ERROR("deserialize");
                return false;
            }

            // done
            return true;
        };
        auto found=getForUpdate();
        HATN_CHECK_EC(ec)

        // if not found then call not found callback
        if (!found)
        {
            //! @todo Special processing when not found
#if 0
            auto tryUpdate=notFoundCb(ec);
            HATN_CHECK_EC(ec)
            if (tryUpdate)
            {
                HATN_CTX_SCOPE("tryupdate");
                found=getForUpdate();
                if (!found)
                {
                    return ec;
                }
            }
            else
#endif
            {
                return Error{OK};
            }
        }

        // extract old keys for updated fields
        IndexKeyUpdateSet oldKeys;
        RocksdbModelT<modelType>::updatingKeys(keys,request,ns.topic(),objectId,obj,oldKeys);

        // apply request to object
        update::ApplyRequest(&obj,request);

        // serialize object
        dataunit::WireBufSolid buf{allocatorFactory};
        auto ec=serializeObject(obj,buf);
        HATN_CHECK_EC(ec)

        // save object
        auto ttlMark=TtlMark::ttlMark(readSlice);
        ec=saveObject(rdbTx,partition,key,buf,ttlMark);
        HATN_CHECK_EC(ec)

        // extract new keys for updated fields
        IndexKeyUpdateSet newKeys;
        RocksdbModelT<modelType>::updatingKeys(keys,request,ns.topic(),objectId,obj,newKeys);

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
                auto status=rdbTx->Delete(indexCf,oldKey.key);
                if (!status.ok())
                {
                    HATN_CTX_SCOPE_PUSH("idx_key",oldKey.key);
                    return makeError(DbError::DELETE_INDEX_FAILED,status);
                }
            }
        }

        // save new keys        
        ROCKSDB_NAMESPACE::SliceParts indexValue{&key,1};
        for (auto&& newKey : newKeys)
        {
            if (!newKey.exists)
            {                
                // save new key
                ec=SaveSingleIndex(newKey,rdbTx,indexValue);
                HATN_CTX_SCOPE_PUSH("idx_key",newKey.key);
                return makeError(DbError::SAVE_INDEX_FAILED,status);
            }
        }

        // update ttl index if actual
        if (RocksdbModelT<modelType>::checkTtlFieldUpdated(request))
        {
            using ttlIndexesT=TtlIndexes<modelType>;
            ROCKSDB_NAMESPACE::Slice objectIdS{objectId.data(),objectId.size()};

            // delete old ttl index
            ttlIndexesT::deleteTtlIndex(ec,rdbTx,partition,objectIdS,ttlMark);
            HATN_CHECK_EC(ec)

            // save new ttl index
            ttlIndexesT::saveTtlIndex(ec,model,obj,buf,rdbTx,partition,objectIdS,factory);
            HATN_CHECK_EC(ec)
        }

        // done
        return Error{OK};
    };

    // invoke transaction
    return handler.transaction(transactionFn,tx,true);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBUPDATE_IPP
