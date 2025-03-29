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
#include <hatn/common/env.h>
#include <hatn/common/translate.h>
#include <hatn/common/logger.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configtreeloader.h>

#include <hatn/logcontext/withlogger.h>

#include <hatn/crypt/ciphersuite.h>

#include <hatn/db/asyncclient.h>

#include <hatn/app/app.h>

DECLARE_LOG_MODULE_EXPORT(app,HATN_APP_EXPORT)

HATN_APP_NAMESPACE_BEGIN

using Logger=HATN_LOGCONTEXT_NAMESPACE::WithLogger;
using LoggerHandlerBuilder=HATN_LOGCONTEXT_NAMESPACE::LoggerHandlerBuilder;
using Db=HATN_DB_NAMESPACE::AsyncDb;
using AllocatorFactory=common::pmr::WithFactory;
using CipherSuites=HATN_CRYPT_NAMESPACE::WithCipherSuites;
using Translator=common::WithTranslator;

using AppEnv=common::Env<AllocatorFactory,Logger,Db,CipherSuites,Translator>;

class BaseApp_p;

struct AppName
{
    std::string execName;
    std::string displayName;
};

class HATN_APP_EXPORT BaseApp
{
    public:

        BaseApp(AppName appName);

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

        Db& database() noexcept
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

        const auto& threads() const noexcept
        {
            return m_threads;
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

    private:

        Error applyConfig();
        void initAppDataFolder();

        AppName m_appName;

        std::shared_ptr<HATN_BASE_NAMESPACE::ConfigTree> m_configTree;
        std::shared_ptr<HATN_BASE_NAMESPACE::ConfigTreeLoader> m_configTreeLoader;

        std::map<std::string,std::shared_ptr<common::ThreadQWithTaskContext>> m_threads;

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
