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

HATN_ROCKSDB_NAMESPACE_BEGIN

struct UpdateObject
{
    template <typename ModelT, typename NotFoundCbT>
    Error operator()(const ModelT&,
                     RocksdbHandler& handler,
                     RocksdbPartition* partition,
                     const ROCKSDB_NAMESPACE::Slice& key,
                     const update::Request& request,
                     AllocatorFactory* factory,
                     const NotFoundCbT& notFoundCb,
                     Transaction* tx
                    ) const
    {
        using modelType=std::decay_t<ModelT>;

        // transaction fn
        auto transactionFn=[&](Transaction* tx)
        {
            Error ec;
            auto rdbTx=RocksdbTransaction::native(tx);

            //! @todo get for update
            typename modelType::Type obj{factory};
            auto getForUpdate=[&]()
            {
                // handler.p()->transaction->GetForUpdate();
            };
            auto found=getForUpdate();

            // if not found then call not found callback
            if (!found)
            {
                auto tryUpdate=notFoundCb(ec);
                HATN_CHECK_EC(ec)
                if (tryUpdate)
                {
                    found=getForUpdate();
                    if (!found)
                    {
                        //! @todo Report error
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

            //! @todo save object

            //! @todo update indexes

            // done
            return Error{OK};
        };

        // invoke transaction
        return handler.transaction(transactionFn,tx,true);
    }
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBUPDATEOBJECT_IPP
