/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/test/pluginlist.h
 *
 *     Get list of plugins for module
 *
 */
/****************************************************************************/

#ifndef HATNTESTPLUGINLIST_H
#define HATNTESTPLUGINLIST_H

#include <iostream>
#include <fstream>
#include <string>

#include <boost/test/unit_test.hpp>

#include <hatn/common/plugin.h>

#include "hatn_test_config.h"
#include <hatn/test/multithreadfixture.h>

namespace hatn {
namespace test {

class PluginList
{
    public:

        struct Context
        {
            std::string module;
            std::string type;
            std::vector<const common::PluginInfo*> plugins;
            size_t index=0;
        };

        static Context pluginsForModule(const std::string& module, const std::string& type)
        {
            Context context;
            context.module=module;
            context.type=type;
            auto plugins=common::PluginLoader::instance().listDynamicPlugins(pluginsPath(module));
            for (auto&& it:plugins)
            {
                if (HATN_TEST_PLUGINS.find(it->name)==HATN_TEST_PLUGINS.end()
                    &&
                    HATN_TEST_PLUGINS.find(std::string("hatn")+it->name)==HATN_TEST_PLUGINS.end()
                   )
                {
                    common::PluginLoader::instance().free(it.get());
                }
            }
            auto tmpPlugins=common::PluginLoader::instance().listPlugins(type);
            std::string dr("hatn");
            for (auto&& it:tmpPlugins)
            {
                if (HATN_TEST_PLUGINS.find(it->name)==HATN_TEST_PLUGINS.end())
                {
                    auto name=it->name;
                    name.erase(0,dr.size());
                    if (HATN_TEST_PLUGINS.find(name)==HATN_TEST_PLUGINS.end())
                    {
                        common::PluginLoader::instance().free(const_cast<common::PluginInfo*>(it));
                    }
                }
            }
            context.plugins=common::PluginLoader::instance().listPlugins(type);
            return context;
        }

        template <typename T>
        static std::shared_ptr<T> nextPlugin(Context& context, bool stopOnFail=false)
        {
            if (context.index<context.plugins.size())
            {
                auto pluginInfo=context.plugins[context.index++];
                auto plugin=common::PluginLoader::instance().loadPlugin<T>(pluginInfo);
                if (!plugin)
                {
                    std::string msg=std::string("Failed to load plugin ")+pluginInfo->name;
                    if (stopOnFail)
                    {
                        BOOST_FAIL(msg);
                    }
                    else
                    {
                        BOOST_TEST_MESSAGE(msg);
                        return nextPlugin<T>(context,stopOnFail);
                    }
                }
                return plugin;
            }
            return std::shared_ptr<T>();
        }

        static std::string pluginsPath(const std::string& module)
        {
            return fmt::format("./plugins/{}",module);
        }

        static std::string assetsPath(const std::string& module, const std::string& pluginName=std::string())
        {
            if (pluginName.empty())
            {
                return fmt::format("{}/{}/assets",MultiThreadFixture::assetsPath(),module);
            }
            return fmt::format("{}/plugins/{}/{}/assets",MultiThreadFixture::assetsPath(),module,pluginName);
        }

        static std::string linefromFile(const std::string &fname)
        {
            std::ifstream ss(fname);
            std::string s;
            getline(ss, s);
            return s;
        }

        static void eachLinefromFile(const std::string &fname,
                                     const std::function<void (std::string&)>& handler
                                     )
        {
            std::ifstream ss(fname);
            for (std::string line; std::getline(ss, line); )
            {
                handler(line);
            }
        }
};

class PluginTest
{
    public:

        PluginTest()=default;

        ~PluginTest()
        {
            reset();
        }

        PluginTest(const PluginTest&)=delete;
        PluginTest(PluginTest&&) =delete;
        PluginTest& operator=(const PluginTest&)=delete;
        PluginTest& operator=(PluginTest&&) =delete;

        template <typename Traits>
        void loadPlugins()
        {
            reset();
            m_ctx=PluginList::pluginsForModule(Traits::module,Traits::plugin_type::Type);
            BOOST_REQUIRE(!m_ctx.plugins.empty());
        }
        template <typename Traits>
        void eachPlugin(const std::function<void (std::shared_ptr<typename Traits::plugin_type>&)>& handler=
                    std::function<void (std::shared_ptr<typename Traits::plugin_type>&)>()
                )
        {
            m_ctx.index=0;
            do
            {
                auto plugin=PluginList::nextPlugin<typename Traits::plugin_type>(m_ctx);
                BOOST_REQUIRE(plugin);
                if (handler)
                {
                    handler(plugin);
                }
            } while (m_ctx.index<m_ctx.plugins.size());
            m_ctx.index=0;
        }
        void reset() noexcept
        {
            m_ctx.index=0;
            m_ctx.plugins.clear();
        }

    private:

        PluginList::Context m_ctx;
};

}
}
#endif // HATNTESTPLUGINLIST_H
