/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientapp.сpp
  *
  */

#include <future>

#include <hatn/common/flatmap.h>
#include <hatn/common/plainfile.h>
#include <hatn/crypt/securekey.h>

#include <hatn/app/app.h>

#include <hatn/logcontext/loggerconfig.h>

#include <hatn/clientapp/clientbridge.h>
#include <hatn/clientapp/eventdispatcher.h>
#include <hatn/clientapp/systemservice.h>
#include <hatn/clientapp/clientappdbmodelsprovider.h>
#include <hatn/clientapp/clientappdbmodels.h>
#include <hatn/clientapp/clientappsettings.h>
#include <hatn/clientapp/clientappfilesettings.h>
#include <hatn/clientapp/lockingcontroller.h>
#include <hatn/clientapp/methodsetloggerconfig.h>
#include <hatn/clientapp/feedbackprovider.h>
#include <hatn/clientapp/logsprovider.h>
#include <hatn/clientapp/crashreporter.h>
#include <hatn/clientapp/clientapp.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

class ClientApp_p
{
    public:

        ClientApp_p(HATN_APP_NAMESPACE::AppName appName, ClientApp* owner)
            : app(std::move(appName)), fileSettings(owner)
        {}

        HATN_APP_NAMESPACE::App app;
        Dispatcher bridge;
        EventDispatcher eventDispatcher;

        common::FlatMap<std::string,common::SharedPtr<crypt::SymmetricKey>,std::less<>> encryptionKeys;

        std::string mainDbType=ClientApp::DbMain;
        std::string mainStorageKeyName=ClientApp::MainStorageKey;

        std::map<std::string,std::shared_ptr<db::Schema>> dbSchemas;
        std::shared_ptr<ClientAppSettings> appSettings;
        ClientAppFileSettings fileSettings;
        std::shared_ptr<LockingController> lockingController;

        FeedbackProviderRegistry feedbackProviderRegistry;
        LogsProviderRegistry logsProviderRegistry;
        CrashReporterRegistry crashReporterRegistry;

        bool open=false;
        int closeDataTimeoutMs=5000;
};

//--------------------------------------------------------------------------

ClientApp::ClientApp(HATN_APP_NAMESPACE::AppName appName) : pimpl(std::make_unique<ClientApp_p>(std::move(appName), this))
{}

//--------------------------------------------------------------------------

ClientApp::~ClientApp()
{}

//--------------------------------------------------------------------------

HATN_APP_NAMESPACE::App& ClientApp::app()
{
    return pimpl->app;
}

//--------------------------------------------------------------------------

const HATN_APP_NAMESPACE::App& ClientApp::app() const
{
    return pimpl->app;
}

//--------------------------------------------------------------------------

Dispatcher& ClientApp::bridge()
{
    return pimpl->bridge;
}

//--------------------------------------------------------------------------

const Dispatcher& ClientApp::bridge() const
{
    return pimpl->bridge;
}

//--------------------------------------------------------------------------

EventDispatcher& ClientApp::eventDispatcher()
{
    return pimpl->eventDispatcher;
}

//--------------------------------------------------------------------------

Error ClientApp::init()
{
    // init app
    auto ec=app().init();
    if (!ec)
    {
        pimpl->appSettings=std::make_shared<ClientAppSettings>(this);
        pimpl->lockingController=std::make_shared<LockingController>(this);
    }
    HATN_CHECK_EC(ec);

    // init locking controller
    pimpl->lockingController->init();

    // load file settings (no DB required)
    {
        lib::filesystem::path filePath{app().appDataFolder()};
        filePath.append(ClientAppFileSettings::FileName);
        ec = pimpl->fileSettings.load(filePath.string());
        if (ec)
        {
            HATN_CTX_ERROR(ec,"failed to load file settings")
        }
    }

    // Apply persisted custom logger settings on top of the app-config defaults.
    // File settings have just been loaded; the app logger already carries the
    // config-file defaults set during app().init().
    {
        common::Error lec;
        auto json = pimpl->fileSettings.getJson(
            HATN_BASE_NAMESPACE::ConfigTreePath{HATN_LOGCONTEXT_NAMESPACE::LoggerConfigSettingsPath},
            lec
        );
        if (!lec && !json.empty())
        {
            auto* loggerWrapper = &app().logger();
            if (loggerWrapper->logger())
            {
                lec = applyLoggerConfigJson(loggerWrapper->logger(), json);
                if (lec)
                {
                    HATN_CTX_ERROR(lec, "failed to apply custom logger config from file settings")
                }
            }
        }
    }

    // done
    return OK;
}

//--------------------------------------------------------------------------

Error ClientApp::initBridge()
{
    bridge().registerService(std::make_shared<SystemService>(this));
    initBridgeConfirmations();
    return initBridgeServices();
}

//--------------------------------------------------------------------------

void ClientApp::loadEncryptionKey(
        std::string name,
        common::SharedPtr<crypt::SymmetricKey> key
    )
{
    pimpl->encryptionKeys.emplace(std::move(name),std::move(key));
}

//--------------------------------------------------------------------------

void ClientApp::removeEncryptionKey(lib::string_view name)
{
    pimpl->encryptionKeys.erase(name);
}

//--------------------------------------------------------------------------

void ClientApp::clearEncryptionKeys()
{
    pimpl->encryptionKeys.clear();
}

//--------------------------------------------------------------------------

common::SharedPtr<crypt::SymmetricKey> ClientApp::encryptionKey(lib::string_view name) const
{
    auto it=pimpl->encryptionKeys.find(name);
    if (it!=pimpl->encryptionKeys.end())
    {
        return it->second;
    }
    return common::SharedPtr<crypt::SymmetricKey>{};
}

//--------------------------------------------------------------------------

Error ClientApp::initDb()
{
    // create db schema
    auto mainSchema=std::make_shared<HATN_DB_NAMESPACE::Schema>(mainDbType());    
    mainSchema->addModelsProvider(std::make_shared<ClientAppDbModelsProvider>(ClientAppDbModels::defaultInstance()));
    pimpl->dbSchemas.emplace(mainDbType(),mainSchema);

    // init db schema in derived class
    auto ec=doInitDbSchemas(pimpl->dbSchemas);
    HATN_CHECK_EC(ec)

    // register db schemas in app
    for (auto&& schema : pimpl->dbSchemas)
    {
        app().registerDbSchema(schema.second);
    }

    // done
    return OK;
}

//--------------------------------------------------------------------------

Error ClientApp::openMainDb(bool create)
{
    HATN_CTX_SCOPE("clientapp::openmaindb")

    // init database encryption
    if (app().isDbEncrypted())
    {
        auto key=encryptionKey(mainStorageKeyName());
        if (!key)
        {
            if (mainStorageKeyName()==NotificationsStorageKey)
            {
                return clientAppError(ClientAppError::STORAGE_NOTIFICATION_KEY_REQUIRED);
            }
            return clientAppError(ClientAppError::STORAGE_KEYS_REQUIRED);
        }
        app().setDbEncryptionKey(std::move(key));
        app().makeDbEncryptionManager();
    }

    // open main db
    auto ec=app().openDb(create);
    if (ec)
    {
        HATN_CTX_SCOPE_ERROR("failed to open main database")
        return ec;
    }

    // set main schema to db client
    auto mainDbSchemaIt=pimpl->dbSchemas.find(mainDbType());
    Assert(mainDbSchemaIt!=pimpl->dbSchemas.end(),"Main database schema not initialized yet");
    ec=app().database().setSchema(mainDbSchemaIt->second);
    if (ec)
    {
        HATN_CTX_SCOPE_ERROR("failed to set main database schema to client")
        return ec;
    }

    // done
    return OK;
}

//--------------------------------------------------------------------------

Error ClientApp::openData(bool init)
{
    HATN_CTX_SCOPE("clientapp::opendata")

    // open main database
    auto ec=openMainDb(init);
    HATN_CHECK_EC(ec)

    HATN_CTX_DEBUG("main db opened")

    // load app settings
    ec=pimpl->appSettings->load();
    if (ec)
    {
        HATN_CTX_ERROR(ec,"failed to app settings")
    }

    // start locking controller
    pimpl->lockingController->start();

    // open data in derived class
    ec=doOpenData(init);
    HATN_CHECK_EC(ec)

    // create folder-ready file
    if (init)
    {
        // ensure the data folder exists before writing the init marker
        auto ec0=app().createDataFolder();
        if (ec0)
        {
            HATN_CTX_SCOPE_ERROR("failed to create data folder before writing init file")
            return ec0;
        }

        lib::filesystem::path initFilePath{app().dataFolder()};
        initFilePath.append(DataInitFile);
        common::PlainFile initFile;
        ec=initFile.open(initFilePath.string(),common::File::Mode::write_new);
        if (ec)
        {
            HATN_CTX_SCOPE_ERROR("failed to open init file")
            return ec;
        }
        initFile.close();

        lib::fs_error_code fsec;
        lib::filesystem::permissions(
            initFilePath,
            lib::filesystem::perms::owner_read,
            lib::filesystem::perm_options::replace,
            fsec
        );
        if (fsec)
        {
            auto ec1=lib::makeFilesystemError(fsec);
            HATN_CTX_ERROR(ec1,"failed to change permissions of .init file");
        }
    }

    // done
    pimpl->open=true;
    return OK;
}

//--------------------------------------------------------------------------

Error ClientApp::closeData()
{
    if (!pimpl->open)
    {
        return OK;
    }

    // build context for async close
    auto ctx=bridge().defaultContextBuilder()->makeContext(bridge().defaultEnv());

    // close data in derived class
    Error ec;
    auto appThread=app().appThread();
    if (appThread!=nullptr
        && appThread->isStarted()
        && common::Thread::currentThread()!=appThread)
    {
        // Post doCloseData to the app thread and block on completion via std::future.
        std::promise<Error> promise;
        auto future=promise.get_future();
        appThread->execAsync(
            [this,ctx,&promise]()
            {
                doCloseData(ctx,
                    [&promise](common::SharedPtr<Context>, const Error& err)
                    {
                        promise.set_value(err);
                    });
            }
        );
        if (future.wait_for(std::chrono::milliseconds(pimpl->closeDataTimeoutMs))==std::future_status::timeout)
        {
            ec=commonError(CommonError::TIMEOUT);
        }
        else
        {
            ec=future.get();
        }
    }
    else
    {
        // App thread not running, or we are already on it — call directly
        // with no callback to avoid a self-deadlock.
        doCloseData(std::move(ctx),{});
    }
    if (ec)
    {
        HATN_CTX_ERROR(ec,"failed to close data in derived class")
    }

    // close locking controller
    pimpl->lockingController->close();

    // close main database
    auto ec1=app().closeDb();

    HATN_CHECK_EC(ec)
    HATN_CHECK_EC(ec1)

    pimpl->open=false;
    return OK;
}

//--------------------------------------------------------------------------

Error ClientApp::removeData()
{
    // remove data in derived class
    auto ec=doRemoveData();
    if (ec)
    {
        ec=chainAndLogError(std::move(ec),_TR("failed to remove data in derived class","clientapp"));
    }

    // destroy main database
    auto ec1=app().destroyDb();
    if (ec1)
    {
        ec1=chainAndLogError(std::move(ec1),_TR("failed to destroy main database","clientapp"));
    }

    // remove data directory (dataFolder() is separate from appDataFolder() when explicitly
    // configured, so this removes only the user data, not the metadata folder)
    auto dataFolder=app().dataFolder();
    if (!dataFolder.empty())
    {
        lib::fs_error_code fsErr;
        lib::filesystem::remove_all(dataFolder,fsErr);
        if (fsErr)
        {
            return chainAndLogError(lib::makeFilesystemError(fsErr),_TR("failed to remove data folder","clientapp"));
        }
    }

    // done
    HATN_CHECK_EC(ec1)
    HATN_CHECK_EC(ec)
    return OK;
}

//--------------------------------------------------------------------------

HATN_DB_NAMESPACE::AsyncDb& ClientApp::mainDb()
{
    return app().database();
}

//--------------------------------------------------------------------------

void ClientApp::setMainDbType(std::string name)
{
    pimpl->mainDbType=std::move(name);
}

//--------------------------------------------------------------------------

const std::string& ClientApp::mainDbType() const
{
    return pimpl->mainDbType;
}

//--------------------------------------------------------------------------

void ClientApp::setMainStorageKeyName(std::string name)
{
    pimpl->mainStorageKeyName=std::move(name);
}

//--------------------------------------------------------------------------

const std::string& ClientApp::mainStorageKeyName() const
{
    return pimpl->mainStorageKeyName;
}

//--------------------------------------------------------------------------

void ClientApp::setCloseDataTimeoutMs(int ms) noexcept
{
    pimpl->closeDataTimeoutMs=ms;
}

int ClientApp::closeDataTimeoutMs() const noexcept
{
    return pimpl->closeDataTimeoutMs;
}

//--------------------------------------------------------------------------

const ClientAppSettings* ClientApp::appSettings() const
{
    return pimpl->appSettings.get();
}

//--------------------------------------------------------------------------

ClientAppSettings* ClientApp::appSettings()
{
    return pimpl->appSettings.get();
}

//--------------------------------------------------------------------------

const ClientAppFileSettings& ClientApp::fileSettings() const
{
    return pimpl->fileSettings;
}

//--------------------------------------------------------------------------

ClientAppFileSettings& ClientApp::fileSettings()
{
    return pimpl->fileSettings;
}

//--------------------------------------------------------------------------

void ClientApp::flushAppSettings(std::string section)
{
    if (appDataInitialized())
    {
        auto ctx=bridge().defaultContextBuilder()->makeContext(bridge().defaultEnv());
        pimpl->appSettings->flush(std::move(ctx),[](common::SharedPtr<Context>, const Error&){},std::move(section));
    }
}

//--------------------------------------------------------------------------

const LockingController* ClientApp::lockingController() const
{
    return pimpl->lockingController.get();
}

//--------------------------------------------------------------------------

LockingController* ClientApp::lockingController()
{
    return pimpl->lockingController.get();
}

//--------------------------------------------------------------------------

bool ClientApp::appDataInitialized() const
{
    lib::filesystem::path initFilePath{app().dataFolder()};
    initFilePath.append(DataInitFile);

    lib::fs_error_code fsec;
    auto ok=lib::filesystem::exists(initFilePath,fsec);
    if (fsec)
    {
        auto ec1=lib::makeFilesystemError(fsec);
        HATN_CTX_ERROR(ec1,"failed to check existence of .init file");
    }
    return ok;
}

//--------------------------------------------------------------------------

Error ClientApp::initTests()
{
    return OK;
}

//--------------------------------------------------------------------------

std::shared_ptr<db::Schema> ClientApp::dbSchema(const std::string& name) const
{
    auto it=pimpl->dbSchemas.find(name);
    if (it!=pimpl->dbSchemas.end())
    {
        return it->second;
    }
    return std::shared_ptr<db::Schema>{};
}

//--------------------------------------------------------------------------

void ClientApp::publishEvent(
        std::shared_ptr<HATN_APP_NAMESPACE::Event> event,
        const std::string& envId
    )
{
    HATN_CLIENTAPP_NAMESPACE::DefaultContextBuilder ctxBuilder{};
    auto env=bridge().env(envId);
    auto ctx=ctxBuilder.makeContext(app().env());
    ctx->beforeThreadProcessing();

    {
        eventDispatcher().publish(
            env,
            ctx,
            std::move(event)
        );
    }

    ctx->afterThreadProcessing();
}

//--------------------------------------------------------------------------

FeedbackProviderRegistry& ClientApp::feedbackProviderRegistry() noexcept
{
    return pimpl->feedbackProviderRegistry;
}

//--------------------------------------------------------------------------

const FeedbackProviderRegistry& ClientApp::feedbackProviderRegistry() const noexcept
{
    return pimpl->feedbackProviderRegistry;
}

//--------------------------------------------------------------------------

LogsProviderRegistry& ClientApp::logsProviderRegistry() noexcept
{
    return pimpl->logsProviderRegistry;
}

//--------------------------------------------------------------------------

const LogsProviderRegistry& ClientApp::logsProviderRegistry() const noexcept
{
    return pimpl->logsProviderRegistry;
}

//--------------------------------------------------------------------------

CrashReporterRegistry& ClientApp::crashReporterRegistry() noexcept
{
    return pimpl->crashReporterRegistry;
}

//--------------------------------------------------------------------------

const CrashReporterRegistry& ClientApp::crashReporterRegistry() const noexcept
{
    return pimpl->crashReporterRegistry;
}

//--------------------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
