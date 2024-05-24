/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbschemadef.h
  *
  *   Macros for RocksDB database schema lib.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBSCHEMADEF_H
#define HATNROCKSDBSCHEMADEF_H

#include <hatn/common/classuid.h>
#include <hatn/db/db.h>
#include <hatn/dataunit/dataunit.h>

#include <hatn/db/plugins/rocksdb/rocksdbdriver.h>

#ifdef _WIN32
#    ifndef HATN_ROCKSDB_SCHEMA_EXPORT
#        ifdef BUILD_HATN_ROCKSDB_SCHEMA
#            define HATN_ROCKSDB_SCHEMA_EXPORT __declspec(dllexport)
#        else
#            define HATN_ROCKSDB_SCHEMA_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_ROCKSDB_SCHEMA_EXPORT
#endif

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace common=HATN_COMMON_NAMESPACE;
namespace db=HATN_DB_NAMESPACE;
using CUID_TYPE=common::CUID_TYPE;

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSCHEMADEF_H
