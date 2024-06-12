/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbhandler.cpp
  *
  *   RocksDB database handler source.
  *
  */

/****************************************************************************/

#include <rocksdb/db.h>

#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler_p.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbHandler_p **************************/

//---------------------------------------------------------------

RocksdbHandler_p::RocksdbHandler_p(ROCKSDB_NAMESPACE::DB* db, ROCKSDB_NAMESPACE::TransactionDB* transactionDb)
    : db(db),
    transactionDb(transactionDb),
    readOnly(transactionDb==nullptr),
    inTransaction(false)
{}

/********************** RocksdbHandler **************************/

//---------------------------------------------------------------

RocksdbHandler::RocksdbHandler(RocksdbHandler_p* pimpl):d(pimpl)
{}

//---------------------------------------------------------------

RocksdbHandler::~RocksdbHandler()
{}

//---------------------------------------------------------------

Error RocksdbHandler::transaction(const TransactionFn& fn, bool relaxedIfInTransaction)
{
    if (d->inTransaction)
    {
        if (relaxedIfInTransaction)
        {
            return fn();
        }
        Assert(false,"Nested transactions not allowed");
        return commonError(CommonError::UNSUPPORTED);
    }

    auto tx=d->transactionDb->BeginTransaction(d->writeOptions,d->transactionOptions);
    if (tx==nullptr)
    {
        return dbError(DbError::TX_BEGIN_FAILED);
    }
    d->inTransaction=true;
    auto ec=fn();
    d->inTransaction=false;
    if (ec)
    {
        auto status=tx->Rollback();
        if (!status.ok())
        {
            //! @todo report error
        }
    }
    else
    {
        auto status=tx->Commit();
        if (!status.ok())
        {
            //! @todo construct error
            ec=dbError(DbError::TX_COMMIT_FAILED);
        }
    }
    return ec;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
