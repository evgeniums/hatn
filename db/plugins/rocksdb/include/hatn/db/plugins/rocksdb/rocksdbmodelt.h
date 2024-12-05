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

#include <hatn/common/flatmap.h>
#include <hatn/common/stdwrappers.h>

#include <hatn/db/model.h>
#include <hatn/db/update.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct IndexKeyUpdate
{
    lib::string_view indexName;

    KeyBuf key;
    bool exists;
    bool unique;
    size_t slice2Offset;
    bool replace;

    IndexKeyUpdate(const std::string& indexName, IndexKeySlice k, bool unique)
        : indexName(indexName),exists(false),unique(unique),slice2Offset(k[0].size()),replace(false)
    {
        key.append(k[0].data(),k[0].size());
        key.append(k[1].data(),k[1].size());
    }

    ROCKSDB_NAMESPACE::Slice keySlice() const noexcept
    {
        return ROCKSDB_NAMESPACE::Slice{key.data(),key.size()};
    }

    IndexKeySlice keySlices() const noexcept
    {
        IndexKeySlice slices;
        slices[0]=ROCKSDB_NAMESPACE::Slice{key.data(),slice2Offset};
        slices[1]=ROCKSDB_NAMESPACE::Slice{key.data()+slice2Offset,key.size()-slice2Offset};
        return slices;
    }
};

struct IndexKeyUpdateCmp
{
    bool operator () (const IndexKeyUpdate& l, const IndexKeyUpdate& r) const noexcept
    {
        return l.key<r.key;
    }
};

using IndexKeyUpdateSet=common::pmr::set<IndexKeyUpdate,IndexKeyUpdateCmp>;

template <typename ObjectT>
using UpdateIndexKeyExtractor=
            std::function<void (
                    Keys& keysHandler,
                    const lib::string_view& topic,
                    const ROCKSDB_NAMESPACE::Slice& objectId,
                    const ObjectT* obj,
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
            Keys& keysHandler,
            const update::Request& request,
            const lib::string_view& topic,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const ObjectT* object,
            IndexKeyUpdateSet& keys,
            bool ttlUpdated=false
        );

        static bool checkTtlFieldUpdated(const update::Request& request) noexcept;

    private:

        static std::multimap<FieldPath,UpdateIndexKeyExtractor<ObjectT>,FieldPathCompare> updateIndexKeyExtractors;
        static common::FlatSet<FieldPath,FieldPathCompare> ttlFields;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODELT_H
