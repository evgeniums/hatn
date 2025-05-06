/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file app/clientapp.h
  */

/****************************************************************************/

#ifndef HATNCLIENTAPP_H
#define HATNCLIENTAPP_H

#include <memory>

#include <hatn/app/app.h>
#include <hatn/app/appname.h>

#include <hatn/api/api.h>

HATN_APP_NAMESPACE_BEGIN
class App;
HATN_APP_NAMESPACE_END

HATN_API_CLIENT_BRIDGE_NAMESPACE_BEGIN

class Dispatcher;
class ClientApp_p;

class HATN_API_EXPORT ClientApp
{
    public:

        ClientApp(HATN_APP_NAMESPACE::AppName appName);
        ~ClientApp();

        ClientApp(const ClientApp&)=delete;
        ClientApp(ClientApp&&)=default;
        ClientApp& operator =(const ClientApp&)=delete;
        ClientApp& operator =(ClientApp&&)=default;

        HATN_APP_NAMESPACE::App& app();

        HATN_API_CLIENT_BRIDGE_NAMESPACE::Dispatcher& bridge();

    private:

        std::unique_ptr<ClientApp_p> pimpl;
};

HATN_API_CLIENT_BRIDGE_NAMESPACE_END

#endif // HATNCLIENTAPP_H
