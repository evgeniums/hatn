/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdboperror.cpp
  *
  *   RocksDB operation error.
  *
  */

/****************************************************************************/

#include <hatn/db/plugins/rocksdb/rocksdboperror.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace {
static thread_local Error Ec;
}

//---------------------------------------------------------------

const Error& RocksdbOpError::ec()
{
    return Ec;
}

//---------------------------------------------------------------

void RocksdbOpError::setEc(const Error& ec)
{
    Ec=ec;
}

//---------------------------------------------------------------

void RocksdbOpError::resetEc()
{
    Ec.reset();
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
