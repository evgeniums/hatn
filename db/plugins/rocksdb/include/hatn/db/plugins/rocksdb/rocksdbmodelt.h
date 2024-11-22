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

    IndexKeySlice key;
    bool exists;
    bool unique;

    IndexKeyUpdate(const std::string& indexName, IndexKeySlice key)
        : indexName(indexName),key(key),exists(false),unique(false)
    {}

    bool operator < (const IndexKeyUpdate& other) const noexcept
    {
        if (key.size()<other.key.size())
        {
            return true;
        }
        if (key.size()>other.key.size())
        {
            return false;
        }

        if (key.size()>0)
        {
            int cmp0=key[0].compare(other.key[0]);
            if (cmp0<0)
            {
                return true;
            }
            if (cmp0>0)
            {
                return false;
            }

            if (key.size()>1)
            {
                int cmp1=key[1].compare(other.key[1]);
                if (cmp1<0)
                {
                    return true;
                }
                if (cmp1>0)
                {
                    return false;
                }
            }
        }

        return false;
    }

    ROCKSDB_NAMESPACE::Slice keySlice() const noexcept
    {
        return key[0];
    }
};

using IndexKeyUpdateSet=common::FlatSet<IndexKeyUpdate>;

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
            IndexKeyUpdateSet& keys
        );

        static bool checkTtlFieldUpdated(const update::Request& request) noexcept;

    private:

        static std::multimap<FieldPath,UpdateIndexKeyExtractor<ObjectT>,FieldPathCompare> updateIndexKeyExtractors;
        static common::FlatSet<FieldPath,FieldPathCompare> ttlFields;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODELT_H
