/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/baseapp.h
  *
  */

/****************************************************************************/

#ifndef HATNAPPBASEAPP_H
#define HATNAPPBASEAPP_H

#include <hatn/common/threadwithqueue.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configtreeloader.h>

#include <hatn/app/app.h>
#include <hatn/app/appname.h>
#include <hatn/app/appenv.h>

DECLARE_LOG_MODULE_EXPORT(app,HATN_APP_EXPORT)

HATN_APP_NAMESPACE_BEGIN

class BaseApp_p;

class HATN_APP_EXPORT BaseApp
{
    public:

        BaseApp(AppName appName);
        ~BaseApp();

        BaseApp(const BaseApp&)=delete;
        BaseApp(BaseApp&&)=default;
        BaseApp& operator= (const BaseApp&)=delete;
        BaseApp& operator= (BaseApp&&)=default;

        Error loadConfigString(
            common::lib::string_view source,
            const std::string& format=std::string()
        );

        Error loadConfigFile(
            const std::string& fileName,
            const std::string& format=std::string()
        );

        Error init();

        void close();

        Logger& logger() noexcept
        {
            return m_env->get<Logger>();
        }

        const Logger& logger() const noexcept
        {
            return m_env->get<Logger>();
        }

        Db& database() noexcept
        {
            return m_env->get<Db>();
        }

        const Db& database() const noexcept
        {
            return m_env->get<Db>();
        }

        const AllocatorFactory& allocatorFactory() const noexcept
        {
            return m_env->get<AllocatorFactory>();
        }

        const CipherSuites& cipherSuites() const noexcept
        {
            return m_env->get<CipherSuites>();
        }

        const Translator& translator() const noexcept
        {
            return m_env->get<Translator>();
        }

        const HATN_BASE_NAMESPACE::ConfigTree& configTree() const noexcept
        {
            return *m_configTree;
        }

        std::shared_ptr<HATN_BASE_NAMESPACE::ConfigTree> configTreeShared() const
        {
            return m_configTree;
        }

        void setConfigTreeLoader(std::shared_ptr<HATN_BASE_NAMESPACE::ConfigTreeLoader> configTreeLoader)
        {
            m_configTreeLoader=std::move(configTreeLoader);
        }

        std::shared_ptr<HATN_BASE_NAMESPACE::ConfigTreeLoader> configTreeLoader() const
        {
            return m_configTreeLoader;
        }

        void setAppDataFolder(
            std::string appDataFolder
        )
        {
            m_appDataFolder=std::move(appDataFolder);
        }

        const std::string& appDataFolder() const noexcept
        {
            return m_appDataFolder;
        }

        void setDefaultAppDataFolder(
                std::string folder
            )
        {
            m_appsDataFolder=std::move(folder);
        }

        const std::string& appsDataFolder() const noexcept
        {
            return m_appsDataFolder;
        }

        Error createAppDataFolder();

        void registerLoggerHandlerBuilder(std::string name, LoggerHandlerBuilder builder);

        void setAppConfigRoot(std::string val)
        {
            m_appConfigRoot=std::move(val);
        }

        const std::string& appConfigRoot() const noexcept
        {
            return m_appConfigRoot;
        }

        void setDefaultThreadCount(uint8_t val)
        {
            m_defaultThreadCount=val;
        }

        uint8_t defaultThreadCount() const noexcept
        {
            return m_defaultThreadCount;
        }

        void addPluginFolders(std::vector<std::string> folders);

        Error openDb(bool create=true);

        Error destroyDb();

        common::SharedPtr<AppEnv> env() const
        {
            return m_env;
        }

        common::ThreadQWithTaskContext* appThread() const noexcept
        {
            if (m_threads.empty())
            {
                return nullptr;
            }
            return m_threads.back().get();
        }

        const AppName& appName() const
        {
            return m_appName;
        }

        std::shared_ptr<common::ThreadQWithTaskContext> threadShared(size_t idx) const
        {
            return m_threads[idx];
        }

        common::ThreadQWithTaskContext* thread(size_t idx, bool appThreadFallback=true) const
        {
            if (idx>=m_threads.size() && appThreadFallback)
            {
                return appThread();
            }

            return m_threads[idx].get();
        }

        size_t threadCount() const noexcept
        {
            return m_threads.size();
        }

        static std::string threadName(size_t idx)
        {
            return fmt::format("t{}",idx);
        }

        const Threads& envThreads() const noexcept
        {
            return m_env->get<Threads>();
        }

        Threads& envThreads() noexcept
        {
            return m_env->get<Threads>();
        }

    private:

        Error applyConfig();
        void initAppDataFolder();

        AppName m_appName;

        std::shared_ptr<HATN_BASE_NAMESPACE::ConfigTree> m_configTree;
        std::shared_ptr<HATN_BASE_NAMESPACE::ConfigTreeLoader> m_configTreeLoader;

        std::vector<std::shared_ptr<common::ThreadQWithTaskContext>> m_threads;

        common::SharedPtr<AppEnv> m_env;

        std::unique_ptr<BaseApp_p> d;

        std::string m_appDataFolder;
        std::string m_appsDataFolder;

        std::string m_appConfigRoot;
        uint8_t m_defaultThreadCount;

        friend class BaseApp_p;
};

HATN_APP_NAMESPACE_END

#endif // HATNAPPBASEAPP_H
