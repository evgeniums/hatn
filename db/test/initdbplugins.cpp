/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/initdbplugins.сpp
  *
  *  Init db plugin.
  *
*/

#include <boost/test/unit_test.hpp>

#ifdef NO_DYNAMIC_HATN_PLUGINS

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/rocksdbplugin.h>
HATN_PLUGIN_INIT(HATN_ROCKSDB_NAMESPACE::RocksdbPlugin)
#endif

#endif

#include "initdbplugins.h"

HATN_TEST_NAMESPACE_BEGIN

bool DbPluginTest::m_initialized=false;

DbPluginTest& DbPluginTest::instance()
{
    static DbPluginTest inst;
    return inst;
}

void DbPluginTest::initOnce()
{
    if (!m_initialized)
    {
        BOOST_TEST_MESSAGE("Setup database plugins");
        if (HATN_TEST_PLUGINS.empty())
        {
            BOOST_FAIL("db module is enabled for testing but list of database plugins is empty");
        }

        m_initialized=true;
        instance().init();
    }
}

void DbTestFixture::setup()
{
    DbPluginTest::instance().initOnce();
}

void DbTestFixture::teardown()
{
}

HATN_TEST_NAMESPACE_END

namespace {

struct DbTestGlobal
{
    void setup()
    {
    }

    void teardown()
    {
        if (HATN_TEST_NAMESPACE::DbPluginTest::instance().isInitialized())
        {
            BOOST_TEST_MESSAGE("Clean database plugins");
            HATN_TEST_NAMESPACE::DbPluginTest::instance().cleanup();
        }
    }
};

}

BOOST_TEST_GLOBAL_FIXTURE(DbTestGlobal);
