/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file db/plugins/rocksdb/rocksdbplugin.cpp
  *
  *   Implementation of database plugin for RocksDB backend.
  *
  */

/****************************************************************************/

#include <hatn/db/plugins/rocksdb/rocksdbclient.h>
#include <hatn/db/plugins/rocksdb/rocksdbplugin.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

//---------------------------------------------------------------

std::shared_ptr<Client> RocksdbPlugin::makeClient() const
{
    return std::make_shared<RocksdbClient>();
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END

#ifndef NO_DYNAMIC_HATN_PLUGINS
    HATN_PLUGIN_EXPORT(HATN_ROCKSDB_NAMESPACE::RocksdbPlugin)
#endif
