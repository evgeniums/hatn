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

        //! @todo get for update
        typename modelType::Type obj{factory};
        auto getForUpdate=[&]()
        {
            // ROCKSDB_NAMESPACE::PinnableSlice readSlice;
            // auto status=rdb->Get(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
            // if (!status.ok())
            // {
            //     if (status.code()==ROCKSDB_NAMESPACE::Status::kNotFound)
            //     {
            //         return dbError(DbError::NOT_FOUND);
            //     }

            //     HATN_CTX_SCOPE_ERROR("get");
            //     return makeError(DbError::READ_FAILED,status);
            // }
            // check if object expired
            // TtlMark::refreshCurrentTimepoint();
            // if (TtlMark::isExpired(readSlice))
            // {
            //     return dbError(DbError::EXPIRED);
            // }

            // deserialize object
            // auto objSlice=TtlMark::stripTtlMark(readSlice);
            // dataunit::WireBufSolid buf{objSlice.data(),objSlice.size(),true};
            // Error ec;
            // if (!dataunit::io::deserialize(obj,buf,ec))
            // {
            //     HATN_CTX_SCOPE_ERROR("deserialize");
            //     return ec;
            // }
        };
        auto found=getForUpdate();

        // if not found then call not found callback
        if (!found)
        {
            auto tryUpdate=notFoundCb(ec);
            HATN_CHECK_EC(ec)
            if (tryUpdate)
            {
                found=getForUpdate();
                if (!found)
                {
                    //! @todo Report error
                    return ec;
                }
            }
            else
            {
                return Error{OK};
            }
        }

        //! @todo extract old keys for updated fields
        IndexKeyUpdateSet oldKeys;

        // apply request to object
        update::ApplyRequest(&obj,request);

        //! @todo save object

        //! @todo extract new keys for updated fields
        IndexKeyUpdateSet newKeys;

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

        // delete old keys
        for (auto& oldKey : oldKeys)
        {
            if (!oldKey.exists)
            {
                //! @todo delete old key
            }
        }

        // save new keys
        for (auto& newKey : newKeys)
        {
            if (!newKey.exists)
            {
                //! @todo save new key
            }
        }

        //! @todo update ttl index if actual

        // done
        return Error{OK};
    };

    // invoke transaction
    return handler.transaction(transactionFn,tx,true);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBUPDATE_IPP
