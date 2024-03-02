/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/plugin.сpp
  *
  *      Plugin interface.
  *
  */
#include <iostream>

#include <boost/filesystem.hpp>

#include <hatn/common/format.h>

#include <hatn/thirdparty/sha1/sha1.h>

#include <hatn/common/fileutils.h>
#include <hatn/common/logger.h>
#include <hatn/common/translate.h>
#include <hatn/common/plugin.h>
#include <hatn/common/loggermoduleimp.h>

#ifndef NO_DYNAMIC_HATN_PLUGINS
/********************** Library routines **************************/

#if defined(_WIN32) // Microsoft compiler
    #include <windows.h>
#elif defined(__GNUC__) // GNU compiler
    #include <dlfcn.h>
#else
#error define your compiler
#endif

/*
#define RTLD_LAZY   1
#define RTLD_NOW    2
#define RTLD_GLOBAL 4
*/

static void* LoadSharedLibrary(const std::string& fileName, int iMode = 2)
{
    #if defined(_WIN32) // Microsoft compiler
        (void)iMode;
        return (void*)LoadLibrary(fileName.c_str());
    #elif defined(__GNUC__) // GNU compiler
        return dlopen(fileName.c_str(),iMode);
    #endif
}
static void *GetFunction(void *Lib, const char *Fnname)
{
#if defined(_WIN32) // Microsoft compiler
    return (void*)GetProcAddress((HINSTANCE)Lib,Fnname);
#elif defined(__GNUC__) // GNU compiler
    return dlsym(Lib,Fnname);
#endif
}
static bool FreeSharedLibrary(void *hDLL)
{
#if defined(_WIN32) // Microsoft compiler
    return FreeLibrary((HINSTANCE)hDLL)!=0;
#elif defined(__GNUC__) // GNU compiler
    return dlclose(hDLL)==0;
#endif
}
static std::string GetLastFileError()
{
#if defined(_WIN32) // Microsoft compiler
  DWORD error = GetLastError();
  if (error)
  {
    LPVOID lpMsgBuf;
    DWORD bufLen = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
    if (bufLen)
    {
      LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
      std::string result(lpMsgStr, lpMsgStr+bufLen);

      LocalFree(lpMsgBuf);

      return result;
    }
  }
  return std::string();
#elif defined(__GNUC__) // GNU compiler
    std::string result(dlerror());
    return result;
#endif
}

/********************** End of library routines **************************/
#endif

INIT_LOG_MODULE(plugin,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

namespace
{
struct Lib_p
{
    int count;
    void *library;
    explicit Lib_p(void *lib):count(0),library(lib)
    {}
};
std::map<std::string,std::shared_ptr<Lib_p>> Libraries;
}

/********************** Plugin **************************/

//---------------------------------------------------------------
Plugin::Plugin(const PluginInfo *pluginInfo) noexcept : m_pluginInfo(pluginInfo)
{
    if (pluginInfo)
    {
        ++const_cast<PluginInfo*>(pluginInfo)->refCount;
    }
}

//---------------------------------------------------------------
Plugin::~Plugin()
{
    if (m_pluginInfo)
    {
        --const_cast<PluginInfo*>(m_pluginInfo)->refCount;
    }
}

/********************** PluginInfo **************************/

//---------------------------------------------------------------
PluginInfo::PluginInfo(
            PluginMeta pluginMeta,
            std::function<hatn::common::Plugin* (const PluginInfo*)> loader
    ) : PluginMeta(std::move(pluginMeta)),
        loader(std::move(loader))
{
    PluginLoader::instance().registerPluginInfo(this);
}

/********************** VSamplePlugin **************************/

static PluginInfo SamplePluginInfo(GetPluginMeta<VSamplePlugin>(),
                                   [](const PluginInfo* info)
                                   {
                                        return new VSamplePlugin(info);
                                   }
                                   );

/********************** PluginLoader **************************/

HATN_SINGLETON_INIT(PluginLoader)

//---------------------------------------------------------------
PluginLoader& PluginLoader::instance()
{
    static PluginLoader Instance;
    return Instance;
}

//---------------------------------------------------------------
std::vector<std::shared_ptr<hatn::common::PluginInfo>>
PluginLoader::listDynamicPlugins(
        const std::string &path,
        bool keepLoaded
    )
{
#ifndef NO_DYNAMIC_HATN_PLUGINS

    if (!boost::filesystem::exists(path))
    {
        return std::vector<std::shared_ptr<hatn::common::PluginInfo>>();
    }

    std::vector<std::shared_ptr<hatn::common::PluginInfo>> result;
    boost::filesystem::directory_iterator itEnd;
    for (boost::filesystem::directory_iterator it(path); it != itEnd; ++it)
    {
        if (boost::filesystem::is_regular_file(it->path()))
        {
            std::string error;
            std::string fileName = it->path().string();
#if defined(_WIN32)
            std::replace(fileName.begin(),fileName.end(),'\\','/');
#endif
            std::shared_ptr<hatn::common::Plugin> plugin=loadDynamicPlugin(fileName,error);
            if (plugin)
            {
                bool closed=false;
                auto pluginInfo=plugin->info();
                if (pluginInfo)
                {
                    auto info=findPluginInfo(pluginInfo->type,pluginInfo->name);
                    if (info)
                    {
                        const_cast<PluginInfo*>(info)->fileName=fileName;
                        std::string sha1;
                        if (SHA1::fileHash(fileName,sha1))
                        {
                            const_cast<PluginInfo*>(info)->sha1=sha1;
                        }
                        auto resultInfo=std::make_shared<PluginInfo>(*info);
                        resultInfo->resetLoader();

                        result.push_back(resultInfo);
                        if (!keepLoaded)
                        {
                            plugin.reset();
                            free(const_cast<PluginInfo*>(info));
                            closed=true;
                        }
                        else
                        {
                            m_infoByFilename[fileName]=info;
                        }
                    }
                }
                if (!closed)
                {
                    plugin.reset();
                    if (!keepLoaded)
                    {
                        closeLibrary(fileName);
                    }
                }
            }
        }
    }
    return result;
#else

    std::ignore=path;
    std::ignore=keepLoaded;

    return std::vector<std::shared_ptr<hatn::common::PluginInfo>>();
#endif
}

//---------------------------------------------------------------
std::shared_ptr<hatn::common::Plugin> PluginLoader::loadDynamicPlugin(
        std::string fileName,
        std::string &error
    )
{
#ifndef NO_DYNAMIC_HATN_PLUGINS

#if defined(_WIN32)
    std::replace(fileName.begin(),fileName.end(),'\\','/');
#endif

    auto it=m_infoByFilename.find(fileName);
    if (it!=m_infoByFilename.end())
    {
        return loadPlugin(it->second);
    }

    std::shared_ptr<hatn::common::Plugin> plugin;
    if (!boost::filesystem::exists(fileName))
    {
        error=fmt::format(_TR("Could not load plugin {}: file does not exists"),fileName);
        HATN_WARN(plugin,error);
        return plugin;
    }

    auto lib=LoadSharedLibrary(fileName);
    if (lib)
    {
        constexpr const char* funcName="PluginLoader";
        auto func=GetFunction(lib,funcName);
        if (func)
        {
            Plugin::pluginLoader loader=reinterpret_cast<Plugin::pluginLoader>(func);
            plugin.reset(loader());
            if (plugin)
            {
                auto libStored=std::make_shared<Lib_p>(lib);
                auto it=Libraries.find(fileName);
                if (it!=Libraries.end())
                {
                    libStored=it->second;
                }
                else
                {
                    libStored->library=lib;
                    Libraries[fileName]=libStored;
                }
                libStored->count++;

                auto pluginInfo=plugin->info();
                if (pluginInfo)
                {
                    bool incRefcount=true;
                    auto info=findPluginInfo(pluginInfo->type,pluginInfo->name);
                    if (!info)
                    {
                        // dynamic library can be unloaded and loaded again but static variables not always can re-register implicitly
                        // in that case explicitly register the plugin back
                        registerPluginInfo(pluginInfo);
                        info=findPluginInfo(pluginInfo->type,pluginInfo->name);
                        Assert(info,"Could not register plugin");

                        // this must be the first created plugin that corresponds to the PluginInfo
                        // so, set 1 for the refCount
                        incRefcount=false;
                        const_cast<PluginInfo*>(info)->refCount=1;
                    }
                    if (plugin->info()!=info)
                    {
                        plugin->m_pluginInfo=info;

                        const_cast<PluginInfo*>(info)->fileName=fileName;
                        std::string sha1;
                        if (SHA1::fileHash(fileName,sha1))
                        {
                            const_cast<PluginInfo*>(info)->sha1=sha1;
                        }
                        const_cast<PluginInfo*>(info)->loader=pluginInfo->loader;
                        if (incRefcount)
                        {
                            ++const_cast<PluginInfo*>(info)->refCount;
                        }

                        m_infoByFilename[fileName]=info;
                    }
                }
            }
            else
            {
                error=fmt::format(_TR("Invalid plugin loader in {}!"),fileName);
                FreeSharedLibrary(lib);
            }
        }
        else
        {
            error=fmt::format(_TR("Failed to find plugin signature in {}: {}!"),fileName,GetLastFileError());
            FreeSharedLibrary(lib);
        }
    }
    else
    {
        error=fmt::format(_TR("Failed to load dynamic library {}: {}!"),fileName,GetLastFileError());
    }
    if (!plugin)
    {
        HATN_WARN(plugin,_TR("Could not load plugin")<<": "<<error);
    }
    return plugin;
#else

    std::ignore=fileName;
    std::ignore=error;

    return std::shared_ptr<hatn::common::Plugin>();
#endif
}

//---------------------------------------------------------------
void PluginLoader::closeDynamicPlugin(
        const std::string &fileName
    )
{
#ifndef NO_DYNAMIC_HATN_PLUGINS
    auto it=m_infoByFilename.find(fileName);
    if (it!=m_infoByFilename.end())
    {
        closeDynamicPlugin(it->second);
    }
    else
    {
        closeLibrary(fileName);
    }
#else
    std::ignore=fileName;
#endif
}

//---------------------------------------------------------------
void PluginLoader::registerPluginInfo(const PluginInfo *pluginInfo)
{
    auto typePlugins=typedPlugins(pluginInfo->type);
    if (!typePlugins)
    {
        auto inserted=m_plugins.emplace(std::make_pair(pluginInfo->type,
                                                       std::make_shared<std::map<std::string,const PluginInfo*>>())
                                        );
        Assert(inserted.second,"Failed to register plugin");
        typePlugins=inserted.first->second;
    }
    auto it1=typePlugins->find(pluginInfo->name);
    if (it1!=typePlugins->end())
    {
        const auto& existing=it1->second;
        HATN_WARN(plugin,HATN_FORMAT("Duplicate plugin: {}/{} ()",pluginInfo->type,pluginInfo->name,pluginInfo->description));
        HATN_WARN(plugin,HATN_FORMAT("Previuos was: {}/{} ()",existing->type,existing->name,existing->description));
        return;
    }
    (*typePlugins)[pluginInfo->name]=new PluginInfo(*pluginInfo);
}

//---------------------------------------------------------------
const PluginInfo* PluginLoader::findPluginInfo(const std::string &type, const std::string &name) const noexcept
{
    auto typed=typedPlugins(type);
    if (typed)
    {
        if (!name.empty())
        {
            return pluginInfoByName(name,typed);
        }
        return typed->begin()->second;
    }
    return nullptr;
}

//---------------------------------------------------------------
void PluginLoader::closeLibrary(const std::string &fileName)
{
#ifndef NO_DYNAMIC_HATN_PLUGINS
    auto it=Libraries.find(fileName);
    if (it!=Libraries.end())
    {
        auto libStored=it->second;
        if (!FreeSharedLibrary(libStored->library))
        {
            HATN_WARN(plugin,HATN_FORMAT("Failed to colse library {}: {}",fileName,GetLastFileError()));
        }
        --libStored->count;
        if (libStored->count<=0)
        {
            Libraries.erase(it);
        }
    }
#else
    std::ignore=fileName;
#endif
}

//---------------------------------------------------------------
void PluginLoader::closeDynamicPlugin(const PluginInfo* info)
{
#ifndef NO_DYNAMIC_HATN_PLUGINS
    if (!info->isEmbedded())
    {
        Assert(info->refCount,"Can not close plugin whene there are plugin objects");
        auto infoD=const_cast<PluginInfo*>(info);
        infoD->resetLoader();
        closeLibrary(info->fileName);
    }
#else
    std::ignore=info;
#endif
}

//---------------------------------------------------------------
void PluginLoader::doFree(PluginInfo* info, bool erase)
{
    if (info->refCount!=0)
    {
        return;
    }

    auto typed=typedPlugins(info->type);
    if (typed)
    {
        auto name=info->name;
        auto type=info->type;
        if (erase)
        {
            typed->erase(name);
            if (typed->empty())
            {
                m_plugins.erase(type);
            }
        }
    }
#ifndef NO_DYNAMIC_HATN_PLUGINS
    if (!info->isEmbedded())
    {
        if (erase)
        {
            m_infoByFilename.erase(info->fileName);
        }
        std::string fileName=info->fileName;
        delete info;
        closeLibrary(fileName);
    }
    else
    {
        delete info;
    }
#else
    delete info;
#endif
}

//---------------------------------------------------------------
void PluginLoader::free()
{
    for (auto&& it:m_plugins)
    {
        for (auto&& it1:*it.second)
        {
            doFree(const_cast<PluginInfo*>(it1.second),false);
        }
    }
    m_plugins.clear();
    m_infoByFilename.clear();
}

//---------------------------------------------------------------
std::vector<const PluginInfo*>
PluginLoader::listPlugins(const std::string& type) const
{
    std::vector<const PluginInfo*> plugins;
    auto typed=typedPlugins(type);
    if (typed)
    {
        for (auto&& it:*typed)
        {
            plugins.push_back(it.second);
        }
    }
    return plugins;
}

//---------------------------------------------------------------
std::shared_ptr<Plugin> PluginLoader::loadPlugin(const std::string &type, const std::string &name) const
{
    auto info=findPluginInfo(type,name);
    if (info)
    {
        return std::shared_ptr<Plugin>(info->buildPlugin());
    }
    return std::shared_ptr<Plugin>();
}

//---------------------------------------------------------------
std::string PluginLoader::dynlibName(const std::string &libName)
{
#ifdef HATN_DEBUG

#ifdef _WIN32
    #if defined(__GNUC__)
        std::string filename=std::string("lib")+libName+std::string("d.dll");
    #else
        std::string filename=libName+"d.dll";
    #endif
#elif __APPLE__
    std::string filename=std::string("lib")+libName+std::string("d.dylib");
#else
    std::string filename=std::string("lib")+libName+std::string("d.so");
#endif

#else

#ifdef _WIN32
    #if defined(__GNUC__)
        std::string filename=std::string("lib")+libName+std::string(".dll");
    #else
        std::string filename=libName+".dll";
    #endif
#elif __APPLE__
    std::string filename=std::string("lib")+libName+std::string(".dylib");
#else
    std::string filename=std::string("lib")+libName+std::string(".so");
#endif

#endif

    return filename;
}

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
