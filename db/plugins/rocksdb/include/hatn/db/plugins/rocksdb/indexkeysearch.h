/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/indexkeysearch.h
  *
  *   RocksDB index keys search.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBINDEXKEYSEARCH_H
#define HATNROCKSDBINDEXKEYSEARCH_H

#include <cstddef>

#include "rocksdb/comparator.h"
#include "rocksdb/snapshot.h"

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/topic.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/query.h>
#include <hatn/db/indexquery.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace index_key_search
{

using Partitions=std::vector<std::shared_ptr<RocksdbPartition>>;

using KeyHandlerFn=std::function< bool (RocksdbPartition* partition,
                                    const lib::string_view& topic,
                                    ROCKSDB_NAMESPACE::Slice* key,
                                    ROCKSDB_NAMESPACE::Slice*,
                                    Error& ec
                                    )>;

struct HATN_ROCKSDB_SCHEMA_EXPORT IndexKey
{
    constexpr static const size_t FieldsOffset=2*sizeof(SeparatorCharC)+common::Crc32HexLength;

    IndexKey();

    IndexKey(
        ROCKSDB_NAMESPACE::Slice* k,
        ROCKSDB_NAMESPACE::Slice* v,
        const lib::string_view& topic,
        RocksdbPartition* p,
        AllocatorFactory* allocatorFactory
    );

    common::pmr::string key;
    common::pmr::string value;
    RocksdbPartition* partition;
    common::pmr::vector<ROCKSDB_NAMESPACE::Slice> keyParts;

    static lib::string_view keyPrefix(const lib::string_view& key, const lib::string_view& topic, size_t pos) noexcept;

    private:

        void fillKeyParts(const lib::string_view& topic);
};

struct IndexKeyCompare
{
    IndexKeyCompare() : idxQuery(nullptr)
    {}

    IndexKeyCompare(const ModelIndexQuery& idxQuery) : idxQuery(&idxQuery)
    {}

    inline bool operator ()(const IndexKey& l, const IndexKey& r) const noexcept
    {
        // compare key parts according to ordering of query fields
        for (size_t i=0;i<idxQuery->query.fields().size();i++)
        {
            if (i>=l.keyParts.size() || i>=r.keyParts.size())
            {
                return false;
            }

            const auto& field=idxQuery->query.field(i);
            const auto& leftPart=l.keyParts[i];
            const auto& rightPart=r.keyParts[i];
            int cmp{0};
            if (field.order==query::Order::Desc)
            {
                cmp=rightPart.compare(leftPart);
            }
            else
            {
                cmp=leftPart.compare(rightPart)<0;
            }
            if (cmp!=0)
            {
                return cmp<0;
            }
        }

        return false;
    }

    const ModelIndexQuery* idxQuery;
};

using IndexKeys=common::pmr::FlatSet<IndexKey,IndexKeyCompare>;

Result<IndexKeys> HATN_ROCKSDB_SCHEMA_EXPORT indexKeys(
    const ROCKSDB_NAMESPACE::Snapshot* snapshot,
    RocksdbHandler& handler,
    const ModelIndexQuery& idxQuery,
    const Partitions& partitions,
    AllocatorFactory* allocatorFactory,
    bool single,
    bool firstFieldPartitioned
);

struct Cursor
{
    Cursor(
            const lib::string_view& indexId,
            const Topic& topic_,
            RocksdbPartition* partition
        ) :
            pos(0),
            topic(topic_),
            partition(partition)
    {
        keyPrefix.append(topic);
        keyPrefix.append(SeparatorCharStr);
        keyPrefix.append(indexId);
    }

    void appendPrefix(const lib::string_view& prefixKey)
    {
        auto offset=keyPrefix.size();
        if (prefixKey.size()>offset)
        {
            keyPrefix.append(prefixKey.data()+offset,prefixKey.size()-offset);
        }
    }

    void restorePrefix(size_t prevSize)
    {
        keyPrefix.resize(prevSize);
    }

    KeyBuf keyPrefix;
    size_t pos;
    lib::string_view topic;

    RocksdbPartition* partition;
};

Error HATN_ROCKSDB_SCHEMA_EXPORT nextKeyField(
    Cursor& cursor,
    RocksdbHandler& handler,
    const ModelIndexQuery& idxQuery,
    const KeyHandlerFn& keyCallback,
    const ROCKSDB_NAMESPACE::Snapshot* snapshot,
    AllocatorFactory* allocatorFactory
);

} // namespace index_key_search

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBINDEXKEYSEARCH_H
