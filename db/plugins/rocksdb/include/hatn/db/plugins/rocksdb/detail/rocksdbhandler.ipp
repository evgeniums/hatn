/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbhandler.ipp
  *
  *   RocksDB database handler pimpl header.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBHANDLER_IPP
#define HATNROCKSDBHANDLER_IPP

#include <map>
#include <map>

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/common/result.h>
#include <hatn/common/datetime.h>
#include <hatn/common/daterange.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/flatmap.h>

#include <hatn/db/dberror.h>
#include <hatn/db/transaction.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbPartition **************************/

//---------------------------------------------------------------

struct RocksdbPartition
{
    enum class CfType : int
    {
        Collection,
        Index,
        Ttl,
        Blob
    };

    constexpr static const char* CollectionSuffix="collections";
    constexpr static const char* IndexSuffix="indexes";
    constexpr static const char* TtlSuffix="ttl";
    constexpr static const char* BlobSuffix="blobs";

    RocksdbPartition(
            const common::DateRange& range=common::DateRange{}
        ) : range(range)
    {}

    std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle> mainCf;
    std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle> blobCf;

    std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle> indexCf;
    std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle> ttlCf;
    common::DateRange range;

    ROCKSDB_NAMESPACE::ColumnFamilyHandle* dataCf(bool blob=false)
    {
        if (blob)
        {
            return blobCf.get();
        }
        return mainCf.get();
    }

    void setMainCf(ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf)
    {
        mainCf.reset(cf);
    }

    void setBlobCf(ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf)
    {
        blobCf.reset(cf);
    }

    static std::string columnFamilyName(CfType cfType, const common::DateRange& range)
    {
        switch (cfType)
        {
            case (CfType::Collection):
                return fmt::format("{}_{}",range.toString(),CollectionSuffix);
                break;

            case (CfType::Index):
                return fmt::format("{}_{}",range.toString(),IndexSuffix);
                break;

            case (CfType::Ttl):
                return fmt::format("{}_{}",range.toString(),TtlSuffix);
                break;

            case (CfType::Blob):
                return fmt::format("{}_{}",range.toString(),BlobSuffix);
                break;
        }

        Assert(false,"Unknown column family type");
        return std::string{};
    }
};

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END

namespace std
{

template <>
struct less<shared_ptr<HATN_ROCKSDB_NAMESPACE::RocksdbPartition>>
{
    bool operator()(const shared_ptr<HATN_ROCKSDB_NAMESPACE::RocksdbPartition>& l, const shared_ptr<HATN_ROCKSDB_NAMESPACE::RocksdbPartition>& r) const noexcept
    {
        return l->range < r->range;
    }

    bool operator()(const shared_ptr<HATN_ROCKSDB_NAMESPACE::RocksdbPartition>& l, uint32_t r) const noexcept
    {
        return l->range.value() < r;
    }

    bool operator()(uint32_t l, const shared_ptr<HATN_ROCKSDB_NAMESPACE::RocksdbPartition>& r) const noexcept
    {
        return l < r->range.value();
    }

    bool operator()(const HATN_COMMON_NAMESPACE::DateRange& l, const shared_ptr<HATN_ROCKSDB_NAMESPACE::RocksdbPartition>& r) const noexcept
    {
        return l < r->range;
    }

    bool operator()(const shared_ptr<HATN_ROCKSDB_NAMESPACE::RocksdbPartition>& l, const HATN_COMMON_NAMESPACE::DateRange& r) const noexcept
    {
        return l->range < r;
    }
};
}

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbHandler_p **************************/

//---------------------------------------------------------------

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbHandler_p
{
    public:

        RocksdbHandler_p(
            ROCKSDB_NAMESPACE::DB* db,
            ROCKSDB_NAMESPACE::TransactionDB* transactionDb=nullptr
        );

        ~RocksdbHandler_p();

        std::shared_ptr<RocksdbSchema> schema;
        ROCKSDB_NAMESPACE::DB* db;
        ROCKSDB_NAMESPACE::TransactionDB* transactionDb;

        //! @todo make sync=true for mobile platforms and configurable for other
        ROCKSDB_NAMESPACE::WriteOptions writeOptions;

        //! @todo make configurable deadline
        ROCKSDB_NAMESPACE::ReadOptions readOptions;

        ROCKSDB_NAMESPACE::TransactionOptions transactionOptions;

        ROCKSDB_NAMESPACE::ColumnFamilyOptions collColumnFamilyOptions;
        ROCKSDB_NAMESPACE::ColumnFamilyOptions indexColumnFamilyOptions;
        ROCKSDB_NAMESPACE::ColumnFamilyOptions ttlColumnFamilyOptions;
        ROCKSDB_NAMESPACE::ColumnFamilyOptions blobColumnFamilyOptions;

        bool readOnly;

        common::FlatSet<std::shared_ptr<RocksdbPartition>,std::less<std::shared_ptr<RocksdbPartition>>> partitions;
        std::shared_ptr<RocksdbPartition> defaultPartition;
        std::unique_ptr<ROCKSDB_NAMESPACE::ColumnFamilyHandle> defaultCf;

        mutable common::lib::shared_mutex partitionMutex;


        bool blobEnabled;

        Result<std::shared_ptr<RocksdbPartition>> partition(uint32_t partitionKey) const noexcept
        {
            common::lib::shared_lock<common::lib::shared_mutex> l{partitionMutex};

            auto it=partitions.find(partitionKey);
            if (it==partitions.end())
            {
                return dbError(DbError::PARTITION_NOT_FOUND);
            }
            return *it;
        }
};

//---------------------------------------------------------------

using Slice=ROCKSDB_NAMESPACE::Slice;

inline lib::string_view sliceView(const Slice& slice)
{
    return lib::string_view{slice.data(),slice.size()};
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBHANDLER_IPP
