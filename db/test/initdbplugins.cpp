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

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include "initdbplugins.h"

#ifdef NO_DYNAMIC_HATN_PLUGINS

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/rocksdbplugin.h>
HATN_PLUGIN_INIT(HATN_ROCKSDB_NAMESPACE::RocksdbPlugin)
#endif

#endif

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

    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(
        std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>())
    );
    m_logCtx=HATN_COMMON_NAMESPACE::makeTaskContext<HATN_LOGCONTEXT_NAMESPACE::ContextWrapper>();
    m_logCtx->beforeThreadProcessing();

}

void DbTestFixture::teardown()
{
    m_logCtx->afterThreadProcessing();
    m_logCtx.reset();
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
