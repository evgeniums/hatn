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

#ifdef HATN_ENABLE_PLUGIN_OPENSSL
#include <hatn/crypt/plugins/openssl/opensslplugin.h>
#endif

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/rocksdbplugin.h>
#endif

#endif

HATN_TEST_NAMESPACE_BEGIN

#ifdef NO_DYNAMIC_HATN_PLUGINS

#ifdef HATN_ENABLE_PLUGIN_OPENSSL
HATN_PLUGIN_INIT_FN(HATN_OPENSSL_NAMESPACE::OpenSslPlugin,initOpensslPlugin)
#else
void initOpensslPlugin()
{}
#endif

#ifdef HATN_ENABLE_PLUGIN_OPENSSL
HATN_PLUGIN_INIT_FN(HATN_ROCKSDB_NAMESPACE::RocksdbPlugin,initRocksdbPlugin)
#else
void initRocksdbPlugin()
{}
#endif

#else

void initOpensslPlugin()
{}

void initRocksdbPlugin()
{}

#endif

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
        initOpensslPlugin();
        initRocksdbPlugin();

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
    m_logCtx=HATN_LOGCONTEXT_NAMESPACE::makeLogCtx();
    m_logCtx->beforeThreadProcessing();
    HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context)->setStackLockingEnabled(false);
}

void DbTestFixture::teardown()
{
    if (m_logCtx)
    {
        m_logCtx->afterThreadProcessing();
        m_logCtx.reset();
    }
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
