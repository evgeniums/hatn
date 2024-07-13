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
#include <hatn/db/namespace.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
struct ReadObjectT
{
    template <typename ModelT, typename DateT>
    Result<typename ModelT::SharedPtr> operator ()(const ModelT& model,
                                                  RocksdbHandler& handler,
                                                  const Namespace& ns,
                                                  const ObjectId& objectId,
                                                  const DateT& date,
                                                  AllocatorFactory* allocatorFactory) const;
};
template <typename BufT>
constexpr ReadObjectT<BufT> ReadObject{};

template <typename BufT>
template <typename ModelT, typename DateT>
Result<typename ModelT::SharedPtr> ReadObjectT<BufT>::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const Namespace& ns,
        const ObjectId& objectId,
        const DateT& date,
        AllocatorFactory* factory
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("rocksdbreadobject")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())
    HATN_CTX_SCOPE_PUSH("topic",ns.topic())
    auto idData=objectId.toArray();
    auto idDataStr=lib::string_view{idData.data(),idData.size()};
    HATN_CTX_SCOPE_PUSH("object",idDataStr)

    // handle partition
    auto partitionR=hana::eval_if(
        hana::bool_<modelType::isDatePartitioned()>{},
        [&](auto _)
        {
            using dateT=std::decay_t<decltype(_(date))>;
            std::pair<std::shared_ptr<RocksdbPartition>,common::DateRange> r;
            r.second=hana::eval_if(
                std::is_same<dateT,hana::false_>{},
                [&](auto _)
                {
                    // check if partition field is _id
                    using modelType=std::decay_t<decltype(_(model))>;
                    using eqT=std::is_same<
                        std::decay_t<decltype(object::_id)>,
                        std::decay_t<decltype(modelType::datePartitionField())>
                        >;
                    if constexpr (eqT::value)
                    {
                        return datePartition(_(objectId),_(model));
                    }
                    else
                    {
                        Assert(eqT::value,"Object ID must be a date partition index field");
                        return HATN_COMMON_NAMESPACE::DateRange{};
                    }
                },
                [&](auto _)
                {
                    return datePartition(_(date),_(model));
                }
            );
            HATN_CTX_SCOPE_PUSH_("partition",r.second)
            r.first=_(handler).partition(r.second);
            return r;
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

    // construct key
    Keys<BufT> keys{factory->bytesAllocator()};
    ROCKSDB_NAMESPACE::Slice objectIdS{idData.data(),idData.size()};
    auto key=keys.objectKeySlice(keys.makeObjectKey(model,ns,objectIdS));

    // read object from db
    auto rdb=handler.p()->db;
    ROCKSDB_NAMESPACE::PinnableSlice readSlice;
    auto status=rdb->Get(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
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
        //! @todo Delete object?
        return dbError(DbError::EXPIRED);
    }

    //! @todo Check datetime index?

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

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBREADOBJECT_IPP
