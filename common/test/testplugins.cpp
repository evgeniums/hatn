#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <hatn/common/logger.h>
#include <hatn/thirdparty/sha1/sha1.h>
#include <hatn/common/plugin.h>

#include <hatn_test_config.h>

HATN_USING
HATN_COMMON_USING

BOOST_AUTO_TEST_SUITE(TestPlugins)

#ifndef NO_DYNAMIC_HATN_PLUGINS

BOOST_AUTO_TEST_CASE(Sha1)
{
    std::string sha1;
    SHA1::fileHash("common/assets/testplugin.dat",sha1);
    BOOST_CHECK_EQUAL(sha1,std::string("e8293e8808e4eaf2114d7e8a79dd0088942bbc96"));
}

BOOST_AUTO_TEST_CASE(DynamicPlugin)
{
    std::string pluginFolder="plugins/common/";

    std::string filename=PluginLoader::dynlibName("hatntestplugin");
    auto r=PluginLoader::instance().loadDynamicPlugin(pluginFolder+filename);
    if (r)
    {
        BOOST_TEST_MESSAGE(fmt::format("error: {}",r.error().message()));
    }
    BOOST_REQUIRE(!r);
    auto plugin=r.takeValue();
    BOOST_REQUIRE(plugin);
    BOOST_CHECK_EQUAL(plugin->info()->name,std::string("hatntestplugin"));
    BOOST_CHECK_EQUAL(plugin->info()->description,std::string("Test Plugin"));
    BOOST_CHECK_EQUAL(plugin->info()->type,std::string("TestPlugin"));
    BOOST_CHECK_EQUAL(plugin->info()->revision,std::string("1.0.0"));
    BOOST_CHECK_EQUAL(plugin->info()->vendor,std::string("hatn"));
    std::string sha1;
    SHA1::fileHash(pluginFolder+filename,sha1);
    BOOST_CHECK_EQUAL(plugin->info()->sha1,sha1);

    auto info=const_cast<PluginInfo*>(PluginLoader::instance().findPluginInfo("TestPlugin","hatntestplugin"));
    BOOST_CHECK(info!=nullptr);

    auto plugins=PluginLoader::instance().listDynamicPlugins(pluginFolder,false);
    BOOST_CHECK_EQUAL(plugins.size(),static_cast<uint32_t>(1));
    auto pluginInfo=plugins.front();
    BOOST_CHECK_EQUAL(pluginInfo->name,std::string("hatntestplugin"));
    BOOST_CHECK_EQUAL(pluginInfo->description,std::string("Test Plugin"));
    BOOST_CHECK_EQUAL(pluginInfo->type,std::string("TestPlugin"));
    BOOST_CHECK_EQUAL(pluginInfo->revision,std::string("1.0.0"));
    BOOST_CHECK_EQUAL(pluginInfo->vendor,std::string("hatn"));
    BOOST_CHECK_EQUAL(pluginInfo->sha1,sha1);

    info=const_cast<PluginInfo*>(PluginLoader::instance().findPluginInfo("TestPlugin","hatntestplugin"));
    BOOST_CHECK(info!=nullptr);

    info=const_cast<PluginInfo*>(PluginLoader::instance().findPluginInfo("TestPlugin","hatntestplugin"));
    BOOST_REQUIRE(info!=nullptr);
    PluginLoader::instance().free(info);
    info=const_cast<PluginInfo*>(PluginLoader::instance().findPluginInfo("TestPlugin","hatntestplugin"));
    BOOST_REQUIRE(info!=nullptr);
    plugin.reset();
    PluginLoader::instance().free(info);
    info=const_cast<PluginInfo*>(PluginLoader::instance().findPluginInfo("TestPlugin","hatntestplugin"));
    BOOST_CHECK(info==nullptr);

    plugins=PluginLoader::instance().listDynamicPlugins(pluginFolder);
    BOOST_CHECK_EQUAL(plugins.size(),static_cast<uint32_t>(1));
    info=const_cast<PluginInfo*>(PluginLoader::instance().findPluginInfo("TestPlugin","hatntestplugin"));
    BOOST_REQUIRE(info!=nullptr);
    plugins.clear();
    PluginLoader::instance().free(info);

    info=const_cast<PluginInfo*>(PluginLoader::instance().findPluginInfo("TestPlugin","hatntestplugin"));
    BOOST_CHECK(info==nullptr);

    plugins=PluginLoader::instance().listDynamicPlugins(pluginFolder,false);
    info=const_cast<PluginInfo*>(PluginLoader::instance().findPluginInfo("TestPlugin","hatntestplugin"));
    BOOST_CHECK(info==nullptr);

    plugins=PluginLoader::instance().listDynamicPlugins(pluginFolder);
    BOOST_REQUIRE_EQUAL(plugins.size(),static_cast<uint32_t>(1));
    pluginInfo=plugins.front();
    r=PluginLoader::instance().loadPlugin(pluginInfo.get());
    if (r)
    {
        BOOST_TEST_MESSAGE(fmt::format("error: {}",r.error().message()));
    }
    BOOST_REQUIRE(!r);
    plugin=r.takeValue();
    BOOST_CHECK(plugin);
    info=const_cast<PluginInfo*>(PluginLoader::instance().findPluginInfo("TestPlugin","hatntestplugin"));
    BOOST_REQUIRE(info!=nullptr);
    plugin.reset();

    PluginLoader::instance().free(info);
}
#endif

BOOST_AUTO_TEST_CASE(EmbeddedPlugin)
{
    auto r=PluginLoader::instance().loadPlugin<VSamplePlugin>();
    if (r)
    {
        BOOST_TEST_MESSAGE(fmt::format("error: {}",r.error().message()));
    }
    BOOST_REQUIRE(!r);
    auto plugin=r.takeValue();
    BOOST_REQUIRE(plugin);
    BOOST_CHECK_EQUAL(plugin->info()->name,std::string("hatnsampleplugin"));
    BOOST_CHECK_EQUAL(plugin->info()->description,std::string("Sample Plugin"));
    BOOST_CHECK_EQUAL(plugin->info()->type,std::string("VSamplePlugin"));
    BOOST_CHECK_EQUAL(plugin->info()->revision,std::string("1.0.0"));
    BOOST_CHECK_EQUAL(plugin->info()->vendor,std::string("hatn"));
    plugin.reset();

#if !defined(NO_DYNAMIC_HATN_PLUGINS) && !defined(HATN_TEST_MODULE_ALL)
    PluginLoader::instance().free();
    r=PluginLoader::instance().loadPlugin<VSamplePlugin>();
    BOOST_CHECK(r);
#endif
}

BOOST_AUTO_TEST_SUITE_END()
