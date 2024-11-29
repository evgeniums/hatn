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
    HDU_FIELD(topic,TYPE_BOOL,5)
)

struct TtlIndexStub : public hana::false_
{
    template <typename ...Args>
    TtlIndexStub(Args&&...)
    {}
};

template <typename ModelT, typename ObjectT>
TtlMark makeTtlMark(
    const ModelT& model,
    const ObjectT* obj
    )
{    
    return TtlMark{model,obj};
}

template <typename ModelT, typename =hana::when<true>>
struct TtlIndexes
{
    using ttlT=TtlIndexStub;
    using modelType=ModelT;
    using objectT=typename ModelT::UnitType::type;

    static ttlT prepareTtl(
        ttlT&,
        const ModelT&,
        const ObjectId&,
        const common::DateRange&
        )
    {
        return ttlT{};
    }

    static void putTtlToTransaction(
        Error&,
        dataunit::WireBufSolid&,
        ROCKSDB_NAMESPACE::Transaction*,
        const ttlT&,
        const std::shared_ptr<RocksdbPartition>&,
        const ROCKSDB_NAMESPACE::Slice&,
        const ROCKSDB_NAMESPACE::Slice&
        )
    {}

    static void deleteTtlIndex(
            Error&,
            ROCKSDB_NAMESPACE::Transaction*,
            RocksdbPartition*,
            const ROCKSDB_NAMESPACE::Slice&,
            const ROCKSDB_NAMESPACE::Slice&
        )
    {}

    static void saveTtlIndex(
            Error&,
            const ModelT&,
            const objectT*,
            dataunit::WireBufSolid&,
            ROCKSDB_NAMESPACE::Transaction*,
            RocksdbPartition*,
            const ROCKSDB_NAMESPACE::Slice&,
            AllocatorFactory*
        )
    {
    }

    static void saveTtlIndexWithMark(
        const TtlMark&,
        Error&,
        const ModelT&,
        const objectT*,
        dataunit::WireBufSolid&,
        ROCKSDB_NAMESPACE::Transaction*,
        RocksdbPartition*,
        const ROCKSDB_NAMESPACE::Slice&,
        AllocatorFactory*
        )
    {
    }

    static void saveTtlIndexWithMark(
        const ROCKSDB_NAMESPACE::Slice&,
        Error&,
        const ModelT&,
        const objectT*,
        dataunit::WireBufSolid&,
        ROCKSDB_NAMESPACE::Transaction*,
        RocksdbPartition*,
        const ROCKSDB_NAMESPACE::Slice&,
        AllocatorFactory*
        )
    {
    }
};

template <typename ModelT>
struct TtlIndexes<ModelT,hana::when<decltype(ModelT::isTtlEnabled())::value>>
{
    using ttlT=ttl_index::type;
    using modelType=ModelT;
    using objectT=typename ModelT::UnitType::type;

    static void prepareTtl(
            ttl_index::type& ttlIndex,
            const ModelT& model,
            const ObjectId& objectId,
            const common::DateRange& dateRange
        )
    {
        HATN_CTX_SCOPE("ttlidxprepare")

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

        // set topic flag
        ttlIndex.field(ttl_index::topic).set(model.canBeTopic());
    }

    static void putTtlToTransaction(
            Error& ec,
            dataunit::WireBufSolid& buf,
            ROCKSDB_NAMESPACE::Transaction* tx,
            const ttl_index::type& ttlIndex,
            RocksdbPartition* partition,
            const ROCKSDB_NAMESPACE::Slice& objectIdSlice,
            const ROCKSDB_NAMESPACE::Slice& ttlMark
        )
    {
        HATN_CTX_SCOPE("rdbttlindexput")

        // serialize
        dataunit::io::serialize(ttlIndex,buf,ec);
        if(ec)
        {
            HATN_CTX_SCOPE_ERROR("serialize ttl index");
        }
        else
        {
            // put to transaction
            std::array<ROCKSDB_NAMESPACE::Slice,2> keyParts{ttlMark,objectIdSlice};
            ROCKSDB_NAMESPACE::SliceParts keySlices{&keyParts[0],static_cast<int>(keyParts.size())};
            ROCKSDB_NAMESPACE::Slice valueSlice{buf.mainContainer()->data(),buf.mainContainer()->size()};
            ROCKSDB_NAMESPACE::SliceParts valueSlices{&valueSlice,1};
            auto status=tx->Put(partition->ttlCf.get(),keySlices,valueSlices);
            if (!status.ok())
            {
                HATN_CTX_SCOPE_ERROR("tx-ttl-index");
                copyRocksdbError(ec,DbError::SAVE_TTL_INDEX_FAILED,status);
            }
        }
    }

    static void deleteTtlIndex(
            Error& ec,
            ROCKSDB_NAMESPACE::Transaction* tx,
            RocksdbPartition* partition,
            const ROCKSDB_NAMESPACE::Slice& objectIdSlice,
            const ROCKSDB_NAMESPACE::Slice& ttlMark
        )
    {
        std::array<ROCKSDB_NAMESPACE::Slice,2> keyParts{ttlMark,objectIdSlice};
        ROCKSDB_NAMESPACE::SliceParts keySlices{&keyParts[0],static_cast<int>(keyParts.size())};
        auto status=tx->Delete(partition->ttlCf.get(),keySlices);
        if (!status.ok())
        {
            HATN_CTX_SCOPE_ERROR("tx-ttl-index");
            copyRocksdbError(ec,DbError::DELETE_TTL_INDEX_FAILED,status);
        }
    }

    static void saveTtlIndex(
            Error& ec,
            const ModelT& model,
            const objectT* obj,
            dataunit::WireBufSolid& buf,
            ROCKSDB_NAMESPACE::Transaction* tx,
            RocksdbPartition* partition,
            const ROCKSDB_NAMESPACE::Slice& objectIdSlice,
            AllocatorFactory* allocatorFactory
        )
    {
        saveTtlIndexWithMark(makeTtlMark(model,obj),ec,model,obj,buf,tx,partition,objectIdSlice,allocatorFactory);
    }

    static void saveTtlIndexWithMark(
            const ROCKSDB_NAMESPACE::Slice& ttlMark,
            Error& ec,
            const ModelT& model,
            const objectT* obj,
            dataunit::WireBufSolid& buf,
            ROCKSDB_NAMESPACE::Transaction* tx,
            RocksdbPartition* partition,
            const ROCKSDB_NAMESPACE::Slice& objectIdSlice,
            AllocatorFactory* allocatorFactory
        )
    {
        ttlT ttlIndex{allocatorFactory};
        prepareTtl(ttlIndex,model,obj->field(object::_id).value(),partition->range);
        putTtlToTransaction(ec,buf,tx,ttlIndex,partition,objectIdSlice,ttlMark);
    }

    static void saveTtlIndexWithMark(
        const TtlMark& ttlMark,
        Error& ec,
        const ModelT& model,
        const objectT* obj,
        dataunit::WireBufSolid& buf,
        ROCKSDB_NAMESPACE::Transaction* tx,
        RocksdbPartition* partition,
        const ROCKSDB_NAMESPACE::Slice& objectIdSlice,
        AllocatorFactory* allocatorFactory
        )
    {
        return saveTtlIndexWithMark(ttlMark.slice(),ec,model,obj,buf,tx,partition,objectIdSlice,allocatorFactory);
    }
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBTTLINDEXES_IPP
