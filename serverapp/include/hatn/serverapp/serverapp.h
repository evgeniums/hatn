/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/serverapp.h
  */

/****************************************************************************/

#ifndef HATNSERVERAPP_H
#define HATNSERVERAPP_H

#include <memory>

#include <hatn/app/app.h>

#include <hatn/api/server/microservicefactory.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_APP_NAMESPACE_BEGIN
class BaseApp;
struct AppName;
HATN_APP_NAMESPACE_END

HATN_SERVERAPP_NAMESPACE_BEGIN

class ServerApp_p;

class HATN_SERVERAPP_EXPORT ServerApp
{
    public:

        ServerApp(const HATN_APP_NAMESPACE::AppName& appName);
        ~ServerApp();

        ServerApp(const ServerApp&)=delete;
        ServerApp(ServerApp&&)=default;
        ServerApp& operator =(const ServerApp&)=delete;
        ServerApp& operator =(ServerApp&&)=default;

        HATN_APP_NAMESPACE::BaseApp& app();

        int init(
            int argc,
            char *argv[]
        );

        void exec();
        void close();
        void stop();

        void setMicroServiceFactory(
            std::shared_ptr<HATN_API_NAMESPACE::server::MicroServiceFactory> factory
        );

    private:

        std::shared_ptr<ServerApp_p> pimpl;
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERAPP_H
