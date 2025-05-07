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

#ifndef HATNMMOBILEAPP_H
#define HATNMMOBILEAPP_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include <hatn/app/appname.h>

#include <hatn/clientapp/clientappdefs.h>

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

struct Error
{
    int code;
    std::string codeString;
    std::string message;
};

using Callback=std::function<void (Error, Response response)>;

class MobileApp
{
    public:

        MobileApp(std::shared_ptr<HATN_CLIENTAPP_NAMESPACE::ClientApp> app);
        ~MobileApp();

        MobileApp(const MobileApp&)=delete;
        MobileApp(MobileApp&&)=default;
        MobileApp& operator=(const MobileApp&)=delete;
        MobileApp& operator=(MobileApp&&)=default;

        int init(MobilePlatformContext* platformCtx, std::string configFile, std::string dataDir);
        int close();

        void exec(
            const std::string& service,
            const std::string& method,
            Request request,
            Callback callback
        );

        int initTests();

        std::vector<std::string> listLogFiles() const;

    private:

        std::unique_ptr<MobileApp_p> pimpl;
};

HATN_CLIENTAPP_MOBILE_NAMESPACE_END

#endif // HATNMMOBILEAPP_H
