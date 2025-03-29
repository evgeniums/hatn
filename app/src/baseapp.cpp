/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/baseapp.сpp
  *
  */

#include <hatn/common/filesystem.h>
#include <hatn/common/plugin.h>

#include <hatn/base/configobject.h>

#include <hatn/logcontext/streamlogger.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/crypt/cryptplugin.h>

#include <hatn/db/dbplugin.h>

#include <hatn/app/baseapp.h>

#include <hatn/common/loggermoduleimp.h>

INIT_LOG_MODULE(app,HATN_APP_EXPORT)


HATN_APP_NAMESPACE_BEGIN

namespace base=HATN_BASE_NAMESPACE;
namespace log=HATN_LOGCONTEXT_NAMESPACE;
namespace crypt=HATN_CRYPT_NAMESPACE;
namespace db=HATN_DB_NAMESPACE;

namespace {

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

} // anonymous namespace


#ifdef HATN_APP_THREADS_NUMBER
constexpr static const uint8_t DefaultThreadCountr=HATN_APP_THREADS_NUMBER;
#else
constexpr static const uint8_t DefaultThreadCount=1;
#endif

#ifdef HATN_APP_CONFIG_ROOT
constexpr static const char* AppConfigRoot=HATN_APP_CONFIG_ROOT;
#else
constexpr static const char* AppConfigRoot="app";
#endif

#ifdef HATN_APP_LOGGER_CONFIG_ROOT
constexpr static const char* LoggerConfigRoot=HATN_APP_LOGGER_CONFIG_ROOT;
#else
constexpr static const char* LoggerConfigRoot="logger";
#endif

#ifdef HATN_APP_DB_CONFIG_ROOT
constexpr static const char* DbConfigRoot=HATN_APP_DB_CONFIG_ROOT;
#else
constexpr static const char* DbConfigRoot="db";
#endif

#ifdef HATN_APP_CRYPT_CONFIG_ROOT
constexpr static const char* DbConfigRoot=HATN_APP_CRYPT_CONFIG_ROOT;
#else
constexpr static const char* CryptConfigRoot="crypt";
#endif

constexpr static const char* CryptPluginsFolder="crypt";
constexpr static const char* DbPluginsFolder="db";

//---------------------------------------------------------------

HDU_UNIT(app_config,
    HDU_FIELD(thread_count,TYPE_UINT8,1,DefaultThreadCount)
    HDU_FIELD(data_folder,TYPE_STRING,2)
    HDU_REPEATED_FIELD(plugin_folders,TYPE_STRING,3)
)

HDU_UNIT(logger_config,
    HDU_FIELD(name,TYPE_STRING,1,false,log::StreamLoggerName)
)

HDU_UNIT(db_config,
    HDU_FIELD(provider,TYPE_STRING,1,false,"hatnrocksdb")
)

HDU_UNIT(crypt_config,
    HDU_FIELD(provider,TYPE_STRING,1,false,"hatnopenssl")
)

//---------------------------------------------------------------

class BaseApp_p
{
    public:

        BaseApp* app;

        BaseApp_p(BaseApp* app) : app(app)
        {}

        base::ConfigObject<app_config::type> appConfig;
        base::ConfigObject<logger_config::type> loggerConfig;
        base::ConfigObject<db_config::type> dbConfig;
        base::ConfigObject<crypt_config::type> cryptConfig;

        std::map<std::string,LoggerHandlerBuilder,std::less<>> loggerBuilders;
        std::shared_ptr<log::Logger> logger;

        std::vector<std::string> pluginFolders;

        std::shared_ptr<db::DbPlugin> dbPlugin;
        std::shared_ptr<db::Client> dbClient;

        common::SharedPtr<log::TaskLogContext> currentThreadLogCtx;
        std::map<std::string,common::SharedPtr<log::TaskLogContext>> threadLogCtxs;

        void loadCryptPlugins();
        void loadDbPlugins();
        void loadPlugins(const std::string& pluginFolder);

        Result<std::shared_ptr<db::DbPlugin>> loadDbPlugin(lib::string_view name);
        db::ClientConfig dbClientConfig(lib::string_view name) const;
};

//---------------------------------------------------------------

BaseApp::BaseApp(AppName appName) :
        m_appName(std::move(appName)),
        m_configTree(std::make_shared<HATN_BASE_NAMESPACE::ConfigTree>()),
        m_configTreeLoader(std::make_shared<HATN_BASE_NAMESPACE::ConfigTreeLoader>()),
        d(std::make_unique<BaseApp_p>(this)),
        m_appConfigRoot(AppConfigRoot),
        m_defaultThreadCount(DefaultThreadCount)
{
    registerLoggerHandlerBuilder(log::StreamLoggerName,
                                 []() -> std::shared_ptr<log::LoggerHandler>
                                 {
                                     return std::make_shared<log::StreamLogger>();
                                 }
    );
}

//---------------------------------------------------------------

Error BaseApp::loadConfigString(
        common::lib::string_view source,
        const std::string& format
    )
{
    auto ec=m_configTreeLoader->loadFromString(*m_configTree,source,HATN_BASE_NAMESPACE::ConfigTreePath{},format);
    HATN_CHECK_EC(ec)
    return applyConfig();
}

//---------------------------------------------------------------

Error BaseApp::loadConfigFile(
    const std::string& fileName,
    const std::string& format
    )
{
    auto ec=m_configTreeLoader->loadFromFile(*m_configTree,fileName,HATN_BASE_NAMESPACE::ConfigTreePath{},format);
    HATN_CHECK_EC(ec)
    return applyConfig();
}

//---------------------------------------------------------------

Error BaseApp::applyConfig()
{
    base::config_object::LogRecords logRecords;

    // make and init logger
    //! @todo Log logger config
    //! @todo validate log config
    auto ec=d->loggerConfig.loadLogConfig(*m_configTree,LoggerConfigRoot,logRecords);
    HATN_CHECK_EC(ec)
    std::string loggerName{d->loggerConfig.config().fieldValue(logger_config::name)};
    auto logHandlerIt=d->loggerBuilders.find(loggerName);
    if (logHandlerIt==d->loggerBuilders.end())
    {
        //! @todo Use app error code
        return commonError(CommonError::UNSUPPORTED);
    }
    auto logHandler=logHandlerIt->second();
    std::string loggerConfigPath=fmt::format("{}.{}",LoggerConfigRoot,loggerName);
    ec=logHandler->loadConfig(*m_configTree,loggerConfigPath);
    HATN_CHECK_EC(ec)
    d->logger=log::makeLogger(logHandler);
    d->currentThreadLogCtx=log::makeLogCtx();
    auto& currentLogCtx=d->currentThreadLogCtx->get<log::Context>();
    currentLogCtx.setLogger(d->logger.get());
    log::ThreadLocalFallbackContext::set(&currentLogCtx);

    //! @todo Log app config
    //! @todo validate app config
    ec=d->appConfig.loadLogConfig(*m_configTree,m_appConfigRoot,logRecords);
    HATN_CHECK_EC(ec)
    const auto& pluginsFolderField=d->appConfig.config().field(app_config::plugin_folders);
    for (size_t i=0;i<pluginsFolderField.count();i++)
    {
        d->pluginFolders.emplace_back(pluginsFolderField.at(i).stringView());
    }

    // load db config
    //! @todo Log db config
    ec=d->dbConfig.loadLogConfig(*m_configTree,DbConfigRoot,logRecords);
    HATN_CHECK_EC(ec)

    // load crypt config
    //! @todo Log crypt config
    ec=d->cryptConfig.loadLogConfig(*m_configTree,CryptConfigRoot,logRecords);
    HATN_CHECK_EC(ec)

    // done
    return OK;
}

//---------------------------------------------------------------

Error BaseApp::init()
{
    // init data folder
    initAppDataFolder();

    // load plugins
    //! @todo Do not load unused plugins
    d->loadCryptPlugins();
    d->loadDbPlugins();

    // create and start threads
    size_t count=m_defaultThreadCount;
    if (d->appConfig.config().field(app_config::thread_count).isSet())
    {
        count=d->appConfig.config().field(app_config::thread_count).value();
        if (count==0)
        {
            count=std::thread::hardware_concurrency();
            if (count>2)
            {
                // reserve one thread for system and one thread is the current thread
                count-=2;
            }
        }
    }
    for (size_t i=0;i<count;i++)
    {
        auto threadName=fmt::format("t{}",i);
        auto thread=std::make_shared<common::TaskWithContextThread>(threadName);
        m_threads.emplace(threadName,thread);
        thread->start();

        // create fallback log context for thread
        std::ignore=thread->execSync(
            [this,&threadName]()
            {
                auto taskCtx=log::makeLogCtx();
                auto& currentLogCtx=taskCtx->get<log::Context>();
                currentLogCtx.setLogger(d->logger.get());
                log::ThreadLocalFallbackContext::set(&currentLogCtx);
                d->threadLogCtxs[threadName]=taskCtx;
            }
        );
    }        

    //! @todo configure/create allocator factory

    //! @todo create cipher suites

    //! @todo create translator

    // create env
    m_env=common::makeEnvType<AppEnv>(
        common::subcontexts(
            common::subcontext(),
            common::subcontext(d->logger),
            common::subcontext(),
            common::subcontext(),
            common::subcontext()
        )
    );    

    // done
    return OK;
}

//---------------------------------------------------------------

void BaseApp::close()
{
    //! @todo close db client async
    if (d->dbClient)
    {
        auto ec=d->dbClient->closeDb();
        //! @todo log error
        std::cerr << "Failed to close db: "<< ec.value() << ": " << ec.message() << std::endl;
        d->dbClient.reset();
    }

    //! @todo close logger

    // stop all threads
    for (auto&& it: m_threads)
    {
        it.second->stop();
    }

    // destroy env
    m_env.reset();

    // cleanup plugins
    if (d->dbPlugin)
    {
        auto ec=d->dbPlugin->cleanup();
        std::cerr << "Failed to cleanup db plugin " << d->dbPlugin->info()->name << ": " << ec.value() << ": " << ec.message() << std::endl;
    }
}

//---------------------------------------------------------------

void BaseApp::initAppDataFolder()
{
    // set app data folder
    if (m_appDataFolder.empty())
    {
        m_appDataFolder=d->appConfig.config().fieldValue(app_config::data_folder);
    }
    if (m_appDataFolder.empty())
    {
        if (m_appsDataFolder.empty())
        {
#ifdef _WIN32
            m_appsDataFolder=getenv("APPDATA");
#else
            m_appsDataFolder=getenv("HOME");
#endif
        }
        lib::filesystem::path p{m_appsDataFolder};
#ifdef _WIN32
        p.append(m_appName.execName);
#else
        p.append(fmt::format(".{}",m_appName.execName));
#endif
        m_appDataFolder=p.string();
    }
}

//---------------------------------------------------------------

Error BaseApp::createAppDataFolder()
{
    initAppDataFolder();
    lib::fs_error_code ec;
    lib::filesystem::create_directories(m_appDataFolder,ec);
    if (ec)
    {
        //! @todo Log error
        return Error{ec.value(),&ec.category()};
    }
    return OK;
}

//---------------------------------------------------------------

void BaseApp::registerLoggerHandlerBuilder(std::string name, LoggerHandlerBuilder builder)
{
    d->loggerBuilders[std::move(name)]=std::move(builder);
}

//------------------------------------------------- --------------

void BaseApp_p::loadPlugins(const std::string& pluginFolder)
{
#ifndef NO_DYNAMIC_HATN_PLUGINS
    for (auto&& folder: pluginFolders)
    {
        lib::filesystem::path p{folder};
        p.append(pluginFolder);
        common::PluginLoader::instance().listDynamicPlugins(p.string());
    }
#else
    std::ignore=pluginFolder;
#endif
}

//------------------------------------------------- --------------

void BaseApp_p::loadCryptPlugins()
{
    initOpensslPlugin();
    loadPlugins(CryptPluginsFolder);
}

//------------------------------------------------- --------------

void BaseApp_p::loadDbPlugins()
{
    initRocksdbPlugin();
    loadPlugins(DbPluginsFolder);
}

//------------------------------------------------- --------------

void BaseApp::addPluginFolders(std::vector<std::string> folders)
{
    d->pluginFolders.insert(d->pluginFolders.end(),folders.begin(),folders.end());
}

//------------------------------------------------- --------------

Result<std::shared_ptr<db::DbPlugin>> BaseApp_p::loadDbPlugin(lib::string_view name)
{
    if (dbPlugin)
    {
        return dbPlugin;
    }

    // load plugin
    dbPlugin=common::PluginLoader::instance().loadPlugin<db::DbPlugin>(std::string{name});
    if (!dbPlugin)
    {
        return db::dbError(db::DbError::DB_PLUGIN_FAILED);
    }

    // init plugin
    auto ec=dbPlugin->init();
    HATN_CHECK_EC(ec)

    // done
    return dbPlugin;
}

//------------------------------------------------- --------------

db::ClientConfig BaseApp_p::dbClientConfig(lib::string_view name) const
{
    //! @todo Create/init encryption manager

    auto cfgPath=base::ConfigTreePath{DbConfigRoot}.copyAppend(name);
    db::ClientConfig cfg{app->m_configTree,
                         app->m_configTree,
                         cfgPath.copyAppend("main"),
                         cfgPath.copyAppend("options")
                         };
    return cfg;
}

//------------------------------------------------- --------------

Error BaseApp::openDb(bool create)
{
    // load plugin
    auto name=d->dbConfig.config().fieldValue(db_config::provider);
    auto plugin=d->loadDbPlugin(name);
    if (plugin)
    {
        //! @todo log error
        return plugin.takeError();
    }
    d->dbPlugin=plugin.takeValue();

    // create db client
    d->dbClient=d->dbPlugin->makeClient();
    Assert(d->dbClient,"Failed to create db client in plugin");

    // open db
    base::config_object::LogRecords logRecords;
    auto ec=d->dbClient->openDb(d->dbClientConfig(name),logRecords,create);
    //! @todo log records
    if (ec)
    {
        //! @todo log error
        return ec;
    }

    // set db client in context
    database().setDbClient(std::make_shared<db::AsyncClient>(d->dbClient));

    // done
    return OK;
}

//------------------------------------------------- --------------

Error BaseApp::destroyDb()
{
    // load plugin
    auto name=d->dbConfig.config().fieldValue(db_config::provider);
    auto plugin=d->loadDbPlugin(name);
    if (plugin)
    {
        //! @todo log error
        return plugin.takeError();
    }
    d->dbPlugin=plugin.takeValue();

    // create db client
    auto dbClient=d->dbPlugin->makeClient();
    Assert(dbClient,"Failed to create db client in plugin");

    // destroy db
    base::config_object::LogRecords logRecords;
    auto ec=dbClient->destroyDb(d->dbClientConfig(name),logRecords);
    //! @todo log records
    if (ec)
    {
        //! @todo log error
        return ec;
    }

    // done
    return OK;
}

//---------------------------------------------------------------

HATN_APP_NAMESPACE_END
