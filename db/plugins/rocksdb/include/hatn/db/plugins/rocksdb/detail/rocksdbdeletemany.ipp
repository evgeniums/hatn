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

template <typename BufT>
struct DeleteManyT
{
    template <typename ModelT>
    Error operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& query,
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
        const ModelIndexQuery& idxQuery,
        AllocatorFactory* allocatorFactory,
        Transaction* tx
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("deletemany")
    TtlMark::refreshCurrentTimepoint();
    Keys<BufT> keys{allocatorFactory->bytesAllocator()};
    using ttlIndexesT=TtlIndexes<modelType>;
    static ttlIndexesT ttlIndexes{};

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

    return FindModifyMany<BufT>(model,handler,idxQuery,allocatorFactory,keyCallback);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBDELETEMANY_IPP
