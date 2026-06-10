/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodsetloggerconfig.cpp
  *
  */

#include <hatn/base/configtreepath.h>
#include <hatn/base/configtreejson.h>

#include <hatn/logcontext/loggerconfig.h>
#include <hatn/logcontext/withlogger.h>

#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/clientappfilesettings.h>
#include <hatn/clientapp/systemservice.h>
#include <hatn/clientapp/methodsetloggerconfig.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------
// Shared helper: parse a logger-config JSON string and apply it to the live logger.
// Called by MethodSetLoggerConfig::exec() and by the ClientApp startup hook.
//---------------------------------------------------------------

Error applyLoggerConfigJson(
        HATN_LOGCONTEXT_NAMESPACE::LoggerBase* logger,
        const std::string& json
    )
{
    HATN_BASE_NAMESPACE::ConfigTree tree;
    HATN_BASE_NAMESPACE::ConfigTreeJson parser;
    auto ec = parser.parse(tree, json);
    if (ec)
    {
        return ec;
    }

    HATN_BASE_NAMESPACE::config_object::LogRecords records;
    // The JSON encodes the whole logger_config subtree at root level,
    // so pass an empty path ("") as the config path.
    return logger->loadLogConfig(tree, "", records);
}

//---------------------------------------------------------------

void MethodSetLoggerConfig::exec(
        common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
        common::SharedPtr<Context> ctx,
        Request request,
        Callback callback
    )
{
    HATN_CTX_SCOPE("setloggerconfig::exec")

    auto* ca = clientApp(env, ctx);
    if (!ca)
    {
        callback(Error{}, Response{});
        return;
    }

    // Serialize the incoming logger_config DU to JSON.
    auto msg = request.message.as<HATN_LOGCONTEXT_NAMESPACE::logger_config::managed>();
    std::string json;
    if (!msg->toJSON(json))
    {
        callback(clientAppError(ClientAppError::FAILED_PARSE_BRIDGE_JSON), Response{});
        return;
    }

    // Persist BEFORE applying to the live logger (defensive ordering: no I/O can
    // pump the event loop between persist and apply here, but maintain the pattern).
    auto ec = ca->fileSettings().setJson(
        HATN_BASE_NAMESPACE::ConfigTreePath{HATN_LOGCONTEXT_NAMESPACE::LoggerConfigSettingsPath},
        json
    );
    if (ec)
    {
        callback(ec, Response{});
        return;
    }

    // Apply to the live logger.
    auto* loggerWrapper = &ca->app().logger();
    if (loggerWrapper->logger())
    {
        ec = applyLoggerConfigJson(loggerWrapper->logger(), json);
        if (ec)
        {
            HATN_CTX_ERROR(ec, "failed to apply logger config to live logger")
            // Non-fatal: the config is already persisted and will be loaded on next start.
        }
    }

    callback(Error{}, Response{});
}

//---------------------------------------------------------------

std::string MethodSetLoggerConfig::messageType() const
{
    return HATN_LOGCONTEXT_NAMESPACE::logger_config::conf().name;
}

//---------------------------------------------------------------

MessageBuilderFn MethodSetLoggerConfig::messageBuilder() const
{
    return messageBuilderT<HATN_LOGCONTEXT_NAMESPACE::logger_config::managed>();
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
