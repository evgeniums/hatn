/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbhandler.h
  *
  *   RocksDB database handler header.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBHANDLER_H
#define HATNROCKSDBHANDLER_H

#include <memory>
#include <functional>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

using TransactionFn=std::function<Error ()>;

class RocksdbHandler_p;
class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbHandler
{
    public:

        RocksdbHandler(RocksdbHandler_p* pimpl);
        ~RocksdbHandler();

        RocksdbHandler(const RocksdbHandler&)=delete;
        RocksdbHandler& operator=(const RocksdbHandler&)=delete;
        RocksdbHandler(RocksdbHandler&&)=default;
        RocksdbHandler& operator=(RocksdbHandler&&)=default;

        RocksdbHandler_p* p() noexcept
        {
            return d.get();
        }

        Error transaction(const TransactionFn& fn, bool relaxedIfInTransaction=false);

    private:

        std::unique_ptr<RocksdbHandler_p> d;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBHANDLER_H
