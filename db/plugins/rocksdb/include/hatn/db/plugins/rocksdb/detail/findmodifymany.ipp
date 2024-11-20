/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/findmodifymany.ipp
  *
  *   RocksDB handler to find and modify many objcts.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBFINDMODIFYMANY_IPP
#define HATNROCKSDBFINDMODIFYMANY_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/common/runonscopeexit.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/topic.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/indexquery.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/indexkeysearch.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/querypartitions.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbdelete.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct FindModifyManyT
{
    template <typename ModelT>
    Error operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& query,
        AllocatorFactory* allocatorFactory,
        const index_key_search::KeyHandlerFn& keyCallback
    ) const;
};
constexpr FindModifyManyT FindModifyMany{};

template <typename ModelT>
Error FindModifyManyT::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& idxQuery,
        AllocatorFactory* allocatorFactory,
        const index_key_search::KeyHandlerFn& keyCallback
    ) const
{
    HATN_CTX_SCOPE("findmodifymany")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())

    // collect partitions for processing
    index_key_search::Partitions partitions;
    index_key_search::queryPartitions(partitions,model,handler,idxQuery);

    // make snapshot
    ROCKSDB_NAMESPACE::ManagedSnapshot managedSnapchot{handler.p()->db};
    const auto* snapshot=managedSnapchot.snapshot();

    // process all partitions
    for (const auto& partition: partitions)
    {
        HATN_CTX_SCOPE_PUSH("partition",partition->range)

        // process all topics
        for (const auto& topic: idxQuery.query.topics())
        {
            HATN_CTX_SCOPE_PUSH("topic",topic.topic())
            HATN_CTX_SCOPE_PUSH("index",idxQuery.query.index()->name())

            index_key_search::Cursor cursor(idxQuery.modelIndexId,topic,partition.get());
            auto ec=index_key_search::nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory,
                        cursor.indexRangeFromSlice(),
                        cursor.indexRangeToSlice()
                    );
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

#endif // HATNROCKSDBFINDMODIFYMANY_IPP
