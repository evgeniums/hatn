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

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/namespace.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/query.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/indexkeysearch.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
struct FindT
{
    template <typename ModelT>
    Result<common::pmr::vector<UnitWrapper>> operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        IndexQuery& query,
        AllocatorFactory* allocatorFactory
    ) const;
};
template <typename BufT>
constexpr FindT<BufT> Find{};

template <typename BufT>
template <typename ModelT>
Result<common::pmr::vector<UnitWrapper>> FindT<BufT>::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        IndexQuery& idxQuery,
        AllocatorFactory* allocatorFactory
    ) const
{
    HATN_CTX_SCOPE("rocksdbfind")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())

    // figure out partitions for processing
    auto partitions=index_key_search::partitions(model,handler,idxQuery,allocatorFactory);

    // get rocksdb snapshot
    ROCKSDB_NAMESPACE::ManagedSnapshot managedSnapchot{handler.p()->db};
    const auto* snapshot=managedSnapchot.snapshot();

    // collect index keys
    auto indexKeys=index_key_search::template indexKeys<BufT>(snapshot,handler,idxQuery,partitions,allocatorFactory);
    HATN_CHECK_RESULT(indexKeys)

    // prepare result
    common::pmr::vector<UnitWrapper> objects{allocatorFactory->dataAllocator<UnitWrapper>()};

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
            ROCKSDB_NAMESPACE::PinnableSlice value;
            auto k=KeysBase::objectKeyFromIndexValue(key.value.data(),key.value.size());

            auto pushLogKey=[&k,&key]()
            {
                HATN_CTX_SCOPE_PUSH("obj_key",lib::toStringView(k))
                HATN_CTX_SCOPE_PUSH("idx_key",lib::toStringView(key.key))
                HATN_CTX_SCOPE_PUSH("db_partition",key.partition->range)
            };

            auto status=handler.p()->db->Get(readOptions,key.partition->collectionCf.get(),k,&value);
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
                HATN_CTX_SCOPE_POP()
                continue;
            }

            // create unit
            auto sharedUnit=allocatorFactory->createObject<typename ModelT::ManagedType>(allocatorFactory);

            // deserialize object
            buf.mainContainer()->loadInline(value.data(),value.size());
            dataunit::io::deserialize(*sharedUnit,buf,ec);
            if (ec)
            {
                pushLogKey();
                HATN_CTX_SCOPE_ERROR("deserialize-object")
                return ec;
            }

            // emplace wrapped unit to result vector
            objects.emplace_back(sharedUnit);
        }
    }

    // done
    return objects;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBFIND_IPP
