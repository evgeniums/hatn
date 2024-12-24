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

#include <hatn/db/plugins/rocksdb/detail/findmany.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct DeleteManyT
{
    template <typename ModelT>
    Result<size_t> operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& query,
        const AllocatorFactory* allocatorFactory,
        bool bulk,
        Transaction* tx
    ) const;
};
constexpr DeleteManyT DeleteMany{};

template <typename ModelT>
Result<size_t> DeleteManyT::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& idxQuery,
        const AllocatorFactory* allocatorFactory,
        bool bulk,
        Transaction* tx
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("deletemany")
    TtlMark::refreshCurrentTimepoint();
    Keys keys{allocatorFactory};
    using ttlIndexesT=TtlIndexes<modelType>;
    static ttlIndexesT ttlIndexes{};

    size_t count=0;
    if (bulk)
    {
        auto transactionFn=[&](Transaction* tx)
        {
            auto keyCallback=[&model,&handler,&keys,&tx,&count](RocksdbPartition* partition,
                                                              const lib::string_view& topic,
                                                              ROCKSDB_NAMESPACE::Slice*,
                                                              ROCKSDB_NAMESPACE::Slice* keyValue,
                                                              Error& ec
                                                              )
            {
                auto objectKey=Keys::objectKeyFromIndexValue(*keyValue);
                ec=DeleteObject.doDelete(model,handler,partition,topic,objectKey,keys,ttlIndexes,tx);
                if (!ec)
                {
                    count++;
                }
                return !ec;
            };
            return FindMany(model,handler,idxQuery,allocatorFactory,keyCallback);
        };
        auto ec=handler.transaction(transactionFn,tx,true);
        HATN_CHECK_EC(ec)
        return count;
    }

    auto keyCallback=[&model,&handler,&keys,&tx,&count](RocksdbPartition* partition,
                                                      const lib::string_view& topic,
                                                      ROCKSDB_NAMESPACE::Slice*,
                                                      ROCKSDB_NAMESPACE::Slice* keyValue,
                                                      Error& ec
                                                      )
    {
        auto transactionFn=[&](Transaction* tx)
        {
            auto objectKey=Keys::objectKeyFromIndexValue(*keyValue);
            return DeleteObject.doDelete(model,handler,partition,topic,objectKey,keys,ttlIndexes,tx);
        };
        ec=handler.transaction(transactionFn,tx,true);
        if (!ec)
        {
            count++;
        }
        return !ec;
    };
    auto ec=FindMany(model,handler,idxQuery,allocatorFactory,keyCallback);
    HATN_CHECK_EC(ec)

    return count;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBDELETEMANY_IPP
