/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/rocksdbindexes.ipp
  *
  *   RocksDB indexes processing.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBINDEXES_IPP
#define HATNROCKSDBINDEXES_IPP

#include <rocksdb/db.h>

#include <hatn/common/format.h>
#include <hatn/common/meta/errorpredicate.h>

#include <hatn/validator/utils/foreach_if.hpp>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/namespace.h>
#include <hatn/db/objectid.h>
#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbkeys.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
class Indexes
{
    public:

        using bufT=BufT;

        Indexes(ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf, Keys<BufT>& keys)
            : m_cf(cf),
              m_keys(keys)
        {}

        template <typename ModelT, typename UnitT>
        Error saveIndexes(
                ROCKSDB_NAMESPACE::WriteBatch& batch,
                const ModelT& model,
                const Namespace& ns,
                const ROCKSDB_NAMESPACE::Slice& objectId,
                const ROCKSDB_NAMESPACE::SliceParts& objectKey,
                UnitT* object
            )
        {
            HATN_CTX_SCOPE("saveindexes")

            auto eachIndex=[&,this](auto&& idx, auto&&)
            {
                auto key=m_keys.makeIndexKey(ns,objectId,object,idx);
                ROCKSDB_NAMESPACE::SliceParts keySlices{&key[0],static_cast<int>(key.size())};
                auto status=batch.Put(keySlices,objectKey);
                if (!status.ok())
                {
                    HATN_CTX_SCOPE_ERROR("batch-idx");
                    HATN_CTX_SCOPE_PUSH("idx_name",idx.name());
                    return makeError(DbError::SAVE_INDEX_FAILED,status);
                }
                return Error{OK};
            };
            return HATN_VALIDATOR_NAMESPACE::foreach_if(model.indexes,HATN_COMMON_NAMESPACE::error_predicate,eachIndex);
        }

    private:

        ROCKSDB_NAMESPACE::ColumnFamilyHandle* m_cf;
        Keys<BufT>& m_keys;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBINDEXES_IPP
