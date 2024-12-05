/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/models4.cpp
*/

/****************************************************************************/

// HATN_TEST_SUITE TestFind

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#include "models1.h"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void registerModels4()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB

    rdb::RocksdbModels::instance().registerModel(m1_uint16());
    rdb::RocksdbModels::instance().registerModel(m1_uint32());

#endif
}
