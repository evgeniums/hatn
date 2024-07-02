/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/ttlindexes.ipp
  *
  *   RocksDB TTL indexes processing.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBTTLINDEXES_IPP
#define HATNROCKSDBTTLINDEXES_IPP

#include <rocksdb/db.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/namespace.h>
#include <hatn/db/objectid.h>
#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

HDU_UNIT_WITH(ttl_index,(HDU_BASE(object)),
    HDU_FIELD(ref_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(ref_model_id,HDU_TYPE_FIXED_STRING(8),2)
    HDU_FIELD(date_range,TYPE_DATE_RANGE,3)
    HDU_REPEATED_FIELD(ref_indexes,TYPE_STRING,4)
    HDU_FIELD(topic,TYPE_BOOL,5)
)

struct TtlIndexStub : public hana::false_
{
    template <typename ...Args>
    TtlIndexStub(Args&&...)
    {}
};

template <typename ModelT, typename =hana::when<true>>
struct TtlIndexes
{
    using ttlT=TtlIndexStub;

    static ttlT prepareTtl(
        ttlT&,
        const ModelT&,
        const ObjectId&,
        const common::DateRange&
        )
    {
        return ttlT{};
    }

    static void putTtlToBatch(
        Error,
        dataunit::WireBufSolid&,
        ROCKSDB_NAMESPACE::WriteBatch&,
        const ttlT&,
        const std::shared_ptr<RocksdbPartition>&,
        const ROCKSDB_NAMESPACE::Slice&,
        const ROCKSDB_NAMESPACE::Slice&
        )
    {}

    static void addIndexKeyToTtl(
        ttlT&,
        const IndexKeyT&
        )
    {}

    static IndexKeyHandlerFn makeIndexKeyCb(
            ttlT&
        )
    {
        return IndexKeyHandlerFn{};
    }
};

template <typename ModelT>
struct TtlIndexes<ModelT,hana::when<decltype(ModelT::isTtlEnabled())::value>>
{
    using ttlT=ttl_index::type;
    using modelType=ModelT;

    static void prepareTtl(
            ttl_index::type& ttlIndex,
            const ModelT& model,
            const ObjectId& objectId,
            const common::DateRange& dateRange
        )
    {
        HATN_CTX_SCOPE("rocksdbttlindexprepare")

        initObject(ttlIndex);

        // set object ID
        ttlIndex.field(ttl_index::ref_id).set(objectId);

        // set model ID
        ttlIndex.field(ttl_index::ref_model_id).set(model.modelIdStr());

        // set date range
        if (!dateRange.isNull())
        {
            ttlIndex.field(ttl_index::date_range).set(dateRange);
        }

        // reserve space for indexes
        ttlIndex.field(ttl_index::ref_indexes).reserve(model.indexIds.size());

        // set topic flag
        ttlIndex.field(ttl_index::topic).set(model.canBeTopic());
    }

    static void putTtlToBatch(
            Error ec,
            dataunit::WireBufSolid& buf,
            ROCKSDB_NAMESPACE::WriteBatch& batch,
            const ttl_index::type& ttlIndex,
            const std::shared_ptr<RocksdbPartition>& partition,
            const ROCKSDB_NAMESPACE::Slice& objectIdSlice,
            const ROCKSDB_NAMESPACE::Slice& ttlMark
        )
    {
        HATN_CTX_SCOPE("rocksdbttlindexput")

        // serialize
        dataunit::io::serialize(ttlIndex,buf,ec);
        if(ec)
        {
            HATN_CTX_SCOPE_ERROR("serialize ttl index");
        }
        else
        {
            // put to batch
            std::array<ROCKSDB_NAMESPACE::Slice,2> keyParts{ttlMark,objectIdSlice};
            ROCKSDB_NAMESPACE::SliceParts keySlices{&keyParts[0],static_cast<int>(keyParts.size())};
            ROCKSDB_NAMESPACE::Slice valueSlice{buf.mainContainer()->data(),buf.mainContainer()->size()};
            ROCKSDB_NAMESPACE::SliceParts valueSlices{&valueSlice,1};
            auto status=batch.Put(partition->ttlCf.get(),keySlices,valueSlices);
            if (!status.ok())
            {
                HATN_CTX_SCOPE_ERROR("batch-ttl-index");
                setRocksdbError(ec,DbError::SAVE_TTL_INDEX_FAILED,status);
            }
        }
    }

    static void addIndexKeyToTtl(
            ttl_index::type& ttlIndex,
            const IndexKeyT& key
        )
    {
        auto& indexes=ttlIndex.field(ttl_index::ref_indexes);
        indexes.resize(indexes.size()+1);
        auto buf=indexes.at(indexes.size()-1).buf();
        for (auto&& keyPart: key)
        {
            buf->append(keyPart);
        }
    }

    static IndexKeyHandlerFn makeIndexKeyCb(
            ttlT& ttlIndex
        )
    {
        return [&ttlIndex](const IndexKeyT& idxKey)
        {
            addIndexKeyToTtl(ttlIndex,idxKey);
        };
    }
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBTTLINDEXES_IPP
