/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/test/initcryptplugin.сpp
  *
  *  Init crypt plugin.
  *
  */

#include <iostream>
#include <boost/test/unit_test.hpp>

#include <hatn_test_config.h>
#include <hatn/test/pluginlist.h>

#include <hatn/crypt/cryptplugin.h>

#ifdef NO_DYNAMIC_HATN_PLUGINS

#ifdef HATN_ENABLE_PLUGIN_OPENSSL
#include <hatn/crypt/plugins/openssl/opensslplugin.h>
HATN_PLUGIN_INIT(hatn::crypt::openssl::OpenSslPlugin)
#endif

#endif

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

bool CryptPluginTest::m_initialized=false;

CryptPluginTest& CryptPluginTest::instance()
{
    static CryptPluginTest inst;
    return inst;
}

void CryptPluginTest::initOnce()
{
    if (!m_initialized)
    {
        BOOST_TEST_MESSAGE("Setup crypt plugins");
        if (HATN_TEST_PLUGINS.empty())
        {
            BOOST_FAIL("Crypt module is enabled for testing but list of crypt plugins is empty");
        }

        m_initialized=true;
        instance().init();
    }
}

void CryptTestFixture::setup()
{
    if (!::hatn::common::Logger::isRunning())
    {
        auto handler=[](const ::hatn::common::FmtAllocatedBufferChar &s)
        {
            std::cout<<::hatn::common::lib::toStringView(s)<<std::endl;
        };

        ::hatn::common::Logger::setDefaultVerbosity(LoggerVerbosity::INFO);
        ::hatn::common::Logger::setFatalTracing(false);
        ::hatn::common::Logger::setOutputHandler(handler);
        ::hatn::common::Logger::setFatalLogHandler(handler);
        ::hatn::common::Logger::start(false);
    }

    CryptPluginTest::instance().initOnce();
}

void CryptTestFixture::teardown()
{
    ::hatn::common::Logger::stop();
}

namespace {

struct CryptTestGlobal
{
    void setup()
    {
    }

    void teardown()
    {
        if (CryptPluginTest::instance().isInitialized())
        {
            BOOST_TEST_MESSAGE("Clean crypt plugins");
            CryptPluginTest::instance().cleanup();
        }
    }
};

}

BOOST_TEST_GLOBAL_FIXTURE(CryptTestGlobal);

BOOST_FIXTURE_TEST_SUITE(InitCryptPlugin,CryptTestFixture)

BOOST_AUTO_TEST_CASE(CryptPluginLoad)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>();
}

BOOST_AUTO_TEST_SUITE_END()

}
}
