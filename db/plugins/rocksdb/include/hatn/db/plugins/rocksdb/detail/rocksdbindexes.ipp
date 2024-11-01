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

#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

using IndexKeyHandlerFn=std::function<Error (const IndexKeyT&)>;

template <typename BufT>
class Indexes
{
    public:

        using bufT=BufT;

        Indexes(ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf, Keys<BufT>& keys)
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
            IndexKeyHandlerFn keyCallback=IndexKeyHandlerFn{},
            bool replace=false
            )
        {
            HATN_CTX_SCOPE("saveindex")
            ROCKSDB_NAMESPACE::Status status;

            // make and handle key
            return m_keys.makeIndexKey(ns.topic(),objectId,object,idx,
                [&](auto&& key){

                   ROCKSDB_NAMESPACE::SliceParts keySlices{&key[0],static_cast<int>(key.size())};

                   // put index to transaction
                   bool put=true;
                   if constexpr (std::decay_t<IndexT>::unique())
                   {
                       if (!replace)
                       {
                           HATN_CTX_SCOPE_PUSH("idx_op","merge");
                           put=false;
                           std::string bufk;
                           ROCKSDB_NAMESPACE::Slice k{keySlices,&bufk};
                           std::string bufv;
                           ROCKSDB_NAMESPACE::Slice v{indexValue,&bufv};
                           status=tx->Merge(m_cf,k,v);
                       }
                   }
                   if (put)
                   {
                       HATN_CTX_SCOPE_PUSH("idx_op","put");
                       status=tx->Put(m_cf,keySlices,indexValue);
                   }
                   if (!status.ok())
                   {
                       HATN_CTX_SCOPE_PUSH("idx_name",idx.name());
                       return makeError(DbError::SAVE_INDEX_FAILED,status);
                   }

                   // done
                   if (keyCallback)
                   {
                       auto ec=keyCallback(key);
                       HATN_CHECK_EC(ec)
                   }
                   return Error{OK};
                }
            );
        }

        template <typename ModelT, typename UnitT>
        Error saveIndexes(
                ROCKSDB_NAMESPACE::Transaction* tx,
                const ModelT& model,
                const Namespace& ns,
                const ROCKSDB_NAMESPACE::Slice& objectId,
                const ROCKSDB_NAMESPACE::SliceParts& indexValue,
                UnitT* object,
                IndexKeyHandlerFn keyCallback=IndexKeyHandlerFn{},
                bool replace=false
            )
        {
            HATN_CTX_SCOPE("saveindexes")

            auto eachIndex=[&,this](auto&& idx, auto&&)
            {
                return saveIndex(idx,tx,ns,objectId,indexValue,object,keyCallback,replace);
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

                                           // put index to transaction
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
        Keys<BufT>& m_keys;
};

struct IndexKeyUpdate
{
    IndexKeyT key;
    bool exists;

    IndexKeyUpdate(IndexKeyT key)
        : key(key),exists(false)
    {}
};

using IndexKeyUpdateSet=common::FlatSet<IndexKeyT>;

struct UpdateIndexKeyExtractor
{
    std::function<Error(
        const lib::string_view& topic,
        const ROCKSDB_NAMESPACE::Slice& objectId,
        const dataunit::Unit* object,
        IndexKeyUpdateSet& keys
    )> fn;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBINDEXES_IPP
