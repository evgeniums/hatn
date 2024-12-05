/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdboperror.h
  *
  *   RocksDB operation error.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBOPERROR_H
#define HATNROCKSDBOPERROR_H

#include <hatn/common/error.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbOpError
{
    public:

        static const Error& ec();

        static void setEc(const Error& ec);

        static void resetEc();
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBOPERROR_H
