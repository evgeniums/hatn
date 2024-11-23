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

#include <hatn/db/plugins/rocksdb/detail/findmodifymany.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct DeleteManyT
{
    template <typename ModelT>
    Result<size_t> operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& query,
        AllocatorFactory* allocatorFactory,
        Transaction* tx
    ) const;
};
constexpr DeleteManyT DeleteMany{};

template <typename ModelT>
Result<size_t> DeleteManyT::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& idxQuery,
        AllocatorFactory* allocatorFactory,
        Transaction* tx
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("deletemany")
    TtlMark::refreshCurrentTimepoint();
    Keys keys{allocatorFactory};
    using ttlIndexesT=TtlIndexes<modelType>;
    static ttlIndexesT ttlIndexes{};

//! @todo Bulk deletion in one transaction
#if 0
    auto transactionFn=[&](Transaction* tx)
    {
        auto keyCallback=[&model,&handler,&keys,&tx](RocksdbPartition* partition,
                                                          const lib::string_view& topic,
                                                          ROCKSDB_NAMESPACE::Slice* key,
                                                          ROCKSDB_NAMESPACE::Slice* keyValue,
                                                          Error& ec
                                                          )
        {
            auto objectKey=Keys::objectKeyFromIndexValue(*keyValue);
            return !DeleteObject.doDelete(model,handler,partition,topic,objectKey,keys,ttlIndexes,tx);
        };
        return FindModifyMany(model,handler,idxQuery,allocatorFactory,keyCallback);
    };
    return handler.transaction(transactionFn,tx,true);
#endif

    size_t count=0;
    auto keyCallback=[&model,&handler,&keys,&tx,&count](RocksdbPartition* partition,
                                                      const lib::string_view& topic,
                                                      ROCKSDB_NAMESPACE::Slice* key,
                                                      ROCKSDB_NAMESPACE::Slice* keyValue,
                                                      Error& ec
                                                      )
    {
        auto transactionFn=[&](Transaction* tx)
        {
            auto objectKey=Keys::objectKeyFromIndexValue(*keyValue);
            return DeleteObject.doDelete(model,handler,partition,topic,objectKey,keys,ttlIndexes,tx);
        };
        auto ok=!handler.transaction(transactionFn,tx,true);
        count++;
        return ok;
    };
    auto ec=FindModifyMany(model,handler,idxQuery,allocatorFactory,keyCallback);
    HATN_CHECK_EC(ec)

    return count;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBDELETEMANY_IPP
