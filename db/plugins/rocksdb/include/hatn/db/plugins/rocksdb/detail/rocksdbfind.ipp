/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbfind.ipp
  *
  *   RocksDB database template for finding objects.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBFIND_IPP
#define HATNROCKSDBFIND_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/common/runonscopeexit.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/indexquery.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/indexkeysearch.h>
#include <hatn/db/plugins/rocksdb/rocksdbkeys.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/querypartitions.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct FindT
{
    template <typename ModelT>
    Result<common::pmr::vector<DbObject>> operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& query,
        bool single,
        const AllocatorFactory* allocatorFactory
    ) const;
};
constexpr FindT Find{};

template <typename ModelT>
Result<common::pmr::vector<DbObject>> FindT::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& idxQuery,
        bool single,
        const AllocatorFactory* allocatorFactory
    ) const
{
    HATN_CTX_SCOPE("find")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())

    // collect partitions for processing
    index_key_search::Partitions partitions;
    auto withPartitionQuery=index_key_search::queryPartitions(partitions,model,handler,idxQuery);

    // make snapshot
    ROCKSDB_NAMESPACE::ManagedSnapshot managedSnapshot{handler.p()->db};
    const auto* snapshot=managedSnapshot.snapshot();
    TtlMark::refreshCurrentTimepoint();

    // collect index keys
    auto indexKeys=index_key_search::indexKeys(snapshot,
                                                 handler,
                                                 model.modelIdStr(),
                                                 idxQuery,
                                                 partitions,
                                                 allocatorFactory,
                                                 single,
                                                 withPartitionQuery
                                              );
    HATN_CHECK_RESULT(indexKeys)

    // prepare result
    common::pmr::vector<DbObject> objects{allocatorFactory->dataAllocator<DbObject>()};

    // if keys not found then return empty result
    if (indexKeys->empty())
    {
        return objects;
    }

    // fill result
    {
        HATN_CTX_SCOPE("readobjects")

        Error ec;
        dataunit::WireBufSolid buf;
        ROCKSDB_NAMESPACE::ReadOptions readOptions=handler.p()->readOptions;
        readOptions.snapshot=snapshot;
        objects.reserve(indexKeys->size());
        for (auto&& key: indexKeys.value())
        {
            // get object from rocksdb            
            auto k=Keys::objectKeyFromIndexValue(key.value.data(),key.value.size());

//! @maybe Log debug
#if 0
            std::cout<<"Find: index key "<< logKey(key.key)<<" object key " << logKey(k) << std::endl;
#endif
            auto pushLogKey=[&k,&key]()
            {
                HATN_CTX_SCOPE_PUSH("obj_key",lib::toStringView(logKey(k)))
                HATN_CTX_SCOPE_PUSH("idx_key",lib::toStringView(logKey(key.key)))
                if (!key.partition->range.isNull())
                {
                    HATN_CTX_SCOPE_PUSH("db_partition",key.partition->range)
                }
            };

            ROCKSDB_NAMESPACE::PinnableSlice value;
            auto status=handler.p()->db->Get(readOptions,key.partition->dataCf(model.isBlob()),k,&value);
            if (!status.ok())
            {
                pushLogKey();
                if (status.code()!=ROCKSDB_NAMESPACE::Status::Code::kNotFound)
                {
                    HATN_CTX_SCOPE_ERROR("get-object")
                    return makeError(DbError::READ_FAILED,status);
                }
                HATN_CTX_WARN("missing object in rocksdb")
                HATN_CTX_SCOPE_POP()
                HATN_CTX_SCOPE_POP()
                if (!key.partition->range.isNull())
                {
                    HATN_CTX_SCOPE_POP()
                }
                continue;
            }
            if (TtlMark::isExpired(value))
            {
                pushLogKey();
                HATN_CTX_WARN("object expired in rocksdb")
                HATN_CTX_SCOPE_POP()
                HATN_CTX_SCOPE_POP()
                if (!key.partition->range.isNull())
                {
                    HATN_CTX_SCOPE_POP()
                }
                continue;
            }

            auto addToResult=[&](auto&& sharedUnit)
            {
                // deserialize object
                auto objSlice=TtlMark::stripTtlMark(value);
                buf.loadInline(objSlice.data(),objSlice.size());
                dataunit::io::deserialize(*sharedUnit,buf,ec);
                if (ec)
                {
                    pushLogKey();
                    HATN_CTX_SCOPE_ERROR("deserialize-object")
                    return ec;
                }

//! @maybe Log debug
#if 0
            std::cout<<"Find: object appended to result" << std::endl;
#endif \
                // emplace wrapped unit to result vector
                objects.emplace_back(std::move(sharedUnit),key.topic());

                return Error{};
            };

            // create unit
            auto sharedUnit=allocatorFactory->createObject<typename ModelT::ManagedType>(allocatorFactory);
            sharedUnit->setParseToSharedArrays(idxQuery.query.isParseToSharedArrays(),allocatorFactory);
            ec=addToResult(std::move(sharedUnit));
            HATN_CHECK_EC(ec)
        }
    }

    // done
    return objects;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBFIND_IPP
