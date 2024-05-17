/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file db/plugins/rocksdb/rocksdbplugin.h
  *
  *   Defines database plugin for RocksDB backend.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBPLUGIN_H
#define HATNROCKSDBPLUGIN_H

#include <hatn/db/dbplugin.h>

#include <hatn/db/plugins/rocksdb/rocksdbdriver.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class HATN_ROCKSDB_EXPORT RocksdbPlugin : public DbPlugin
{
    public:

        constexpr static const char* Name="hatnrocksdb";
        constexpr static const char* Description="hatn database driver for RocksDB backend";
        constexpr static const char* Vendor="github.com/evgeniums/hatn";
        constexpr static const char* Revision="1.0.0";

        using DbPlugin::DbPlugin;

        std::shared_ptr<Client> makeClient() const override;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBPLUGIN_H
