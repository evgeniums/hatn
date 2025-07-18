/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/plugin.h
  *
  *      Plugins interface.
  *
  */

/****************************************************************************/

#ifndef HATNPLUGIN_H
#define HATNPLUGIN_H

#include <string>
#include <map>
#include <functional>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <hatn/common/format.h>

#include <hatn/common/common.h>
#include <hatn/common/translate.h>
#include <hatn/common/logger.h>
#include <hatn/common/singleton.h>
#include <hatn/common/result.h>
#include <hatn/common/nativeerror.h>

#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#else
#define DECL_EXPORT
#endif

#define HATN_PLUGIN_INIT(plugin) \
    static HATN_COMMON_NAMESPACE::PluginInfo PluginInfoInst{ \
                HATN_COMMON_NAMESPACE::GetPluginMeta<plugin>(), \
                [](const HATN_COMMON_NAMESPACE::PluginInfo* info) \
                { \
                     return new plugin(info); \
                } \
            };

#define HATN_PLUGIN_INIT_FN(plugin,name) \
    auto& name() \
    {\
        HATN_PLUGIN_INIT(plugin)\
        return PluginInfoInst;\
    }

#define HATN_PLUGIN_EXPORT(plugin) \
    HATN_PLUGIN_INIT(plugin) \
    extern "C" { \
        DECL_EXPORT HATN_COMMON_NAMESPACE::Plugin* PluginLoader() {return PluginInfoInst.buildPlugin();} \
    }

DECLARE_LOG_MODULE_EXPORT(plugin,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

constexpr const char* HatnVendor="hatn";

struct PluginInfo;

struct PluginMeta
{
    const std::string type;
    const std::string name;
    const std::string description;
    const std::string vendor;
    const std::string revision;
};
template <typename T> PluginMeta GetPluginMeta() noexcept
{
    return PluginMeta({T::Type,T::Name,T::Description,T::Vendor,T::Revision});
}

//! Plugin interface
class HATN_COMMON_EXPORT Plugin
{
    public:

        //! Ctor
        explicit Plugin(const PluginInfo* pluginInfo) noexcept;

        virtual ~Plugin();
        Plugin(const Plugin&)=delete;
        Plugin(Plugin&&) =delete;
        Plugin& operator=(const Plugin&)=delete;
        Plugin& operator=(Plugin&&) =delete;

        //! Get plugin info
        const PluginInfo* info() const noexcept
        {
            return m_pluginInfo;
        }

        typedef hatn::common::Plugin* (*pluginLoader)();

    private:

        const PluginInfo* m_pluginInfo;
        friend class PluginLoader;
};

//! Plugin info
struct HATN_COMMON_EXPORT PluginInfo : public PluginMeta
{
    PluginInfo(
        PluginMeta pluginMeta,
        std::function<hatn::common::Plugin* (const PluginInfo*)> loader
    );

    std::string sha1;
    std::string fileName;

    bool isEmbedded() const noexcept
    {
        return fileName.empty();
    }

    hatn::common::Plugin* buildPlugin() const
    {
        if (loader)
        {
            return loader(this);
        }
        return nullptr;
    }

    void resetLoader() noexcept
    {
        loader=std::function<hatn::common::Plugin* (const PluginInfo*)>();
    }

    private:

        std::function<hatn::common::Plugin* (const PluginInfo*)> loader;
        size_t refCount=0;
        friend class PluginLoader;
        friend class Plugin;
};

//! Sample plugin
class VSamplePlugin : public Plugin
{
    public:

        constexpr static const char* Name="hatnsampleplugin";
        constexpr static const char* Type="VSamplePlugin";
        constexpr static const char* Description="Sample Plugin";
        constexpr static const char* Vendor=HatnVendor;
        constexpr static const char* Revision="1.0.0";

        using Plugin::Plugin;
};

//! Plugin loader
class HATN_COMMON_EXPORT PluginLoader final : public Singleton
{
    public:

        HATN_SINGLETON_DECLARE()

        PluginLoader(const PluginLoader&)=delete;
        PluginLoader(PluginLoader&&) =delete;
        PluginLoader& operator=(const PluginLoader&)=delete;
        PluginLoader& operator=(PluginLoader&&) =delete;

        //! Load dynamic plugin of specific type
        template <typename T>
        Result<std::shared_ptr<T>> loadDynamicPlugin(
            const std::string& fileName //!< Plugin path
        )
        {
            std::shared_ptr<T> result;
            auto r=loadDynamicPlugin(fileName);
            HATN_CHECK_RESULT(r)
            auto plugin=r.takeValue();
            if (plugin)
            {
                result=std::dynamic_pointer_cast<T>(plugin);
                if (!result)
                {
                    return genericError(fmt::format(_TR("Could not cast plugin {0} to requested type!"),fileName),CommonError::PLUGIN_FAILED);
                }
            }
            return result;
        }

        /**
         * @brief Load dynamic plugin by file name
         * @param fileName Filename, intentionally not reference
         * @param error Error description
         * @return Loaded plugin
         */
        Result<std::shared_ptr<hatn::common::Plugin>> loadDynamicPlugin(
            std::string fileName
        );

        //! Close plugin
        void closeDynamicPlugin(
            const std::string& fileName //!< Plugin path
        );

        /**
         * @brief Close plugin by plugin's meta information
         * @param info Plugin meta information
         */
        void closeDynamicPlugin(
            const PluginInfo* info
        );

        /**
         * @brief Remove meta information about plugin and close plugin library if it is dynamic
         * @param info Plugin meta information
         *
         * @note infor will be invalidated after calling this function
         *
         */
        void free(PluginInfo* info)
        {
            doFree(info,true);
        }

        /**
         * @brief Close all plugins and clear meta informations
         */
        void free();

        /**
         * @brief List dynamic plugins in directory
         * @param path Directory path
         * @param keepLoaded Plugins will stay loaded in memory
         * @return List of plugin meta info
         *
         * Note that this meta's can't be used to build plugins directly, they only give some reference information.
         * Use loadPlugin() explicitly to build a plugin.
         */
        std::vector<std::shared_ptr<hatn::common::PluginInfo>>
                listDynamicPlugins(const std::string& path, bool keepLoaded=true);

        /**
         * @brief List all plugins of specific type
         * @param type Type
         * @return List of plugins
         */
        std::vector<const PluginInfo*>
                listPlugins(const std::string& type) const;

        template <typename T> std::vector<const PluginInfo*>
                listPlugins() const
        {
            return listPlugins(T::Type);
        }

        /**
         * @brief Load plugin by type and name and cast to type
         * @param name Name of plugin, if empty then first plugin of requested type will be loaded
         * @return Loaded plugin
         */
        template <typename T>
        Result<std::shared_ptr<T>> loadPlugin(
                const std::string& name=std::string()
            ) const
        {
            auto r=loadPlugin(T::Type,name);
            HATN_CHECK_RESULT(r)

            auto plugin=std::dynamic_pointer_cast<T>(r.takeValue());
            if (!plugin)
            {
                return commonError(CommonError::PLUGIN_INCOMPLATIBLE);
            }
            return plugin;
        }

        /**
         * @brief Load plugin by type and name
         * @brief Type of plugin
         * @param name Name of plugin, if empty then the first plugin of requested type will be loaded
         * @return Loaded plugin
         */
        Result<std::shared_ptr<Plugin>> loadPlugin(
            const std::string& type,
            const std::string& name=std::string()
        ) const;

        /**
         * @brief Load plugin by plugin info and cast to type
         * @param pluginInfo Plugin meta info
         * @return Loaded plugin
         */
        template <typename T>
        Result<std::shared_ptr<T>> loadPlugin(
                const PluginInfo* pluginInfo
            )
        {
            auto r=loadPlugin(pluginInfo);
            HATN_CHECK_RESULT(r)
            auto plugin=std::dynamic_pointer_cast<T>(r.takeValue());
            if (!plugin)
            {
                return commonError(CommonError::PLUGIN_INCOMPLATIBLE);
            }
            return plugin;
        }

        /**
         * @brief Load plugin by plugin meta info
         * @param pluginInfo Plugin meta info
         * @return Loaded plugin
         */
        Result<std::shared_ptr<Plugin>> loadPlugin(
            const PluginInfo* pluginInfo
        )
        {
            auto info=findPluginInfo(pluginInfo->type,pluginInfo->name);
            if (info)
            {
                auto plugin=std::shared_ptr<Plugin>(info->buildPlugin());
                if (plugin)
                {
                    return plugin;
                }
                if (!info->isEmbedded())
                {
                    auto r=loadDynamicPlugin(info->fileName);
                    HATN_CHECK_RESULT(r)
                    return r.takeValue();
                }
                return commonError(CommonError::PLUGIN_EMBEDDED);
            }
            return commonError(CommonError::PLUGIN_NOT_FOUND);
        }

        /**
         * @brief Registor meta info of plugin
         * @param pluginInfo Meta info of plugin
         */
        void registerPluginInfo(const PluginInfo* pluginInfo);

        /**
         * @brief Find registered meta information of plugin
         * @param type Type of plugin
         * @param name Name of plugin, if empty then first plugin of requested type will be looked for
         * @return Meta information of plugin
         */
        const PluginInfo* findPluginInfo(
                const std::string& type,
                const std::string& name=std::string()
        ) const noexcept;

        /**
         * @brief Find registered meta information of plugin
         * @param type Type of plugin
         * @param name Name of plugin, if empty then first plugin of requested type will be looked for
         * @return Meta information of plugin
         */
        template <typename T>
        const PluginInfo* findPluginInfo(
                const std::string& name=std::string()
        ) const noexcept
        {
            return findPluginInfo(T::Type,name);
        }

        //! Get singleton instance
        static PluginLoader& instance();

        //! Construct full name of dynamic library
        static std::string dynlibName(const std::string& libName);

    private:

        PluginLoader()=default;
        ~PluginLoader()
        {
            free();
        }

        void doFree(PluginInfo* info, bool erase);

        std::shared_ptr<std::map<std::string,const PluginInfo*>>
                typedPlugins(const std::string& type) const noexcept
        {
            auto it=m_plugins.find(type);
            if (it!=m_plugins.end())
            {
                return it->second;
            }
            return std::shared_ptr<std::map<std::string,const PluginInfo*>>();
        }

        const PluginInfo* pluginInfoByName(
                const std::string& name,
                const std::shared_ptr<std::map<std::string,const PluginInfo*>>& typedPlugins
            ) const noexcept
        {
            auto it=typedPlugins->find(name);
            if (it!=typedPlugins->end())
            {
                return it->second;
            }
            return nullptr;
        }

        void closeLibrary(const std::string& filename);

        std::map<std::string,std::shared_ptr<std::map<std::string,const PluginInfo*>>> m_plugins;
        std::map<std::string,const PluginInfo*> m_infoByFilename;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNPLUGIN_H
