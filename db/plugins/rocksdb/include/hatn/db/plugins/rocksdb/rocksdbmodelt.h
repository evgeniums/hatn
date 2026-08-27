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
    bool isSet;

    IndexKeyUpdate(const std::string& indexName, IndexKeySlice k, bool unique, bool isSet)
        : indexName(indexName),exists(false),unique(unique),slice2Offset(k[0].size()),replace(false),isSet(isSet)
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

    //! The slice actually written to rocksdb for this entry, for use when RETRACTING it (a
    //! stale/superseded key found during an update's before/after diff -- see rocksdbupdate.ipp).
    //!
    //! Must mirror SaveSingleIndex's own write-mode decision (savesingleindex.cpp) exactly: a
    //! unique index writes ONLY slice[0] (the field-value prefix) via Merge, deliberately
    //! omitting slice[1] (the object id) so that two objects with equal field values collide on
    //! the same rocksdb key and the merge can reject the duplicate -- that omission is the
    //! entire uniqueness mechanism. A non-unique index (or a unique index whose value was
    //! unset, `isSet==false`, at write time) writes both slices via Put.
    //!
    //! keySlice() (both slices, unconditionally) looked like the right thing to delete with but
    //! is wrong for a unique+set entry: it names a key that was never written, so the actual
    //! (slice[0]-only) entry survives every update that changes it, permanently -- a query
    //! reading through the index still finds it, now returning the row's since-changed content.
    ROCKSDB_NAMESPACE::Slice deleteKeySlice() const noexcept
    {
        if (unique && isSet)
        {
            return ROCKSDB_NAMESPACE::Slice{key.data(),slice2Offset};
        }
        return keySlice();
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
