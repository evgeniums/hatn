/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbdeletemany.ipp
  *
  *   RocksDB database template for deleting objects by index query.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBDELETEMANY_IPP
#define HATNROCKSDBDELETEMANY_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/common/runonscopeexit.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/namespace.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/indexquery.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/indexkeysearch.ipp>
#include <hatn/db/plugins/rocksdb/detail/querypartitions.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbdelete.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
struct DeleteManyT
{
    template <typename ModelT>
    Error operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        IndexQuery& query,
        AllocatorFactory* allocatorFactory,
        Transaction* tx
    ) const;
};
template <typename BufT>
constexpr DeleteManyT<BufT> DeleteMany{};

template <typename BufT>
template <typename ModelT>
Error DeleteManyT<BufT>::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        IndexQuery& idxQuery,
        AllocatorFactory* allocatorFactory,
        Transaction* tx
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("deletemany")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())

    // collect partitions for processing
    thread_local static common::pmr::FlatSet<std::shared_ptr<RocksdbPartition>> partitions{1, allocatorFactory->dataAllocator<std::shared_ptr<RocksdbPartition>>()};
    HATN_SCOPE_GUARD([](){partitions.clear();})
    index_key_search::queryPartitions(partitions,model,handler,idxQuery);

    // make snapshot
    ROCKSDB_NAMESPACE::ManagedSnapshot managedSnapchot{handler.p()->db};
    const auto* snapshot=managedSnapchot.snapshot();
    TtlMark::refreshCurrentTimepoint();

    Keys<BufT> keys{allocatorFactory->bytesAllocator()};
    using ttlIndexesT=TtlIndexes<modelType>;
    static ttlIndexesT ttlIndexes{};

    // callback on each key that deletes object with indexes
    auto keyCallback=[&model,&handler,&keys,&tx](RocksdbPartition* partition,
                                                const lib::string_view& topic,
                                                ROCKSDB_NAMESPACE::Slice* key,
                                                ROCKSDB_NAMESPACE::Slice*,
                                                Error& ec
                                            )
    {
        ec=DeleteObject<BufT>.doDelete(model,handler,partition,topic,*key,keys,ttlIndexes,tx);
        return !ec;
    };

    // process all partitions
    for (const auto& partition: partitions)
    {
        HATN_CTX_SCOPE_PUSH("partition",partition->range)

        // process all topics
        for (const auto& topic: idxQuery.topics())
        {
            HATN_CTX_SCOPE_PUSH("topic",topic)
            HATN_CTX_SCOPE_PUSH("index",idxQuery.index().name())

            index_key_search::Cursor<BufT> cursor(idxQuery.index().id(),topic,partition.get(),allocatorFactory);
            auto ec=index_key_search::nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory);
            HATN_CHECK_EC(ec)

            HATN_CTX_SCOPE_POP()
            HATN_CTX_SCOPE_POP()
        }

        HATN_CTX_SCOPE_POP()
    }

    // done
    return OK;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBDELETEMANY_IPP
