/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodgetloggerconfig.cpp
  *
  */

#include <hatn/base/configtreepath.h>

#include <hatn/logcontext/loggerconfig.h>

#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/clientappfilesettings.h>
#include <hatn/clientapp/systemservice.h>
#include <hatn/clientapp/methodgetloggerconfig.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------

void MethodGetLoggerConfig::exec(
        common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
        common::SharedPtr<Context> ctx,
        Request /*request*/,
        Callback callback
    )
{
    HATN_CTX_SCOPE("getloggerconfig::exec")

    auto* ca = clientApp(env, ctx);
    if (!ca)
    {
        callback(Error{}, Response{});
        return;
    }

    Error ec;
    auto json = ca->fileSettings().getJson(
        HATN_BASE_NAMESPACE::ConfigTreePath{HATN_LOGCONTEXT_NAMESPACE::LoggerConfigSettingsPath},
        ec
    );

    if (ec)
    {
        // Setting not found is not an error — return empty response so caller uses defaults.
        callback(Error{}, Response{});
        return;
    }

    // Return the persisted JSON as the response message using the logger_config unit.
    auto msg = common::makeShared<HATN_LOGCONTEXT_NAMESPACE::logger_config::managed>();
    msg->loadFromJSON(json, ec);
    if (ec)
    {
        callback(ec, Response{});
        return;
    }

    Response response;
    response.setMessage(std::move(msg));
    callback(Error{}, std::move(response));
}

//---------------------------------------------------------------

std::string MethodGetLoggerConfig::messageType() const
{
    return HATN_LOGCONTEXT_NAMESPACE::logger_config::conf().name;
}

//---------------------------------------------------------------

MessageBuilderFn MethodGetLoggerConfig::messageBuilder() const
{
    return messageBuilderT<HATN_LOGCONTEXT_NAMESPACE::logger_config::managed>();
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
