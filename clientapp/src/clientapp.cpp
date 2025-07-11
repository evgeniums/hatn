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

#include <hatn/app/app.h>
#include <hatn/clientapp/clientbridge.h>
#include <hatn/clientapp/eventdispatcher.h>
#include <hatn/clientapp/systemservice.h>
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
};

//--------------------------------------------------------------------------

ClientApp::ClientApp(HATN_APP_NAMESPACE::AppName appName) : pimpl(std::make_unique<ClientApp_p>(std::move(appName)))
{
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

HATN_CLIENTAPP_NAMESPACE_END
