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

#include <hatn/common/error.h>
#include <hatn/common/apierror.h>
#include <hatn/common/stdwrappers.h>

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
    std::string subject;
    std::string messageTypeName;
    std::string messageJson;
    std::vector<std::vector<char>> buffers;
    ConfirmationDescriptor confirmation;

    uint32_t cacheOptions=0;
    uint32_t cacheDbTtl=0;

    Request()
    {}

    Request(
        std::string envId,
        std::string topic,
        std::string subject,
        std::string messageTypeName
        ) : envId(std::move(envId)),
            topic(std::move(topic)),
            subject(std::move(subject)),
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
    std::string subject;
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

    //! App-defined user-presentable classification of this error, filled by the mapper
    //! installed via setUserErrorMapper() - see fillError() below. 0 means "not classified"
    //! (no mapper installed, or the mapper left it at its default). hatn itself does not define
    //! what the values mean; whitemclient owns that vocabulary (whitemclient/usererrorcodes.h).
    int userCode=0;
    //! ApiErrorDisposition of the classified error - Unknown (0) when the server stated none.
    common::ApiErrorDisposition disposition=common::ApiErrorDisposition::Unknown;
    //! Seconds to wait before retrying; meaningful only when disposition==RetryAfter.
    int retryAfter=0;

    void reset()
    {
        code=0;
        codeString.clear();
        message.clear();
        userCode=0;
        disposition=common::ApiErrorDisposition::Unknown;
        retryAfter=0;
    }
};

//! App-installable hook that fills Error::userCode/disposition/retryAfter from an internal
//! HATN_NAMESPACE::Error, optionally scoped by service/method (mirrors exec()'s own
//! service/method parameters). hatn itself never interprets userCode - see Error::userCode
//! above. Not thread-safe to call concurrently with fillError(); intended to be installed once,
//! early, by app startup code (whitemclient's MobileApp constructor).
using UserErrorMapper=std::function<void (const HATN_NAMESPACE::Error& ec,
                                          lib::string_view service,
                                          lib::string_view method,
                                          Error& out)>;

//! Installs the process-wide mapper used by fillError(). A default no-op mapper is installed
//! initially, so hatn behaves exactly as before this hook existed until an app installs one.
HATN_CLIENTAPP_EXPORT void setUserErrorMapper(UserErrorMapper mapper);

//! Fills out's code/codeString/message from ec, then runs the installed UserErrorMapper (if any)
//! to also fill userCode/disposition/retryAfter. The single place mobileapp.cpp turns an
//! internal Error into the bridge-facing one - see its call sites for why: several of them are
//! lambdas that capture neither `this` nor a MobileApp_p, so the mapper is looked up via this
//! process-wide accessor rather than threaded through as a parameter.
HATN_CLIENTAPP_EXPORT void fillError(Error& out, const HATN_NAMESPACE::Error& ec,
                                     lib::string_view service={},
                                     lib::string_view method={});

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

        int getFileSetting(
            const std::string key,
            std::string& jsonValue,
            Error& error
        );

        int setFileSetting(
            const std::string key,
            const std::string& jsonValue,
            Error& error
        );

        int getFileSettingString(const std::string key, std::string& value, Error& error);
        int getFileSettingInt(const std::string key, int64_t& value, Error& error);
        int getFileSettingUInt(const std::string key, uint64_t& value, Error& error);
        int getFileSettingBool(const std::string key, bool& value, Error& error);
        int getFileSettingDouble(const std::string key, double& value, Error& error);

        int setFileSettingString(const std::string key, const std::string& value, Error& error);
        int setFileSettingInt(const std::string key, int64_t value, Error& error);
        int setFileSettingUInt(const std::string key, uint64_t value, Error& error);
        int setFileSettingBool(const std::string key, bool value, Error& error);
        int setFileSettingDouble(const std::string key, double value, Error& error);

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
