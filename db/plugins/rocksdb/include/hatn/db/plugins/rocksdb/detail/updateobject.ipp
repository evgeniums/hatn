/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/updateobject.ipp
  *
  *   RocksDB object update.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBUPDATEOBJECT_IPP
#define HATNROCKSDBUPDATEOBJECT_IPP

#include <rocksdb/db.h>
#include "rocksdb/utilities/write_batch_with_index.h"

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/dberror.h>
#include <hatn/db/update.h>
#include <hatn/db/ipp/updateunit.ipp>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbtransaction.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbcreateobject.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct UpdateObject
{
    template <typename ModelT, typename NotFoundCbT>
    Error operator()(const ModelT&,
                     RocksdbModel*,
                     RocksdbHandler& handler,
                     RocksdbPartition* partition,
                     const ROCKSDB_NAMESPACE::Slice& key,
                     const update::Request& request,
                     AllocatorFactory* factory,
                     const NotFoundCbT& notFoundCb,
                     Transaction* tx
                    ) const
    {
        HATN_CTX_SCOPE("rocksdbupdateobject")
        using modelType=std::decay_t<ModelT>;

        // transaction fn
        auto transactionFn=[&](Transaction* tx)
        {
            Error ec;
            auto rdbTx=RocksdbTransaction::native(tx);

            // get for update
            ROCKSDB_NAMESPACE::PinnableSlice readSlice;
            typename modelType::Type oldObj{factory};
            auto getForUpdate=[&]()
            {
                auto status=rdbTx->GetForUpdate(handler.p()->readOptions,partition->collectionCf.get(),key,&readSlice);
                if (!status.ok())
                {
                    if (status.code()==ROCKSDB_NAMESPACE::Status::kNotFound)
                    {
                        return false;
                    }

                    HATN_CTX_SCOPE_ERROR("get");
                    return makeError(DbError::READ_FAILED,status);
                }

                // check if object expired
                TtlMark::refreshCurrentTimepoint();
                if (TtlMark::isExpired(readSlice))
                {
                    return false;
                }

                // deserialize object
                auto objSlice=TtlMark::stripTtlMark(readSlice);
                dataunit::WireBufSolid buf{objSlice.data(),objSlice.size(),true};
                if (!dataunit::io::deserialize(&obj,buf,ec))
                {
                    HATN_CTX_SCOPE_ERROR("deserialize");
                    return ec;
                }
            };
            auto found=getForUpdate();
            HATN_CHECK_EC(ec)

            // if not found then call not found callback
            if (!found)
            {
                auto tryUpdate=notFoundCb(ec);
                HATN_CHECK_EC(ec)
                if (tryUpdate)
                {
                    HATN_CTX_SCOPE("tryupdate");
                    found=getForUpdate();
                    if (!found)
                    {
                        return ec;
                    }
                }
                else
                {
                    return Error{OK};
                }
            }

            // apply request to object
            update::ApplyRequest(&obj,request);

            // serialize object
            dataunit::WireBufSolid buf{allocatorFactory};
            auto ec=serializeObject(obj,buf);
            HATN_CHECK_EC(ec)

            // save object
            auto ttlMark=TtlMark::ttlMark(readSlice);
            ec=saveObject(rdbTx,partition,key,buf,ttlMark);
            HATN_CHECK_EC(ec)

            //! @todo update indexes
            IndexKeyUpdateSet oldKeys;
            IndexKeyUpdateSet newKeys;

            // done
            return Error{OK};
        };

        // invoke transaction
        return handler.transaction(transactionFn,tx,true);
    }
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBUPDATEOBJECT_IPP
