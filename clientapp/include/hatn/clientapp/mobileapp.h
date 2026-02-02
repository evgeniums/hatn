/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file clientapp/mobileapp.h
*/

/****************************************************************************/

#ifndef HATNMOBILEAPP_H
#define HATNMOBILEAPP_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include <hatn/app/appname.h>

#include <hatn/clientapp/clientappdefs.h>
#include <hatn/clientapp/confirmationdescriptor.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class ClientApp;

HATN_CLIENTAPP_NAMESPACE_END

HATN_CLIENTAPP_MOBILE_NAMESPACE_BEGIN

class MobilePlatformContext;
class MobileApp_p;

struct Request
{
    std::string envId;
    std::string topic;
    std::string messageTypeName;
    std::string messageJson;
    std::vector<std::vector<const char>> buffers;
    ConfirmationDescriptor confirmation;

    Request()
    {}

    Request(
        std::string envId,
        std::string topic,
        std::string messageTypeName
        ) : envId(std::move(envId)),
            topic(std::move(topic)),
            messageTypeName(std::move(messageTypeName))
    {}
};

struct Response
{
    std::string envId;
    std::string topic;
    std::string messageTypeName;
    std::string messageJson;
    std::vector<std::pair<const char*,size_t>> buffers;
    ConfirmationDescriptor confirmation;

    Response()
    {}

    Response(
        std::string envId,
        std::string topic,
        std::string messageTypeName
        ) : envId(std::move(envId)),
        topic(std::move(topic)),
        messageTypeName(std::move(messageTypeName))
    {}
};

struct EventKey
{
    std::string category;
    std::string event;
    std::string envId;
    std::string topic;
    std::string oid;
};

struct Event : public EventKey
{
    std::string messageTypeName;
    std::string messageJson;
    std::string genericParameter;
    std::vector<std::pair<const char*,size_t>> buffers;
};

struct Error
{
    int code;
    std::string codeString;
    std::string message;

    void reset()
    {
        code=0;
        codeString.clear();
        message.clear();
    }
};

using Callback=std::function<void (Error, Response response)>;

using EventHandler=std::function<void (const Event& event)>;

class HATN_CLIENTAPP_EXPORT LockingBridge
{
    public:

        LockingBridge(HATN_CLIENTAPP_NAMESPACE::ClientApp* app);

        void lock();

        void unlock();

        void updateLastActivity();

        void setBackground();
        void setForeground();

        bool isLocked() const;

        bool isBackground() const;

        int autoLockPeriod() const;

        int autoLockMode() const;

        int passphraseThrottlePeriod() const;

        int passphraseThrottleDelay() const;

    private:

        HATN_CLIENTAPP_NAMESPACE::ClientApp* app;
};

class HATN_CLIENTAPP_EXPORT MobileApp
{
    public:

        MobileApp(std::shared_ptr<HATN_CLIENTAPP_NAMESPACE::ClientApp> app);
        ~MobileApp();

        MobileApp(const MobileApp&)=delete;
        MobileApp(MobileApp&&)=delete;
        MobileApp& operator=(const MobileApp&)=delete;
        MobileApp& operator=(MobileApp&&)=delete;

        int init(MobilePlatformContext* platformCtx, std::string configFile, std::string dataDir);
        int close();

        void exec(
            const std::string& service,
            const std::string& method,
            Request request,
            Callback callback
        );

        size_t subscribeEvent(
            EventHandler handler,
            EventKey key={}
        );

        void unsubscribeEvent(
            size_t id
        );

        std::vector<std::string> listLogFiles() const;

        int getAppSetting(
            const std::string key,
            std::string& jsonValue,
            Error& error
        );

        int getAppConfig(
            const std::string key,
            std::string& jsonValue,
            Error& error
        );

        const LockingBridge* locking() const;
        LockingBridge* locking();

        static std::string generateOid();
        static std::string dateTimeToOid(const std::string& datetime);
        static std::string dateTimeToOid(uint64_t epochMs);
        static std::string oidToDateTime(const std::string& oid);
        static uint64_t oidToEpochMs(const std::string& oid);

        HATN_CLIENTAPP_NAMESPACE::ClientApp* app() const;

    private:

        std::unique_ptr<MobileApp_p> pimpl;
};

HATN_CLIENTAPP_MOBILE_NAMESPACE_END

#endif // HATNMOBILEAPP_H
