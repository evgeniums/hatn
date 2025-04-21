/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/appdefs.h
  *
  */

/****************************************************************************/

#ifndef HATNAPPBASEAPP_H
#define HATNAPPBASEAPP_H

#include <hatn/common/threadwithqueue.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configtreeloader.h>

#include <hatn/db/db.h>

#include <hatn/app/appdefs.h>
#include <hatn/app/appname.h>
#include <hatn/app/appenv.h>

HATN_DB_NAMESPACE_BEGIN
class Schema;
HATN_DB_NAMESPACE_END

DECLARE_LOG_MODULE_EXPORT(app,HATN_APP_EXPORT)

HATN_APP_NAMESPACE_BEGIN

class App_p;

class HATN_APP_EXPORT App
{
    public:

        App(AppName appName);
        ~App();

        App(const App&)=delete;
        App(App&&)=default;
        App& operator= (const App&)=delete;
        App& operator= (App&&)=default;

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
        );

        const std::string& appDataFolder() const noexcept
        {
            return m_appDataFolder;
        }

        void setDefaultAppDataFolder(
            std::string folder
        );

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
            return m_appThread;
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

        static void registerDbSchema(std::shared_ptr<HATN_DB_NAMESPACE::Schema> schema);
        static void unregisterDbSchema(const std::string& name);
        static void freeDbSchemas();

    private:

        Error applyConfig();        
        Error initThreads();
        void logAppStart();
        void logAppStop();

        std::string evalAppDataFolder(const HATN_BASE_NAMESPACE::ConfigTree& configTree) const;

        AppName m_appName;

        std::shared_ptr<HATN_BASE_NAMESPACE::ConfigTree> m_configTree;
        std::shared_ptr<HATN_BASE_NAMESPACE::ConfigTreeLoader> m_configTreeLoader;

        std::vector<std::shared_ptr<common::ThreadQWithTaskContext>> m_threads;
        common::ThreadQWithTaskContext* m_appThread;

        common::SharedPtr<AppEnv> m_env;

        std::unique_ptr<App_p> d;

        std::string m_appDataFolder;
        std::string m_appsDataFolder;

        std::string m_appConfigRoot;
        uint8_t m_defaultThreadCount;

        friend class App_p;
};

HATN_APP_NAMESPACE_END

#endif // HATNAPPBASEAPP_H
