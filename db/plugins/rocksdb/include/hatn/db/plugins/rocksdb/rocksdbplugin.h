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

HATN_DB_NAMESPACE_BEGIN

namespace rocksdbdriver {

// //! Database plugin for RocksDB backend.
// class DRACOSHA_DB_EXPORT RocksDbPlugin : public DbPlugin
// {
//     public:

//         constexpr static const char* Name="dracosharocksdbdriver";
//         constexpr static const char* Description="Dracosha database driver for RocksDB backend";
//         constexpr static const char* Vendor="dracosha.com";
//         constexpr static const char* Revision="1.0.0";

//         using DbPlugin::DbPlugin;
// };

} // namespace rocksdbdriver

HATN_DB_NAMESPACE_END

#endif // HATNROCKSDBPLUGIN_H
