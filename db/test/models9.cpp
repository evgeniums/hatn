/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/models9.cpp
*/

/****************************************************************************/

// HATN_TEST_SUITE TestFindPlain

#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/syntax.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/db/object.h>
#include <hatn/db/model.h>

#include "hatn_test_config.h"

#include "models9.h"

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void registerModels9()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB

    rdb::RocksdbModels::instance().registerModel(m9());

#endif
}
