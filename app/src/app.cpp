/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/app.сpp
  *
  */

#include <hatn/common/filesystem.h>
#include <hatn/common/plugin.h>
#include <hatn/common/threadwithqueue.h>

#include <hatn/base/configobject.h>

#include <hatn/logcontext/streamlogger.h>
#include <hatn/logcontext/logconfigrecords.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/db/dbplugin.h>

#ifdef NO_DYNAMIC_HATN_PLUGINS

#ifdef HATN_ENABLE_PLUGIN_OPENSSL
#include <hatn/crypt/plugins/openssl/opensslplugin.h>
#endif

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/rocksdbplugin.h>
#endif

#endif

#include <hatn/app/apperror.h>
#include <hatn/app/app.h>

#include <hatn/common/loggermoduleimp.h>

INIT_LOG_MODULE(app,HATN_APP_EXPORT)


HATN_APP_NAMESPACE_BEGIN

namespace base=HATN_BASE_NAMESPACE;
namespace log=HATN_LOGCONTEXT_NAMESPACE;
namespace crypt=HATN_CRYPT_NAMESPACE;
namespace db=HATN_DB_NAMESPACE;

constexpr auto& logConfig=log::logConfigRecords;

namespace {

#ifdef NO_DYNAMIC_HATN_PLUGINS

#ifdef HATN_ENABLE_PLUGIN_OPENSSL
HATN_PLUGIN_INIT_FN(HATN_OPENSSL_NAMESPACE::OpenSslPlugin,initOpensslPlugin)
#else
void initOpensslPlugin()
{}
#endif

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
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

class LogAppConfig : public HATN_LOGCONTEXT_NAMESPACE::AppConfig
{
    public:

        using HATN_LOGCONTEXT_NAMESPACE::AppConfig::AppConfig;
};

} // anonymous namespace


#ifdef HATN_APP_THREADS_NUMBER
constexpr static const uint8_t DefaultThreadCountr=HATN_APP_THREADS_NUMBER;
#else
constexpr static const uint8_t DefaultThreadCount=1;
#endif

#if defined (BUILD_ANDROID) || defined (BUILD_IOS)
constexpr static const uint8_t ReserveThreadCount=2;
#else
constexpr static const uint8_t ReserveThreadCount=1;
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

constexpr static const char* ThreadTagAppThread="app_default";
constexpr static const char* ThreadTagNotMappedThread="not_mapped";

//---------------------------------------------------------------

HDU_UNIT(thread_config,
    HDU_FIELD(count_percent,TYPE_UINT8,1)
    HDU_FIELD(min_count,TYPE_UINT8,2,false,1)
    HDU_FIELD(id_prefix,TYPE_STRING,3,false,"tg")
    HDU_REPEATED_FIELD(tags,TYPE_STRING,4)
)

HDU_UNIT(app_config,
    HDU_FIELD(thread_count,TYPE_UINT8,1,false,DefaultThreadCount)
    HDU_FIELD(data_folder,TYPE_STRING,2)
    HDU_REPEATED_FIELD(plugin_folders,TYPE_STRING,3)
    HDU_REPEATED_FIELD(threads,thread_config::TYPE,4)
    HDU_FIELD(reserve_thread_count,TYPE_UINT8,5,false,ReserveThreadCount)
)

HDU_UNIT(logger_config,
    HDU_FIELD(name,TYPE_STRING,1,false,log::StreamLoggerName)
)

HDU_UNIT(db_config,
    HDU_FIELD(provider,TYPE_STRING,1,false,"hatnrocksdb")
    HDU_FIELD(thread_count,TYPE_UINT8,2,false,1)
)

HDU_UNIT(crypt_config,
    HDU_FIELD(provider,TYPE_STRING,1,false,"hatnopenssl")
)

//---------------------------------------------------------------

class App_p
{
    public:

        App* app;

        App_p(App* app) : app(app)
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

        void setAppLogger(std::shared_ptr<log::Logger> newLogger)
        {
            logger=std::move(newLogger);
            currentThreadLogCtx=log::makeLogCtx();
            auto& currentLogCtx=currentThreadLogCtx->get<log::Context>();
            currentLogCtx.setLogger(logger.get());
            log::ThreadLocalFallbackContext::set(&currentLogCtx);
        }
};

//---------------------------------------------------------------

App::App(AppName appName) :
        m_appName(std::move(appName)),
        m_configTree(std::make_shared<HATN_BASE_NAMESPACE::ConfigTree>()),
        m_configTreeLoader(std::make_shared<HATN_BASE_NAMESPACE::ConfigTreeLoader>()),
        d(std::make_unique<App_p>(this)),
        m_appConfigRoot(AppConfigRoot),
        m_defaultThreadCount(DefaultThreadCount)
{
    auto buildStreamLogger=[]() -> std::shared_ptr<log::LoggerHandler>
    {
        return std::make_shared<log::StreamLogger>();
    };
    registerLoggerHandlerBuilder(log::StreamLoggerName,buildStreamLogger);
    d->setAppLogger(log::makeLogger(buildStreamLogger()));
}

//---------------------------------------------------------------
App::~App()
{}

//---------------------------------------------------------------

Error App::loadConfigString(
        common::lib::string_view source,
        const std::string& format
    )
{
    auto ec=m_configTreeLoader->loadFromString(*m_configTree,source,HATN_BASE_NAMESPACE::ConfigTreePath{},format);
    if (ec)
    {
        //! @todo Log and stack error
        return ec;
    }
    return applyConfig();
}

//---------------------------------------------------------------

Error App::loadConfigFile(
    const std::string& fileName,
    const std::string& format
    )
{
    auto ec=m_configTreeLoader->loadFromFile(*m_configTree,fileName,HATN_BASE_NAMESPACE::ConfigTreePath{},format);
    if (ec)
    {
        //! @todo Log and stack error
        return ec;
    }
    return applyConfig();
}

//---------------------------------------------------------------

Error App::applyConfig()
{    
    // make and init logger

    // load app logger config
    base::config_object::LogRecords appLoggerRecords;
    auto ec=d->loggerConfig.loadLogConfig(*m_configTree,LoggerConfigRoot,appLoggerRecords);
    if (ec)
    {
        logConfig(_TR("logger configuration","app"),appLoggerRecords);
        //! @todo Log and stack error
        return ec;
    }

    // init log handler
    base::config_object::LogRecords logHandlerRecords;
    std::string loggerName{d->loggerConfig.config().fieldValue(logger_config::name)};
    auto logHandlerIt=d->loggerBuilders.find(loggerName);
    if (logHandlerIt==d->loggerBuilders.end())
    {
        return appError(AppError::UNKNOWN_LOGGER);
    }
    auto logHandler=logHandlerIt->second();
    std::string loggerConfigPath=fmt::format("{}.{}",LoggerConfigRoot,loggerName);
    ec=logHandler->loadLogConfig(*m_configTree,loggerConfigPath,logHandlerRecords);
    if (ec)
    {
        logConfig(_TR("","app"),appLoggerRecords);
        logConfig(_TR("logger configuration","app"),logHandlerRecords);
        //! @todo Log and stack error
        return ec;
    }

    // make and init logger
    base::config_object::LogRecords loggerRecords;
    auto logger=log::makeLogger(logHandler);
    ec=logger->loadLogConfig(*m_configTree,LoggerConfigRoot,loggerRecords);
    logConfig(_TR("logger configuration","app"),logHandlerRecords);
    if (ec)
    {
        logConfig(_TR("logger configuration","app"),appLoggerRecords);
        logConfig(_TR("logger configuration","app"),logHandlerRecords);
        logConfig(_TR("logger configuration","app"),loggerRecords);
        //! @todo Log and stack error
        return ec;
    }
    d->setAppLogger(std::move(logger));
    logConfig(_TR("logger configuration","app"),appLoggerRecords);
    logConfig(_TR("logger configuration","app"),logHandlerRecords);
    logConfig(_TR("logger configuration","app"),loggerRecords);

    base::config_object::LogRecords logRecords;

    //! @todo validate app config
    ec=d->appConfig.loadLogConfig(*m_configTree,m_appConfigRoot,logRecords);
    logConfig(_TR("application configuration","app"),logRecords);
    if (ec)
    {
        //! @todo Log and stack error
        return ec;
    }
    const auto& pluginsFolderField=d->appConfig.config().field(app_config::plugin_folders);
    for (size_t i=0;i<pluginsFolderField.count();i++)
    {
        d->pluginFolders.emplace_back(pluginsFolderField.at(i).stringView());
    }

    // load db config
    ec=d->dbConfig.loadLogConfig(*m_configTree,DbConfigRoot,logRecords);
    logConfig(_TR("application database configuration","app"),logRecords);
    if (ec)
    {
        //! @todo Log and stack error
        return ec;
    }

    // load crypt config
    ec=d->cryptConfig.loadLogConfig(*m_configTree,CryptConfigRoot,logRecords);
    logConfig(_TR("application cryptography configuration","app"),logRecords);
    if (ec)
    {
        //! @todo Log and stack error
        return ec;
    }

    // done
    return OK;
}

//---------------------------------------------------------------

Error App::initThreads()
{
    // create and start threads
    size_t count=m_defaultThreadCount;
    size_t reserveThreadCount=ReserveThreadCount;
    if (d->appConfig.config().field(app_config::reserve_thread_count).isSet())
    {
        reserveThreadCount=d->appConfig.config().field(app_config::reserve_thread_count).value();
    }
    if (d->appConfig.config().field(app_config::thread_count).isSet())
    {
        count=d->appConfig.config().field(app_config::thread_count).value();
        if (count==0)
        {
            count=std::thread::hardware_concurrency();
            if (count>reserveThreadCount)
            {
                // reserve one thread for system and one thread is the current thread
                count-=reserveThreadCount;
            }
        }
    }
    std::vector<uint8_t> threadGroupCounts;
    const auto& threadConfigs=d->appConfig.config().field(app_config::threads);
    for (size_t i=0;i<threadConfigs.count();i++)
    {
        const auto& threadConfig=threadConfigs.at(i);
        uint8_t groupCount=0;
        if (threadConfig.isSet(thread_config::count_percent))
        {
            groupCount=count * threadConfig.fieldValue(thread_config::count_percent) / 100;
        }
        if (threadConfig.isSet(thread_config::min_count))
        {
            auto minCount=threadConfig.fieldValue(thread_config::min_count);
            if (groupCount<minCount)
            {
                groupCount=minCount;
            }
        }
        threadGroupCounts.push_back(groupCount);
    }

    std::string threadName;
    auto createThreadLogFallback=[this,&threadName]()
    {
        auto taskCtx=log::makeLogCtx();
        auto& currentLogCtx=taskCtx->get<log::Context>();
        currentLogCtx.setLogger(d->logger.get());
        log::ThreadLocalFallbackContext::set(&currentLogCtx);
        d->threadLogCtxs[threadName]=taskCtx;
    };

    // create thread groups
    for (size_t i=0;i<threadConfigs.count();i++)
    {
        const auto& threadConfig=threadConfigs.at(i);
        uint8_t groupCount=threadGroupCounts[i];
        for (size_t i=0;i<groupCount;i++)
        {
            threadName=fmt::format("{}{}",threadConfig.fieldValue(thread_config::id_prefix),i);
            auto thread=std::make_shared<common::TaskWithContextThread>(threadName);
            const auto& threadTags=threadConfig.field(thread_config::tags);
            for (size_t j=0;j<threadTags.count();j++)
            {
                auto tag=std::string{threadTags.at(j)};
                if (tag==ThreadTagAppThread)
                {
                    m_appThread=thread.get();
                }
                thread->setTag(std::move(tag));
            }

            m_threads.push_back(thread);
            thread->start();

            // create fallback log context for thread
            std::ignore=thread->execSync(createThreadLogFallback);
        }
    }

    // create threads out of thread groups
    for (size_t i=0;i<count;i++)
    {
        threadName=fmt::format("t{}",i);
        auto thread=std::make_shared<common::TaskWithContextThread>(threadName);
        m_threads.push_back(thread);
        thread->start();

        // create fallback log context for thread
        std::ignore=thread->execSync(createThreadLogFallback);
    }
    if (m_appThread==nullptr)
    {
        m_appThread=m_threads.back().get();
    }

    // done
    return OK;
}

//---------------------------------------------------------------

Error App::init()
{
    // init data folder
    initAppDataFolder();

    // load plugins
    //! @todo Do not load unused plugins
    d->loadCryptPlugins();
    d->loadDbPlugins();

    // create and start threads
    auto ec=initThreads();

    // create mapped threads
    std::shared_ptr<common::MappedThreadQWithTaskContext> mappedThreads=std::make_shared<common::MappedThreadQWithTaskContext>(common::MappedThreadMode::Default,m_appThread);
    for (auto&& thread: m_threads)
    {
        if (!thread->hasTag(ThreadTagNotMappedThread))
        {
            mappedThreads->addMappedThread(thread.get());
        }
    }

    //! @todo configure/create allocator factory

    //! @todo create cipher suites

    //! @todo create translator

    // create env
    m_env=common::makeEnvType<AppEnv>(
        common::subcontexts(
            common::subcontext(),
            common::subcontext(std::move(mappedThreads)),
            common::subcontext(d->logger),
            common::subcontext(),
            common::subcontext(),
            common::subcontext()
        )
    );    

    // start logger
    const auto& factory=m_env->get<AllocatorFactory>();
    LogAppConfig appConfig{factory.factory()};
    d->logger->setAppConfig(appConfig);
    ec=d->logger->start();
    if (ec)
    {
        //! @todo Log and stack error
        return ec;
    }

    // done
    return OK;
}

//---------------------------------------------------------------

void App::close()
{
    if (m_env)
    {
        // close db client
        auto ec=database().close();
        //! @todo log error
        if (ec)
        {
            std::cerr << "Failed to close db: "<< ec.value() << ": " << ec.message() << std::endl;
        }
        database().reset();
    }

    // stop all threads
    for (auto&& it: m_threads)
    {
        it->stop();
    }

    // cleanup plugins
    if (d->dbPlugin)
    {
        auto ec=d->dbPlugin->cleanup();
        if (ec)
        {
            std::cerr << "Failed to cleanup db plugin " << d->dbPlugin->info()->name << ": " << ec.value() << ": " << ec.message() << std::endl;
        }
    }

    // close logger
    if (d->logger)
    {
        //! @todo Log app close
        auto ec=d->logger->close();
        if (ec)
        {
            std::cerr << "Failed to close logger: "<< ec.value() << ": " << ec.message() << std::endl;
        }
    }

    // destroy env
    m_env.reset();    
}

//---------------------------------------------------------------

void App::setDefaultAppDataFolder(
        std::string folder
    )
{
    m_appsDataFolder=std::move(folder);
}

//---------------------------------------------------------------

void App::setAppDataFolder(
    std::string folder
    )
{
    m_appDataFolder=std::move(folder);
    m_configTreeLoader->setPrefixSubstitution("$data_dir",m_appDataFolder);
}

//---------------------------------------------------------------

void App::initAppDataFolder()
{
    // set app data folder
    if (m_appDataFolder.empty())
    {
        m_appDataFolder=d->appConfig.config().fieldValue(app_config::data_folder);
    }
    //! @todo Reload config with new data folder?
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

Error App::createAppDataFolder()
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

void App::registerLoggerHandlerBuilder(std::string name, LoggerHandlerBuilder builder)
{
    d->loggerBuilders[std::move(name)]=std::move(builder);
}

//---------------------------------------------------------------

void App_p::loadPlugins(const std::string& pluginFolder)
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

//---------------------------------------------------------------

void App_p::loadCryptPlugins()
{
    initOpensslPlugin();
    loadPlugins(CryptPluginsFolder);
}

//---------------------------------------------------------------

void App_p::loadDbPlugins()
{
    initRocksdbPlugin();
    loadPlugins(DbPluginsFolder);
}

//---------------------------------------------------------------

void App::addPluginFolders(std::vector<std::string> folders)
{
    d->pluginFolders.insert(d->pluginFolders.end(),folders.begin(),folders.end());
}

//---------------------------------------------------------------

Result<std::shared_ptr<db::DbPlugin>> App_p::loadDbPlugin(lib::string_view name)
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
    //! @todo log error
    HATN_CHECK_EC(ec)

    // done
    return dbPlugin;
}

//---------------------------------------------------------------

db::ClientConfig App_p::dbClientConfig(lib::string_view name) const
{
    //! @todo Create/init encryption manager

    auto cfgPath=base::ConfigTreePath{DbConfigRoot}.copyAppend(name);
    db::ClientConfig cfg{app->m_configTree,
                         app->m_configTree,
                         cfgPath.copyAppend("main"),
                         cfgPath.copyAppend("options")
                         };
    cfg.dbPathPrefix=app->m_appDataFolder;
    return cfg;
}

//---------------------------------------------------------------

Error App::openDb(bool create)
{
    // load plugin
    auto name=d->dbConfig.config().fieldValue(db_config::provider);
    if (name.empty())
    {
        return appError(AppError::UNKNOWN_DB_PROVIDER);
    }
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

    auto thread=appThread();
    Assert(thread!=nullptr,"Threads must be initialized before opening database");

    // open db
    Error ec;
    base::config_object::LogRecords logRecords;
    if (thread->id()==common::Thread::currentThread()->id())
    {
        ec=d->dbClient->openDb(d->dbClientConfig(name),logRecords,create);
    }
    else
    {
        std::ignore=thread->execSync(
            [this,&ec,&logRecords,&name,create]()
            {
                ec=d->dbClient->openDb(d->dbClientConfig(name),logRecords,create);
            }
        );
    }
    logConfig(_TR("application database configuration","app"),logRecords);

    if (ec)
    {
        //! @todo log error
        return ec;
    }

    // set db client in env
    //! @todo Use thread tags
    auto mappedThreads=std::make_shared<common::MappedThreadQWithTaskContext>(common::MappedThreadMode::Default,thread);
    uint8_t dbThreadCount=d->dbConfig.config().fieldValue(db_config::thread_count);
    if (dbThreadCount>m_threads.size()-1)
    {
        dbThreadCount=static_cast<uint8_t>(m_threads.size()-1);
    }
    if (dbThreadCount>1)
    {
        mappedThreads->setThreadMode(common::MappedThreadMode::Mapped);
        size_t i=0;
        for (auto&& it:m_threads)
        {
            mappedThreads->addMappedThread(it.get());
            i++;
            if (i==dbThreadCount)
            {
                break;
            }
        }
    }
    auto asyncDbClient=std::make_shared<db::AsyncClient>(d->dbClient,std::move(mappedThreads));
    database().setDbClient(std::move(asyncDbClient));

    // done
    return OK;
}

//---------------------------------------------------------------

Error App::destroyDb()
{
    HATN_CTX_SCOPE("app:destroyDb")

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
    logConfig(_TR("application database configuration","app"),logRecords);
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
