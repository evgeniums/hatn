/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbtransaction.ipp
  *
  *   RocksDB transaction wrapper.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBTRANSACTION_IPP
#define HATNROCKSDBTRANSACTION_IPP

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/db/dberror.h>
#include <hatn/db/transaction.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>

#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbTransaction **************************/

//---------------------------------------------------------------

class RocksdbTransaction : public Transaction
{
    public:

        RocksdbTransaction()
        {}

        RocksdbTransaction(RocksdbHandler* handler)
            : m_native(handler->p()->transactionDb->BeginTransaction(handler->p()->writeOptions,handler->p()->transactionOptions))
        {}

        static ROCKSDB_NAMESPACE::Transaction* native(Transaction* tx) noexcept
        {
            return tx->derived<RocksdbTransaction>()->m_native.get();
        }

    private:

        std::unique_ptr<ROCKSDB_NAMESPACE::Transaction> m_native;
        friend class RocksdbHandler;
};

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBTRANSACTION_IPP
