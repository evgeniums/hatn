/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file clientapp/clientapp.h
  */

/****************************************************************************/

#ifndef HATNCLIENTAPP_H
#define HATNCLIENTAPP_H

#include <memory>

#include <hatn/common/error.h>
#include <hatn/common/taskcontext.h>
#include <hatn/crypt/crypt.h>
#include <hatn/db/db.h>

#include <hatn/app/appname.h>

#include <hatn/clientapp/clientappdefs.h>

HATN_CRYPT_NAMESPACE_BEGIN
class SymmetricKey;
HATN_CRYPT_NAMESPACE_END

HATN_DB_NAMESPACE_BEGIN
class AsyncDb;
class Schema;
HATN_DB_NAMESPACE_END

HATN_APP_NAMESPACE_BEGIN
class App;
HATN_APP_NAMESPACE_END

HATN_CLIENTAPP_NAMESPACE_BEGIN

using Context=common::TaskContext;

class Dispatcher;
class EventDispatcher;
class ClientApp_p;
class ClientAppSettings;
class LockingController;

class HATN_CLIENTAPP_EXPORT ClientApp
{
    public:

        constexpr static const char* DbMain="main";
        constexpr static const char* DbNotifications="notifications";

        constexpr static const char* MainStorageKey="main";
        constexpr static const char* NotificationsStorageKey="notifications";

        constexpr static const char* PassphraseKey="passphrase";
        constexpr static const char* PincodeKey="pincode";

        constexpr static const char* DataInitFile=".init";

        ClientApp(HATN_APP_NAMESPACE::AppName appName);
        virtual ~ClientApp();

        ClientApp(const ClientApp&)=delete;
        ClientApp(ClientApp&&)=delete;
        ClientApp& operator =(const ClientApp&)=delete;
        ClientApp& operator =(ClientApp&&)=delete;

        HATN_APP_NAMESPACE::App& app();
        const HATN_APP_NAMESPACE::App& app() const;

        Dispatcher& bridge();

        const Dispatcher& bridge() const;

        EventDispatcher& eventDispatcher();

        virtual Error init();

        virtual HATN_NAMESPACE::Error preloadConfig()
        {
            return OK;
        }

        Error initBridge();

        virtual Error initBridgeServices()
        {
            return OK;
        }

        virtual void initBridgeConfirmations()
        {
        }

        Error initDb();

        void loadEncryptionKey(
            std::string name,
            common::SharedPtr<crypt::SymmetricKey> key
        );

        common::SharedPtr<crypt::SymmetricKey> encryptionKey(lib::string_view name) const;

        void removeEncryptionKey(lib::string_view name);

        void clearEncryptionKeys();

        void setMainDbType(std::string name);
        const std::string& mainDbType() const;

        void setMainStorageKeyName(std::string name);
        const std::string& mainStorageKeyName() const;

        Error openMainDb(bool create=true);

        HATN_DB_NAMESPACE::AsyncDb& mainDb();

        const ClientAppSettings* appSettings() const;
        ClientAppSettings* appSettings();

        void flushAppSettings(std::string section={});

        const LockingController* lockingController() const;
        LockingController* lockingController();

        Error openData(bool init);
        Error closeData();
        Error removeData();

        bool appDataInitialized() const;

        virtual hatn::Error initTests();

        std::shared_ptr<db::Schema> dbSchema(const std::string& name) const;

    protected:

        virtual Error doInitDbSchemas(std::map<std::string,std::shared_ptr<db::Schema>>& schemas)
        {
            std::ignore=schemas;
            return OK;
        }

        virtual Error doOpenData(bool init)
        {
            std::ignore=init;
            return OK;
        }

        virtual Error doCloseData()
        {
            return OK;
        }

        virtual Error doRemoveData()
        {
            return OK;
        }

    private:

        std::unique_ptr<ClientApp_p> pimpl;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTAPP_H
