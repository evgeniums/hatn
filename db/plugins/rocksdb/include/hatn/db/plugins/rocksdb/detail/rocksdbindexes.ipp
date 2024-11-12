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
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/common/format.h>
#include <hatn/common/meta/errorpredicate.h>

#include <hatn/validator/utils/foreach_if.hpp>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/namespace.h>
#include <hatn/db/objectid.h>
#include <hatn/db/dberror.h>
#include <hatn/db/update.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/saveuniquekey.h>
#include <hatn/db/plugins/rocksdb/savesingleindex.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

using IndexKeyHandlerFn=std::function<Error (const IndexKeySlice&)>;
class Indexes
{
    public:

        using bufT=BufT;

        Indexes(ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf, Keys& keys)
            : m_cf(cf),
              m_keys(keys)
        {}

        template <typename IndexT, typename UnitT>
        Error saveIndex(
            const IndexT& idx,
            ROCKSDB_NAMESPACE::Transaction* tx,
            const Namespace& ns,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const ROCKSDB_NAMESPACE::SliceParts& indexValue,
            UnitT* object,
            bool replace=false
            )
        {
            HATN_CTX_SCOPE("saveindex")

            // make and handle key
            return m_keys.makeIndexKey(ns.topic(),objectId,object,idx,
                [&](auto&& key){
                    auto ec=SaveSingleIndex(key,idx.unique(),m_cf,tx,indexValue,replace);
                    if (ec)
                    {
                        HATN_CTX_SCOPE_PUSH("idx_name",idx.name());
                    }
                    return ec;
                });
        }

        template <typename ModelT, typename UnitT>
        Error saveIndexes(
                ROCKSDB_NAMESPACE::Transaction* tx,
                const ModelT& model,
                const Namespace& ns,
                const ROCKSDB_NAMESPACE::Slice& objectId,
                const ROCKSDB_NAMESPACE::SliceParts& indexValue,
                UnitT* object,
                bool replace=false
            )
        {
            HATN_CTX_SCOPE("saveindexes")

            auto self=this;
            auto eachIndex=[&,self](auto&& idx, auto&&)
            {
                return self->saveIndex(idx,tx,ns,objectId,indexValue,object,replace);
            };
            return HATN_VALIDATOR_NAMESPACE::foreach_if(model.indexes,HATN_COMMON_NAMESPACE::error_predicate,eachIndex);
        }

        template <typename IndexT, typename UnitT>
        Error deleteIndex(
                const IndexT& idx,
                ROCKSDB_NAMESPACE::Transaction* tx,
                const lib::string_view& topic,
                const ROCKSDB_NAMESPACE::Slice& objectId,
                UnitT* object
            )
        {
            HATN_CTX_SCOPE("deleteindex")

            // make and handle key
            return m_keys.makeIndexKey(topic,objectId,object,idx,
                                       [&](auto&& key){

                                           ROCKSDB_NAMESPACE::SliceParts keySlices{&key[0],static_cast<int>(key.size())};

                                           // put op to transaction
                                           auto status=tx->Delete(m_cf,keySlices);
                                           if (!status.ok())
                                           {
                                               HATN_CTX_SCOPE_PUSH("idx_name",idx.name());
                                               return makeError(DbError::DELETE_INDEX_FAILED,status);
                                           }

                                           // done
                                           return Error{OK};
                                       }
                                       );
        }

        template <typename ModelT, typename UnitT>
        Error deleteIndexes(
                ROCKSDB_NAMESPACE::Transaction* tx,
                const ModelT& model,
                const lib::string_view& topic,
                const ROCKSDB_NAMESPACE::Slice& objectId,
                UnitT* object
            )
        {
            HATN_CTX_SCOPE("deleteindexes")

            auto eachIndex=[&,this](auto&& idx, auto&&)
            {
                return deleteIndex(idx,tx,topic,objectId,object);
            };
            return HATN_VALIDATOR_NAMESPACE::foreach_if(model.indexes,HATN_COMMON_NAMESPACE::error_predicate,eachIndex);
        }

    private:

        ROCKSDB_NAMESPACE::ColumnFamilyHandle* m_cf;
        Keys& m_keys;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBINDEXES_IPP
