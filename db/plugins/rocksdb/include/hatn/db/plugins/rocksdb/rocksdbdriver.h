/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file db/plugins/rocksdb/rocksdbdriver.h
  *
  *   Defines hatn database driver for RocksDB backend.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBDRIVER_H
#define HATNROCKSDBDRIVER_H

#include <hatn/db/config.h>

#ifdef NO_DYNAMIC_HATN_PLUGINS
#include <hatn/db/plugins/rocksdb/config.h>
#endif

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_ROCKSDB_EXPORT
#   ifdef BUILD_HATN_ROCKSDB
#       define HATN_ROCKSDB_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_ROCKSDB_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_ROCKSDB_NAMESPACE_BEGIN namespace hatn { namespace db { namespace rocksdbdriver {
#define HATN_ROCKSDB_NAMESPACE_END }}}

#define HATN_ROCKSDB_NAMESPACE hatn::db::rocksdbdriver
#define HATN_ROCKSDB_NS rocksdbdriver
#define HATN_ROCKSDB_USING using namespace hatn::db::rocksdbdriver;

#endif // HATNROCKSDBDRIVER_H
