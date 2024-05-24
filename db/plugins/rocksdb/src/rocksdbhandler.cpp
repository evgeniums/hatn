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
    readOnly(transactionDb==nullptr)
{}

/********************** RocksdbHandler **************************/

//---------------------------------------------------------------

RocksdbHandler::RocksdbHandler(RocksdbHandler_p* pimpl):d(pimpl)
{}

//---------------------------------------------------------------

RocksdbHandler::~RocksdbHandler()
{}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
