/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbmodelt.h
  *
  *   RocksDB database model.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBMODELT_H
#define HATNROCKSDBMODELT_H

#include <map>
#include <memory>
#include <functional>

#include <hatn/db/model.h>
#include <hatn/db/update.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct IndexKeyUpdate
{
    IndexKeyT key;
    bool exists;

    IndexKeyUpdate(IndexKeyT key)
        : key(key),exists(false)
    {}
};

using IndexKeyUpdateSet=common::FlatSet<IndexKeyUpdate>;

template <typename ObjectT>
using UpdateIndexKeyExtractor=
    std::function<Error(
        const lib::string_view& topic,
        const ROCKSDB_NAMESPACE::Slice& objectId,
        const ObjectT* object,
        IndexKeyUpdateSet& keys
        )>;

template <typename ModelT>
class RocksdbModelT
{
    public:

        using ObjectT=typename ModelT::Type;

        template <typename T>
        static void init(const T& model);

        static void updatingKeys(
            Keys<>& keysHandler,
            const update::Request& request,
            const lib::string_view& topic,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const ObjectT* object,
            IndexKeyUpdateSet& keys
        );

    private:

        static std::multimap<std::string,UpdateIndexKeyExtractor<ObjectT>> updateIndexKeyExtractors;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODELT_H
