/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbcount.ipp
  *
  *   RocksDB database template for counting objects by index query.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBCOUNT_IPP
#define HATNROCKSDBCOUNT_IPP

#include <hatn/db/plugins/rocksdb/detail/findmany.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct CountT
{
    template <typename ModelT>
    Result<size_t> operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& query,
        AllocatorFactory* allocatorFactory
    ) const;
};
constexpr CountT Count{};

template <typename ModelT>
Result<size_t> CountT::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& idxQuery,
        AllocatorFactory* allocatorFactory
    ) const
{
    size_t count=0;
    auto keyCallback=[&model,&handler,&count,&idxQuery](RocksdbPartition*,
                                                      const lib::string_view&,
                                                      ROCKSDB_NAMESPACE::Slice*,
                                                      ROCKSDB_NAMESPACE::Slice*,
                                                      Error&
                                                      )
    {
        count++;
        if (idxQuery.query.limit()!=0 && (count==(idxQuery.query.limit()+idxQuery.query.offset())))
        {
            return false;
        }
        return true;
    };
    auto ec=FindMany(model,handler,idxQuery,allocatorFactory,keyCallback);
    HATN_CHECK_EC(ec)
    if (count>idxQuery.query.offset())
    {
        count-=idxQuery.query.offset();
    }
    else
    {
        count=0;
    }
    return count;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCOUNT_IPP
