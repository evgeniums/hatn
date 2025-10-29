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

#include <hatn/common/flatmap.h>
#include <hatn/common/plainfile.h>

#include <hatn/app/app.h>

#include <hatn/clientapp/clientbridge.h>
#include <hatn/clientapp/eventdispatcher.h>
#include <hatn/clientapp/systemservice.h>
#include <hatn/clientapp/clientappdbmodelsprovider.h>
#include <hatn/clientapp/clientappdbmodels.h>
#include <hatn/clientapp/clientappsettings.h>
#include <hatn/clientapp/lockingcontroller.h>
#include <hatn/clientapp/clientapp.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

class ClientApp_p
{
    public:

        ClientApp_p(HATN_APP_NAMESPACE::AppName appName) : app(std::move(appName))
        {}

        HATN_APP_NAMESPACE::App app;
        Dispatcher bridge;
        EventDispatcher eventDispatcher;

        common::FlatMap<std::string,common::SharedPtr<crypt::SymmetricKey>,std::less<>> encryptionKeys;

        std::string mainDbType=ClientApp::DbMain;
        std::string mainStorageKeyName=ClientApp::MainStorageKey;

        std::map<std::string,std::shared_ptr<db::Schema>> dbSchemas;
        std::shared_ptr<ClientAppSettings> appSettings;
        std::shared_ptr<LockingController> lockingController;

        bool open=false;
};

//--------------------------------------------------------------------------

ClientApp::ClientApp(HATN_APP_NAMESPACE::AppName appName) : pimpl(std::make_unique<ClientApp_p>(std::move(appName)))
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
        lib::filesystem::path initFilePath{app().appDataFolder()};
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
    HATN_CTX_SCOPE("clientapp::closedata")

    if (!pimpl->open)
    {
        return OK;
    }

    // close data in derived class
    auto ec=doCloseData();
    if (ec)
    {
        HATN_CTX_SCOPE_LOCK()
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

    // remove data directory
    auto appDataFolder=app().appDataFolder();
    if (!appDataFolder.empty())
    {
        lib::fs_error_code fsErr;
        lib::filesystem::remove_all(appDataFolder,fsErr);
        if (fsErr)
        {
            return chainAndLogError(lib::makeFilesystemError(fsErr),_TR("failed to remove application data folder","clientapp"));
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

void ClientApp::flushAppSettings(std::string section)
{
    auto ctx=bridge().defaultContextBuilder()->makeContext(bridge().defaultEnv());
    pimpl->appSettings->flush(std::move(ctx),[](common::SharedPtr<Context>, const Error&){},std::move(section));
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
    lib::filesystem::path initFilePath{app().appDataFolder()};
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

HATN_CLIENTAPP_NAMESPACE_END
