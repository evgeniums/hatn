/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/initdbplugins.h
 *
 *     Init database plugins.
 *
 */
/****************************************************************************/

#ifndef HATNINITDBPLUGINS_H
#define HATNINITDBPLUGINS_H

#include <functional>

#include <hatn_test_config.h>

#include <hatn/logcontext/context.h>

#include <hatn/crypt/cryptplugin.h>

#include <hatn/db/dbplugin.h>
#include <hatn/test/pluginlist.h>

HATN_TEST_NAMESPACE_BEGIN

struct DbTestTraits
{
    constexpr static const char* module="db";
    using plugin_type=db::DbPlugin;
};

struct CryptPluginTraits
{
    constexpr static const char* module="crypt";
    using plugin_type=crypt::CryptPlugin;
};

class DbPluginTest : public PluginTest
{
    public:

        void init()
        {
            auto cryptPluginHandler=[](const common::PluginInfo* pluginInfo)
            {
                auto plugin=common::PluginLoader::instance().loadPlugin<CryptPluginTraits::plugin_type>(pluginInfo);
                if (plugin)
                {
                    auto ec=plugin->init();
                    if (ec)
                    {
                        std::string msg="Failed to init crypt plugin "+plugin->info()->name+": "+ec.message();
                        plugin.reset();
                        BOOST_FAIL(msg.c_str());
                    }
                    else
                    {
                        BOOST_TEST_MESSAGE(fmt::format("Loaded crypt plugin {}", plugin->info()->name));
                    }
                }
            };
            auto cryptPlugins=PluginList::pluginsForModule(CryptPluginTraits::module,CryptPluginTraits::plugin_type::Type);
            for (auto&& it:cryptPlugins.plugins)
            {
                cryptPluginHandler(it);
            }

            auto handler=[](std::shared_ptr<db::DbPlugin>& plugin)
            {
                auto ec=plugin->init();
                if (ec)
                {
                    std::string msg="Failed to init db plugin "+plugin->info()->name+": "+ec.message();
                    plugin.reset();
                    BOOST_FAIL(msg.c_str());
                }
                else
                {
                    BOOST_TEST_MESSAGE(fmt::format("Loaded db plugin {}", plugin->info()->name));
                }
            };
            loadPlugins<DbTestTraits>();
            eachPlugin<DbTestTraits>(handler);            
        }

        void cleanup()
        {
            auto handler=[](std::shared_ptr<db::DbPlugin>& plugin)
            {
                auto ec=plugin->cleanup();
                if (ec)
                {
                    std::string msg="Failed to cleanup db plugin "+plugin->info()->name+": "+ec.message();
                    BOOST_WARN_MESSAGE(false,msg.c_str());
                }
                else
                {
                    BOOST_TEST_MESSAGE(fmt::format("Done cleanup db plugin {}", plugin->info()->name));
                }
            };
            eachPlugin<DbTestTraits>(handler);

            auto cryptPluginHandler=[](const common::PluginInfo* pluginInfo)
            {
                auto plugin=common::PluginLoader::instance().loadPlugin<CryptPluginTraits::plugin_type>(pluginInfo);
                if (plugin)
                {
                    auto ec=plugin->cleanup();
                    if (ec)
                    {
                        std::string msg="Failed to cleanup crypt plugin "+plugin->info()->name+": "+ec.message();
                        BOOST_WARN_MESSAGE(false,msg.c_str());
                    }
                    else
                    {
                        BOOST_TEST_MESSAGE(fmt::format("Done cleanup crypt plugin {}", plugin->info()->name));
                    }
                }
            };
            auto cryptPlugins=PluginList::pluginsForModule(CryptPluginTraits::module,CryptPluginTraits::plugin_type::Type);
            for (auto&& it:cryptPlugins.plugins)
            {
                cryptPluginHandler(it);
            }

            reset();
        }

        static DbPluginTest& instance();

        static void initOnce();

        static void cleanUpOnce() noexcept
        {
            if (m_initialized)
            {
                m_initialized=false;
                instance().cleanup();
            }
        }

        static bool isInitialized() noexcept
        {
            return m_initialized;
        }

    private:

        static bool m_initialized;
};

class DbTestFixture : public MultiThreadFixture
{
    public:

        using MultiThreadFixture::MultiThreadFixture;

        void setup();
        void teardown();

        auto logCtx()
        {
            return m_logCtx;
        }

    private:

        HATN_COMMON_NAMESPACE::SharedPtr<HATN_LOGCONTEXT_NAMESPACE::LogCtxType> m_logCtx;
};

HATN_TEST_NAMESPACE_END

#endif // HATNINITDBPLUGINS_H
