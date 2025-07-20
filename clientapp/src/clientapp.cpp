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
};

//--------------------------------------------------------------------------

ClientApp::ClientApp(HATN_APP_NAMESPACE::AppName appName) : pimpl(std::make_unique<ClientApp_p>(std::move(appName)))
{
    pimpl->appSettings=std::make_shared<ClientAppSettings>(this);
    pimpl->lockingController=std::make_shared<LockingController>(this);
}

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

EventDispatcher& ClientApp::eventDispatcher()
{
    return pimpl->eventDispatcher;
}

//--------------------------------------------------------------------------

Error ClientApp::init()
{
    return app().init();
}

//--------------------------------------------------------------------------

Error ClientApp::initBridge()
{
    bridge().registerService(std::make_shared<SystemService>(this));
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
    HATN_CHECK_EC(ec)

    // done
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

HATN_CLIENTAPP_NAMESPACE_END
