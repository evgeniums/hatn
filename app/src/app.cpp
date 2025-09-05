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
#include <hatn/common/pointers/mempool/weakpool.h>

#include <hatn/base/configobject.h>

#include <hatn/logcontext/streamlogger.h>
#include <hatn/logcontext/filelogger.h>
#include <hatn/logcontext/logconfigrecords.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/datauniterror.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/network/asio/careslib.h>

#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/passwordgenerator.h>

#include <hatn/db/dbplugin.h>
#include <hatn/db/encryptionmanager.h>

#ifdef NO_DYNAMIC_HATN_PLUGINS

#ifdef HATN_ENABLE_PLUGIN_OPENSSL
#include <hatn/crypt/plugins/openssl/opensslplugin.h>
#endif

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/rocksdbplugin.h>
#endif

#endif

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
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

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
void freeRocksDb()
{
    HATN_ROCKSDB_NAMESPACE::RocksdbSchemas::free();
    HATN_ROCKSDB_NAMESPACE::RocksdbModels::free();
}
#else
void freeRocksDb()
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
constexpr static const char* CryptConfigRoot=HATN_APP_CRYPT_CONFIG_ROOT;
#else
constexpr static const char* CryptConfigRoot="crypt";
#endif

#ifdef HATN_APP_PASSWORD_GEN_CONFIG_ROOT
constexpr static const char* PasswordGenConfigRoot=HATN_APP_PASSWORD_GEN_CONFIG_ROOT;
#else
constexpr static const char* PasswordGenConfigRoot="password_generator";
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
    HDU_FIELD(provider,TYPE_STRING,1,false,log::StreamLoggerName)
)

HDU_UNIT(db_config,
    HDU_FIELD(provider,TYPE_STRING,1,false,"hatnrocksdb")
    HDU_FIELD(thread_count,TYPE_UINT8,2,false,1)
    HDU_FIELD(encrypted,TYPE_BOOL,3)
    HDU_FIELD(cipher_suite,TYPE_STRING,4)
    HDU_FIELD(encryption_chunk_size,TYPE_UINT32,5,false,crypt::MaxContainerChunkSize)
    HDU_FIELD(encryption_first_chunk_size,TYPE_UINT32,6,false,crypt::MaxContainerFirstChunkSize)
    HDU_FIELD(db_folder,TYPE_STRING,7,false,"db")
    HDU_FIELD(db_prefix,TYPE_STRING,8,false,"main")
    HDU_FIELD(main_db_path,TYPE_STRING,9)
)

HDU_UNIT(crypt_config,
    HDU_FIELD(provider,TYPE_STRING,1)
    HDU_REPEATED_FIELD(cipher_suites,crypt::cipher_suite::TYPE,2,false,Auto,Auto)
    HDU_FIELD(default_cipher_suite,TYPE_STRING,3)
)

HDU_UNIT(password_gen_config,
    HDU_FIELD(min_length,TYPE_UINT32,1,false,crypt::PasswordGeneratorParameters::DefaultMinLength)
    HDU_FIELD(max_length,TYPE_UINT32,2,false,crypt::PasswordGeneratorParameters::DefaultMaxLength)
    HDU_FIELD(letters_weight,TYPE_UINT32,3,false,crypt::PasswordGeneratorParameters::DefaultLettersWeight)
    HDU_FIELD(digits_weight,TYPE_UINT32,4,false,crypt::PasswordGeneratorParameters::DefaultDigitsWeight)
    HDU_FIELD(specials_weight,TYPE_UINT32,5,false,crypt::PasswordGeneratorParameters::DefaultSpecialsWeight)
    HDU_FIELD(has_special,TYPE_BOOL,6,false,crypt::PasswordGeneratorParameters::DefaultHasSpecial)
    HDU_FIELD(has_digit,TYPE_BOOL,7,false,crypt::PasswordGeneratorParameters::DefaultHasDigit)
)

//---------------------------------------------------------------

class App_p
{
    public:

        App* app;

        App_p(App* app) : app(app),
                          fileLogger(std::make_shared<log::FileLogger>()),
                          defaultCryptProviderId(App::DefaultCryptProvider)
        {}

        base::ConfigObject<app_config::type> appConfig;
        base::ConfigObject<logger_config::type> loggerConfig;
        base::ConfigObject<db_config::type> dbConfig;
        base::ConfigObject<crypt_config::type> cryptConfig;
        base::ConfigObject<password_gen_config::type> passwordGenConfig;

        std::map<std::string,LoggerHandlerBuilder,std::less<>> loggerBuilders;
        std::shared_ptr<log::Logger> logger;

        std::vector<std::string> pluginFolders;

        std::shared_ptr<db::DbPlugin> dbPlugin;
        std::shared_ptr<db::Client> dbClient;

        common::SharedPtr<log::TaskLogContext> currentThreadLogCtx;
        std::map<std::string,common::SharedPtr<log::TaskLogContext>> threadLogCtxs;

        Result<std::shared_ptr<crypt::CipherSuites>> initCipherSuites();

        std::shared_ptr<log::FileLogger> fileLogger;

        std::shared_ptr<crypt::CryptPlugin> cryptPlugin;
        std::vector<std::shared_ptr<crypt::CipherSuite>> preloadedCipherSuites;
        std::string defaultCipherSuiteId;
        std::string defaultCryptProviderId;

        std::shared_ptr<db::EncryptionManager> dbEncryptionManager;
        common::SharedPtr<crypt::SymmetricKey> dbEncryptionKey;
        std::shared_ptr<crypt::CipherSuite> dbCipherSuite;

        void loadCryptPlugins();
        void loadDbPlugins();
        void loadPlugins(const std::string& pluginFolder);

        Result<std::shared_ptr<db::DbPlugin>> loadDbPlugin(lib::string_view name);
        Error initDbPlugin();
        db::ClientConfig dbClientConfig(lib::string_view provider);
        void setAppLogger(std::shared_ptr<log::Logger> newLogger)
        {
            logger=std::move(newLogger);
            currentThreadLogCtx=log::makeLogCtx();
            auto& currentLogCtx=currentThreadLogCtx->get<log::Context>();
            currentLogCtx.setLogger(logger.get());
            log::ThreadLocalFallbackContext::set(&currentLogCtx);
        }

        Error loadCryptPlugin(lib::string_view name);        

        std::shared_ptr<common::MappedThreadQWithTaskContext> makeDbMappedThreads(common::ThreadQWithTaskContext* thread) const;
};

//---------------------------------------------------------------

App::App(AppName appName) :
        m_appName(std::move(appName)),
        m_configTree(std::make_shared<HATN_BASE_NAMESPACE::ConfigTree>()),
        m_configTreeLoader(std::make_shared<HATN_BASE_NAMESPACE::ConfigTreeLoader>()),
        m_appThread(nullptr),
        d(std::make_unique<App_p>(this)),
        m_appConfigRoot(AppConfigRoot),
        m_defaultThreadCount(DefaultThreadCount)
{
    auto buildFileLogger=[this]() -> std::shared_ptr<log::LoggerHandler>
    {
        return d->fileLogger;
    };
    registerLoggerHandlerBuilder(log::FileLoggerName,buildFileLogger);
    auto buildStreamLogger=[]() -> std::shared_ptr<log::LoggerHandler>
    {
        return std::make_shared<log::StreamLogger>();
    };
    registerLoggerHandlerBuilder(log::StreamLoggerName,buildStreamLogger);
    d->setAppLogger(log::makeLogger(buildStreamLogger()));

    d->pluginFolders.push_back("plugins");
}

//---------------------------------------------------------------

App::~App()
{}

//---------------------------------------------------------------

void App::logAppStart()
{
    HATN_CTX_INFO_RECORDS_M("****** STARTING APPLICATION ******",HLOG_MODULE(app),{"name",m_appName.displayName})
}

//---------------------------------------------------------------

void App::logAppStop()
{
    HATN_CTX_INFO_RECORDS_M("****** STOPPING APPLICATION ******",HLOG_MODULE(app),{"name",m_appName.displayName})
}

//---------------------------------------------------------------

Error App::loadConfigString(
        common::lib::string_view source,
        const std::string& format
    )
{
    // preload config to find out data dir
    if (m_appDataFolder.empty())
    {
        HATN_BASE_NAMESPACE::ConfigTree t1;
        auto ec=m_configTreeLoader->loadFromString(t1,source,HATN_BASE_NAMESPACE::ConfigTreePath{},format);
        HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load app config from string","app"),HLOG_MODULE(app))

        m_appDataFolder=evalAppDataFolder(t1);
    }
    m_configTree->setDefaultEx(base::ConfigTreePath(m_appConfigRoot).copyAppend("data_folder"),m_appDataFolder);
    m_configTreeLoader->setPrefixSubstitution("$data_dir",m_appDataFolder);

    auto ec=m_configTreeLoader->loadFromString(*m_configTree,source,HATN_BASE_NAMESPACE::ConfigTreePath{},format);
    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load app config from string","app"),HLOG_MODULE(app))
    return applyConfig();
}

//---------------------------------------------------------------

Error App::loadConfigFile(
    const std::string& fileName,
    const std::string& format
    )
{
    // preload config to find out data dir
    if (m_appDataFolder.empty())
    {
        HATN_BASE_NAMESPACE::ConfigTree t1;
        auto ec=m_configTreeLoader->loadFromFile(t1,fileName,HATN_BASE_NAMESPACE::ConfigTreePath{},format);
        HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load app config from file","app"),HLOG_MODULE(app))

        m_appDataFolder=evalAppDataFolder(t1);
    }
    m_configTree->setDefaultEx(base::ConfigTreePath(m_appConfigRoot).copyAppend("data_folder"),m_appDataFolder);
    m_configTreeLoader->setPrefixSubstitution("$data_dir",m_appDataFolder);

    // load app config tree
    auto ec=m_configTreeLoader->loadFromFile(*m_configTree,fileName,HATN_BASE_NAMESPACE::ConfigTreePath{},format);
    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load app config from file","app"),HLOG_MODULE(app))

    // apply config
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
        logAppStart();
        logConfigRecords(_TR("configuration of logger","app"),HLOG_MODULE(app),appLoggerRecords);
        HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load configuration of application logger","app"),HLOG_MODULE(app))
    }

    // init log handler
    base::config_object::LogRecords logHandlerRecords;
    std::string loggerProvider{d->loggerConfig.config().fieldValue(logger_config::provider)};
    auto logHandlerIt=d->loggerBuilders.find(loggerProvider);
    if (logHandlerIt==d->loggerBuilders.end())
    {
        return appError(AppError::UNKNOWN_LOGGER);
    }
    auto logHandler=logHandlerIt->second();
    std::string loggerConfigPath=fmt::format("{}.{}",LoggerConfigRoot,loggerProvider);
    ec=logHandler->loadLogConfig(*m_configTree,loggerConfigPath,logHandlerRecords);
    if (ec)
    {
        logAppStart();
        logConfigRecords(_TR("","app"),HLOG_MODULE(app),appLoggerRecords);
        logConfigRecords(_TR("configuration of logger","app"),HLOG_MODULE(app),logHandlerRecords);
        HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load configuration of logger handler","app"),HLOG_MODULE(app))
    }

    // make and init logger
    base::config_object::LogRecords loggerRecords;
    auto logger=log::makeLogger(logHandler);
    ec=logger->loadLogConfig(*m_configTree,LoggerConfigRoot,loggerRecords);
    if (ec)
    {
        logAppStart();
        logConfigRecords(_TR("configuration of logger","app"),HLOG_MODULE(app),appLoggerRecords);
        logConfigRecords(_TR("configuration of logger","app"),HLOG_MODULE(app),logHandlerRecords);
        logConfigRecords(_TR("configuration of logger","app"),HLOG_MODULE(app),loggerRecords);
        HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load configuration of logger","app"),HLOG_MODULE(app))
    }
    d->setAppLogger(std::move(logger));
    logAppStart();
    logConfigRecords(_TR("configuration of logger","app"),HLOG_MODULE(app),appLoggerRecords);
    logConfigRecords(_TR("configuration of logger","app"),HLOG_MODULE(app),logHandlerRecords);
    logConfigRecords(_TR("configuration of logger","app"),HLOG_MODULE(app),loggerRecords);

    base::config_object::LogRecords logRecords;

    //! @todo validate app config    
    ec=d->appConfig.loadLogConfig(*m_configTree,m_appConfigRoot,logRecords);
    logConfigRecords(_TR("configuration of application","app"),HLOG_MODULE(app),logRecords);
    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load configuration of application","app"),HLOG_MODULE(app))
    const auto& pluginsFolderField=d->appConfig.config().field(app_config::plugin_folders);
    for (size_t i=0;i<pluginsFolderField.count();i++)
    {
        d->pluginFolders.emplace_back(pluginsFolderField.at(i).stringView());
    }

    // load db config
    ec=d->dbConfig.loadLogConfig(*m_configTree,DbConfigRoot,logRecords);
    logConfigRecords(_TR("configuration of application database","app"),HLOG_MODULE(app),logRecords);
    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load configuration of application database","app"),HLOG_MODULE(app))

    // load crypt config
    ec=d->cryptConfig.loadLogConfig(*m_configTree,CryptConfigRoot,logRecords);
    if (!ec)
    {
        bool reloadLogRecords=false;
        if (!d->cryptConfig.config().field(crypt_config::provider).fieldIsSet())
        {
            d->cryptConfig.config().setFieldValue(crypt_config::provider,d->defaultCryptProviderId);
            reloadLogRecords=true;
        }
        if (!d->cryptConfig.config().field(crypt_config::default_cipher_suite).fieldIsSet())
        {
            d->cryptConfig.config().setFieldValue(crypt_config::default_cipher_suite,d->defaultCipherSuiteId);
            reloadLogRecords=true;
        }
        if (!d->preloadedCipherSuites.empty())
        {
            for (const auto& suite : d->preloadedCipherSuites)
            {
                d->cryptConfig.config().field(crypt_config::cipher_suites).append(suite->suite());
            }
            reloadLogRecords=true;
        }
        if (reloadLogRecords)
        {
            logRecords.clear();
            d->cryptConfig.fillLogRecords({},logRecords,CryptConfigRoot);
        }
    }
    logConfigRecords(_TR("configuration of application cryptography","app"),HLOG_MODULE(app),logRecords);
    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load configuration of application cryptography","app"),HLOG_MODULE(app))

    // load password generator config
    if (m_configTree->isSet(PasswordGenConfigRoot,true))
    {
        //! @todo critical: Validate password generator configuration
        ec=d->passwordGenConfig.loadLogConfig(*m_configTree,PasswordGenConfigRoot,logRecords);
        logConfigRecords(_TR("configuration of password generator","app"),HLOG_MODULE(app),logRecords);
        HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to load configuration of password generator","app"),HLOG_MODULE(app))
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
    std::vector<uint32_t> threadGroupCounts;
    const auto& threadConfigs=d->appConfig.config().field(app_config::threads);
    for (size_t i=0;i<threadConfigs.count();i++)
    {
        const auto& threadConfig=threadConfigs.at(i);
        uint32_t groupCount=0;
        if (threadConfig.isSet(thread_config::count_percent))
        {
            groupCount=static_cast<uint32_t>(count * threadConfig.fieldValue(thread_config::count_percent) / 100);
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
    // load plugins
    //! @todo Do not load unused plugins
    d->loadCryptPlugins();
    d->loadDbPlugins();

    // create and start threads
    auto ec=initThreads();
    HATN_CHECK_EC(ec)

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

    // create cipher suites
    auto cipherSuites=d->initCipherSuites();
    if (cipherSuites)
    {
        ec=cipherSuites.takeError();
        HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to init cypher suites","app"),HLOG_MODULE(app))
    }

    //! @todo create translator

    // create env
    m_env=common::makeEnvType<AppEnv>(
        common::subcontexts(
            common::subcontext(),
            common::subcontext(std::move(mappedThreads)),
            common::subcontext(d->logger),
            common::subcontext(),
            common::subcontext(cipherSuites.takeValue()),
            common::subcontext()
        )
    );

    // get factory
    const auto& factory=m_env->get<AllocatorFactory>();

    // init weak pool
    common::pointers_mempool::WeakPool::init(factory.factory()->objectMemoryResource());

    // start logger    
    LogAppConfig appConfig{factory.factory()};
    d->logger->setAppConfig(appConfig);
    ec=d->logger->start();
    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to start logger","app"),HLOG_MODULE(app))

    // init c-ares library
    ec=HATN_NETWORK_NAMESPACE::CaresLib::init(factory.factory());
    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to init c-ares DNS library","app"),HLOG_MODULE(app))

    // done
    return OK;
}

//---------------------------------------------------------------

Error App::closeDb()
{
    HATN_CTX_SCOPE("app::closedb")

    if (m_env)
    {
        HATN_CTX_DEBUG(1,"closing database")

        // close db client
        auto ec=database().close();
        if (ec)
        {
            HATN_CTX_SCOPE_LOCK()
            ec=chainAndLogError(std::move(ec),_TR("failed to close database","app"));
        }
        database().reset();

        HATN_CHECK_EC(ec)
    }

    return OK;
}

//---------------------------------------------------------------

void App::close()
{
    logAppStop();

    std::ignore=closeDb();

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
            std::ignore=chainAndLogError(std::move(ec),fmt::format(_TR("failed to cleanup database plugin","app"),d->dbPlugin->info()->name));
        }
    }

    // close logger
    if (d->logger)
    {
        auto ec=d->logger->close();
        if (ec)
        {
            auto ec1=chainError(std::move(ec),fmt::format(_TR("failed to close logger","app"),d->dbPlugin->info()->name));
            std::cerr << ec1.codeString() << ": " << ec1.message() << std::endl;
        }
    }

    //! @todo Register reset handlers for thread_local static variables
#if 0
    for (auto&& it: m_threads)
    {
        it->start();
        std::ignore=it->execSync(
            []()
            {
                common::ThreadSubcontext<common::TaskSubcontextT<HATN_LOGCONTEXT_NAMESPACE::Context>>::reset();
            }
            );
        it->stop();
    }
#endif

    // destroy env
    m_env.reset();

    // free rocksdb
    freeRocksDb();

    // cleanuo c-ares library
    HATN_NETWORK_NAMESPACE::CaresLib::cleanup();

    // reset log context
    log::ThreadLocalFallbackContext::reset();

    // free weak pool
    common::pointers_mempool::WeakPool::free();
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
}

//---------------------------------------------------------------

std::string App::evalAppDataFolder(const HATN_BASE_NAMESPACE::ConfigTree& configTree) const
{
    auto folder=m_appDataFolder;

    // set app data folder
    if (folder.empty())
    {
        auto r=configTree.get(base::ConfigTreePath(m_appConfigRoot).copyAppend("data_folder"));
        if (!r)
        {
            auto s=r->as<std::string>();
            if (!s)
            {
                folder=s.takeValue();
            }
        }
    }
    if (folder.empty())
    {
        if (folder.empty())
        {
#ifdef _WIN32
            folder=getenv("APPDATA");
#else
            folder=getenv("HOME");
#endif
        }
        lib::filesystem::path p{folder};
#ifdef _WIN32
        p.append(m_appName.execName);
#else
        p.append(fmt::format(".{}",m_appName.execName));
#endif
        folder=p.string();
    }

    return folder;
}

//---------------------------------------------------------------

Error App::createAppDataFolder()
{
    Assert(!m_appDataFolder.empty(),"App data folder must not be empty");

    // create folder
    lib::fs_error_code ec;
    lib::filesystem::create_directories(m_appDataFolder,ec);
    auto ec1=lib::makeFilesystemError(ec);
    HATN_CHECK_CHAIN_LOG_EC(ec1,_TR("failed to create application folder","app"),HLOG_MODULE(app))

    // change permissions
    lib::filesystem::permissions(
        m_appDataFolder,
        lib::filesystem::perms::owner_all,
        lib::filesystem::perm_options::replace,
        ec
    );
    if (ec)
    {
        ec1=lib::makeFilesystemError(ec);
        HATN_CTX_ERROR(ec1,"failed to change permissions of data folder");
    }

    // done
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
        auto plugins=common::PluginLoader::instance().listDynamicPlugins(p.string());
        for (const auto& plugin : plugins)
        {
            HATN_CTX_INFO_RECORDS("found plugin",{"file",plugin->fileName},{"type",plugin->type},{"name",plugin->name},
                                  {"vendor",plugin->vendor},
                                  {"revision",plugin->revision},
                                  {"description",plugin->description}
                            )
        }
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
    std::string nm{name};

    auto r1=common::PluginLoader::instance().loadPlugin(db::DbPlugin::Type,nm);
    Assert(!r1,"failed to load db plugin");
    auto r=common::PluginLoader::instance().loadPlugin<db::DbPlugin>(nm);
    if (r)
    {
        auto ec=common::chainErrors(r.takeError(),db::dbError(db::DbError::DB_PLUGIN_FAILED));
        HATN_CHECK_CHAIN_LOG_EC(ec,nm,HLOG_MODULE(app))
    }
    dbPlugin=r.takeValue();

    // init plugin
    auto ec=dbPlugin->init();
    HATN_CHECK_CHAIN_LOG_EC(ec,fmt::format(_TR("failed to initialize database plugin \"{}\"","app"),nm),HLOG_MODULE(app))

    // done
    return dbPlugin;
}

//---------------------------------------------------------------

db::ClientConfig App_p::dbClientConfig(lib::string_view provider)
{
    auto cfgPath=base::ConfigTreePath{DbConfigRoot}.copyAppend(provider);
    db::ClientConfig cfg{app->m_configTree,
                         app->m_configTree,
                         cfgPath.copyAppend(App::DbConfigMainSection),
                         cfgPath.copyAppend(App::DbConfigOptionsSection)
                         };

    if (dbConfig.config().field(db_config::db_folder).isSet())
    {
        std::string dbPath{dbConfig.config().fieldValue(db_config::db_folder)};
        if (!dbPath.empty())
        {
            lib::filesystem::path p{dbPath};
            if (!p.is_absolute())
            {
                p=app->appDataFolder();
                p.append(dbPath);
                dbPath=p.string();
            }
        }
        cfg.dbPath=dbPath;
    }
    else
    {
        lib::filesystem::path p{app->appDataFolder()};
        p.append("db");
        cfg.dbPath=p.string();
    }

    cfg.dbPrefix=dbConfig.config().fieldValue(db_config::db_prefix);
    cfg.encryptionManager=dbEncryptionManager;
    return cfg;
}

//---------------------------------------------------------------

std::shared_ptr<common::MappedThreadQWithTaskContext> App_p::makeDbMappedThreads(common::ThreadQWithTaskContext* thread) const
{
    //! @todo Use thread tags
    auto mappedThreads=std::make_shared<common::MappedThreadQWithTaskContext>(common::MappedThreadMode::Default,thread);
    uint8_t dbThreadCount=dbConfig.config().fieldValue(db_config::thread_count);
    if (dbThreadCount>app->m_threads.size()-1)
    {
        dbThreadCount=static_cast<uint8_t>(app->m_threads.size()-1);
    }
    if (dbThreadCount>1)
    {
        mappedThreads->setThreadMode(common::MappedThreadMode::Mapped);
        size_t i=0;
        for (auto&& it:app->m_threads)
        {
            mappedThreads->addMappedThread(it.get());
            i++;
            if (i==dbThreadCount)
            {
                break;
            }
        }
    }
    return mappedThreads;
}

//---------------------------------------------------------------

Error App_p::initDbPlugin()
{
    if (!dbPlugin)
    {
        auto name=dbConfig.config().fieldValue(db_config::provider);
        if (name.empty())
        {
            return chainAndLogError(appError(AppError::UNKNOWN_DB_PROVIDER),_TR("name of database provider must not be empty","app"));
        }
        auto plugin=loadDbPlugin(name);
        if (plugin)
        {
            return chainAndLogError(plugin.takeError(),fmt::format(_TR("failed database provider \"{}\"","app"),name));
        }
        dbPlugin=plugin.takeValue();
    }
    return OK;
}

//---------------------------------------------------------------

Error App::openDb(
        bool create
    )
{
    HATN_CTX_SCOPE("app::opendb")
    Error ec;

    // load plugin
    auto provider=d->dbConfig.config().fieldValue(db_config::provider);
    ec=d->initDbPlugin();
    HATN_CHECK_EC(ec)

    // create db client
    d->dbClient=d->dbPlugin->makeClient();
    Assert(d->dbClient,"Failed to create db client in plugin");

    auto thread=appThread();
    Assert(thread!=nullptr,"Threads must be initialized before opening database");

    // open db    
    base::config_object::LogRecords logRecords;
    std::ignore=thread->execSync(
        [this,&ec,&logRecords,&provider,create]()
        {
            auto cfg=d->dbClientConfig(provider);
            ec=d->dbClient->openDb(cfg,logRecords,create);
        }
    );
    logConfigRecords(_TR("configuration of application database","app"),HLOG_MODULE(app),logRecords);

    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to open application database","app"),HLOG_MODULE(app))

    // set db client in env
    auto mappedThreads=d->makeDbMappedThreads(thread);
    auto asyncDbClient=std::make_shared<db::AsyncClient>(d->dbClient,std::move(mappedThreads));
    database().setDbClient(std::move(asyncDbClient));

    // done
    return OK;
}

//---------------------------------------------------------------

Result<std::shared_ptr<db::AsyncClient>> App::openAdditionalDatabase(
        const std::string& name,
        const db::ClientConfig& config,
        bool create
    )
{
    HATN_CTX_SCOPE("app::openadditionaldatabase")
    Error ec;

    // create db client
    auto dbClient=d->dbPlugin->makeClient();
    Assert(dbClient,"Failed to create db client in plugin");

    auto thread=appThread();

    // open db
    base::config_object::LogRecords logRecords;
    logRecords.emplace_back(base::config_object::LogRecord{"db_name",name});
    std::ignore=thread->execSync(
        [dbClient,&config,&ec,&logRecords,create]()
        {
            ec=dbClient->openDb(config,logRecords,create);
        }
    );
    logConfigRecords(_TR("configuration of additional database","app"),HLOG_MODULE(app),logRecords);

    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to open additional database","app"),HLOG_MODULE(app))

    // set db client in env
    auto mappedThreads=d->makeDbMappedThreads(thread);
    auto asyncDbClient=std::make_shared<db::AsyncClient>(dbClient,std::move(mappedThreads));

    // done
    return asyncDbClient;
}

//---------------------------------------------------------------

Error App::destroyDb()
{
    HATN_CTX_SCOPE("app::destroydb")

    // load plugin
    auto name=d->dbConfig.config().fieldValue(db_config::provider);
    auto plugin=d->loadDbPlugin(name);
    if (plugin)
    {
        return plugin.takeError();
    }
    d->dbPlugin=plugin.takeValue();

    // create db client
    auto dbClient=d->dbPlugin->makeClient();
    Assert(dbClient,"Failed to create db client in plugin");

    // destroy db
    base::config_object::LogRecords logRecords;
    auto ec=dbClient->destroyDb(d->dbClientConfig(name),logRecords);
    logConfigRecords(_TR("configuration of application database","app"),HLOG_MODULE(app),logRecords);
    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to destroy application database","app"),HLOG_MODULE(app))

    // done
    return OK;
}

//---------------------------------------------------------------

void App::registerDbSchema(std::shared_ptr<db::Schema> schema)
{
    for (auto&& provider: schema->modelProviders())
    {
        provider->registerRocksdbModels();
    }

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    HATN_ROCKSDB_NAMESPACE::RocksdbSchemas::instance().registerSchema(std::move(schema));
#else
    std::ignore=schema;
#endif
}

//---------------------------------------------------------------

void App::unregisterDbSchema(const std::string& name)
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    HATN_ROCKSDB_NAMESPACE::RocksdbSchemas::instance().unregisterSchema(name);
#else
    std::ignore=name;
#endif
}

//---------------------------------------------------------------

void App::freeDbSchemas()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    HATN_ROCKSDB_NAMESPACE::RocksdbSchemas::free();
#endif
}

//---------------------------------------------------------------

std::vector<std::string> App::listLogFiles() const
{
    return d->fileLogger->listFiles();
}

//---------------------------------------------------------------

Result<std::shared_ptr<crypt::CipherSuites>> App_p::initCipherSuites()
{
    HATN_CTX_SCOPE("app::initciphersuites")

    auto suites=std::make_shared<crypt::CipherSuites>();

    // load crypt plugin
    auto cryptProvider=cryptConfig.config().fieldValue(crypt_config::provider);
    if (cryptProvider.empty())
    {
        return suites;
    }
    auto ec=loadCryptPlugin(cryptProvider);
    HATN_CHECK_EC(ec)

    // set crypt engine
    auto cryptEngine=std::make_shared<crypt::CryptEngine>(cryptPlugin.get());    
    suites->setDefaultEngine(std::move(cryptEngine));

    // fill cipher suites
    const auto& cipherSuites=cryptConfig.config().field(crypt_config::cipher_suites);
    for (size_t i=0;i<cipherSuites.count();i++)
    {
        auto suiteConfig=cipherSuites.at(i).sharedValue();
        auto suite=std::make_shared<crypt::CipherSuite>(std::move(suiteConfig));
        suites->addSuite(std::move(suite));
    }

    // set default cipher suite
    auto defaultSuiteId=cryptConfig.config().fieldValue(crypt_config::default_cipher_suite);
    HATN_CTX_SCOPE_PUSH("default_cipher_suite",defaultSuiteId)

    ec=suites->setDefaultSuite(defaultSuiteId);
    HATN_CHECK_CHAIN_LOG_EC(ec,_TR("failed to set default cipher suite","app"),HLOG_MODULE(app))

    auto dbCipherSuiteId=defaultSuiteId;
    if (dbConfig.config().isSet(db_config::cipher_suite))
    {
        dbCipherSuiteId=dbConfig.config().fieldValue(db_config::cipher_suite);
    }
    dbCipherSuite=suites->suiteShared(dbCipherSuiteId);
    if (!dbCipherSuite)
    {
        return appError(AppError::INVALID_DB_CIPHER_SUITE);
    }

    // init password generator
    crypt::PasswordGeneratorParameters passwordGenParams;
    passwordGenParams.minLength=passwordGenConfig.config().fieldValue(password_gen_config::min_length);
    passwordGenParams.maxLength=passwordGenConfig.config().fieldValue(password_gen_config::max_length);
    passwordGenParams.lettersWeight=passwordGenConfig.config().fieldValue(password_gen_config::letters_weight);
    passwordGenParams.digitsWeight=passwordGenConfig.config().fieldValue(password_gen_config::digits_weight);
    passwordGenParams.specialsWeight=passwordGenConfig.config().fieldValue(password_gen_config::specials_weight);
    passwordGenParams.hasSpecial=passwordGenConfig.config().fieldValue(password_gen_config::has_special);
    passwordGenParams.hasDigit=passwordGenConfig.config().fieldValue(password_gen_config::has_digit);
    suites->passwordGenerator()->setDefaultParameters(passwordGenParams);

    // done
    return suites;
}

//---------------------------------------------------------------

Error App::setPreloadedCipherSuites(const std::vector<std::string>& suitesJson)
{
    HATN_CTX_SCOPE("app::setpreloadedciphersuites")

    d->preloadedCipherSuites.clear();
    for (const auto& json : suitesJson)
    {
        try
        {
            auto suite=crypt::CipherSuite::fromJSON(json);
            d->preloadedCipherSuites.emplace_back(std::move(suite));
        }
        catch (const common::ErrorException& ex)
        {
            auto ec=ex.error();
            auto msg=fmt::format(_TR("failed to preload cipher suite\n{}\n{}"),json,du::RawError::threadLocal().message);
            HATN_CTX_ERROR(ec,msg.c_str())
            return ec;
        }
    }

    return OK;
}

//---------------------------------------------------------------

std::vector<std::shared_ptr<crypt::CipherSuite>> App::preloadedCipherSuites() const
{
    return d->preloadedCipherSuites;
}

//---------------------------------------------------------------

void App::setDefaultCipherSuiteId(std::string id)
{
    d->defaultCipherSuiteId=std::move(id);
}
//---------------------------------------------------------------

std::string App::defaultCipherSuiteId() const
{
    return d->defaultCipherSuiteId;
}

//---------------------------------------------------------------

void App::setDefaultCryptProviderId(std::string id)
{
    d->defaultCryptProviderId=std::move(id);
}

//---------------------------------------------------------------

std::string App::defaultCryptProviderId() const
{
    return d->defaultCryptProviderId;
}

//---------------------------------------------------------------

Error App_p::loadCryptPlugin(lib::string_view nameView)
{
    // only ine cryptplugin can be loaded
    if (cryptPlugin)
    {
        return OK;
    }

    // load plugin
    std::string name{nameView};
    auto r=common::PluginLoader::instance().loadPlugin<crypt::CryptPlugin>(name);
    if (r)
    {
        auto ec=common::chainErrors(r.takeError(),crypt::cryptError(crypt::CryptError::CRYPT_PLUGIN_FAILED));
        return chainAndLogError(std::move(ec),fmt::format("\"{}\"",name));
    }
    cryptPlugin=r.takeValue();

    // init plugin
    auto ec=cryptPlugin->init();
    if (ec)
    {
        return common::chainError(std::move(ec),fmt::format(_TR("failed to initialize cryptographic plugin \"{}\"","app"),name));
    }

    // done
    return OK;
}

//---------------------------------------------------------------

void App::setDbEncryptionKey(common::SharedPtr<crypt::SymmetricKey> key)
{
    d->dbEncryptionKey=std::move(key);
    if (d->dbEncryptionManager)
    {
        d->dbEncryptionManager->setDefaultKey(d->dbEncryptionKey);
    }
}

//---------------------------------------------------------------

std::shared_ptr<db::EncryptionManager> App::makeDbEncryptionManager()
{
    if (d->dbEncryptionManager)
    {
        return d->dbEncryptionManager;
    }

    d->dbEncryptionManager=std::make_shared<db::EncryptionManager>(allocatorFactory().factory());
    d->dbEncryptionManager->setSuite(d->dbCipherSuite);
    d->dbEncryptionManager->setDefaultKey(d->dbEncryptionKey);
    d->dbEncryptionManager->setChunkMaxSize(d->dbConfig.config().fieldValue(db_config::encryption_chunk_size));
    d->dbEncryptionManager->setFirstChunkMaxSize(d->dbConfig.config().fieldValue(db_config::encryption_first_chunk_size));
    if (d->dbConfig.config().isSet(db_config::main_db_path))
    {
        d->dbEncryptionManager->addDb(std::string{d->dbConfig.config().fieldValue(db_config::main_db_path)});
    }
    return d->dbEncryptionManager;
}

//---------------------------------------------------------------

std::shared_ptr<db::EncryptionManager> App::dbEncryptionManager() const
{
    return d->dbEncryptionManager;
}

//---------------------------------------------------------------

std::shared_ptr<db::ClientEnvironment> App::dbEnvironment() const
{
    if (d->dbClient->isOpen())
    {
        return d->dbClient->cloneEnvironment();
    }
    return std::shared_ptr<db::ClientEnvironment>{};
}

//---------------------------------------------------------------

bool App::isDbEncrypted() const noexcept
{
    return d->dbConfig.config().fieldValue(db_config::encrypted);
}

//---------------------------------------------------------------

bool App::dbLocalDirExists() const
{
    if (d->dbConfig.config().isSet(db_config::main_db_path))
    {
        std::string path{d->dbConfig.config().fieldValue(db_config::main_db_path)};
        return lib::filesystem::exists(path);
    }
    return false;
}

//---------------------------------------------------------------

const HATN_CRYPT_NAMESPACE::CipherSuite* App::defaultCipherSuite() const noexcept
{
    return cipherSuites().suites()->defaultSuite();
}

//---------------------------------------------------------------

const HATN_CRYPT_NAMESPACE::CipherSuite* App::storageCipherSuite() const noexcept
{
    if (d->dbCipherSuite)
    {
        return d->dbCipherSuite.get();
    }
    return defaultCipherSuite();
}

//---------------------------------------------------------------

bool App::isDbOpen() const
{
    if (!database().hasDbClient())
    {
        return false;
    }

    return database().dbClient()->client()->isOpen();
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE::ConfigTreePath App::dbConfigProviderPath() const
{
    return HATN_BASE_NAMESPACE::ConfigTreePath{DbConfigRoot}.copyAppend(d->dbConfig.config().fieldValue(db_config::provider));
}

//---------------------------------------------------------------

HATN_APP_NAMESPACE_END
