/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file whitemclient/clientapp.сpp
  *
  */

#include <hatn/app/app.h>
#include <hatn/api/client/clientbridge.h>
#include <hatn/api/client/clientapp.h>

HATN_API_CLIENT_BRIDGE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

class ClientApp_p
{
    public:

        ClientApp_p(HATN_APP_NAMESPACE::AppName appName) : app(std::move(appName))
        {}

        HATN_APP_NAMESPACE::App app;
        Dispatcher bridge;
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

Dispatcher& ClientApp::bridge()
{
    return pimpl->bridge;
}

//--------------------------------------------------------------------------

HATN_API_CLIENT_BRIDGE_NAMESPACE_END
