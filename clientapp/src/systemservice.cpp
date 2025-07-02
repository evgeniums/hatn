/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/systemservice.сpp
  *
  */

#include <hatn/clientapp/methodnetworkstatus.h>
#include <hatn/clientapp/methodsyncdatetime.h>
#include <hatn/clientapp/systemservice.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------

SystemService::SystemService(ClientApp* app) : Service(app,Name)
{
    registerMethod(std::make_shared<MethodNetworkStatus>(this));
    registerMethod(std::make_shared<MethodSyncDateTime>(this));
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END

HATN_TASK_CONTEXT_DEFINE(HATN_CLIENTAPP_NAMESPACE::WithClientApp,WithClientApp)
