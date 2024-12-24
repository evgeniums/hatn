/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbfindcb.ipp
  *
  *   RocksDB database template for finding objects with callback.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBFINDCB_IPP
#define HATNROCKSDBFINDCB_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/common/runonscopeexit.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/indexquery.h>
#include <hatn/db/find.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/indexkeysearch.h>
#include <hatn/db/plugins/rocksdb/rocksdbkeys.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbreadobject.ipp>
#include <hatn/db/plugins/rocksdb/detail/findmany.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct FindCbT
{
    template <typename ModelT>
    Error operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& query,
        const FindCb& cb,
        const AllocatorFactory* allocatorFactory,
        Transaction* tx,
        bool forUpdate
    ) const;
};
constexpr FindCbT FindCbOp{};

template <typename ModelT>
Error FindCbT::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& idxQuery,
        const FindCb& cb,
        const AllocatorFactory* factory,
        Transaction* tx,
        bool forUpdate
    ) const
{
    HATN_CTX_SCOPE("findcb")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())

    TtlMark::refreshCurrentTimepoint();
    size_t count=0;

    auto keyCallback=[&model,&handler,&idxQuery,&cb,&factory,&tx,&forUpdate,&count](
                          RocksdbPartition* partition,
                          const lib::string_view& topic,
                          ROCKSDB_NAMESPACE::Slice*,
                          ROCKSDB_NAMESPACE::Slice* keyValue,
                          Error& ec
                          )
    {
        auto objectKey=Keys::objectKeyFromIndexValue(*keyValue);
        auto r=readSingleObject(model,handler,partition,objectKey,factory,tx,forUpdate);
        if (r)
        {
            ec=r.takeError();
            return false;
        }

        if (count<idxQuery.query.offset())
        {
            count++;
            return true;
        }

        auto ok=cb(DbObject{r.takeValue(),topic},ec);
        if (!ok)
        {
            return false;
        }

        count++;
        if (count==(idxQuery.query.offset()+idxQuery.query.limit()))
        {
            return false;
        }
        return true;
    };
    return FindMany(model,handler,idxQuery,factory,keyCallback);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBFINDCB_IPP
