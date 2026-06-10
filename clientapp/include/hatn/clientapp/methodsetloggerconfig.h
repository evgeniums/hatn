/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodsetloggerconfig.h
  *
  * SystemService method: persist a custom logger config to file settings and
  * apply it to the live logger immediately.  Universal — used by desktop UI
  * and mobile bridge alike.
  */

/****************************************************************************/

#ifndef HATNMETHODSETLOGGERCONFIG_H
#define HATNMETHODSETLOGGERCONFIG_H

#include <hatn/logcontext/logger.h>

#include <hatn/clientapp/bridgemethod.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class SystemService;

/**
 * @brief Parse a logger-config JSON string and apply it to the live logger.
 *
 * Shared between MethodSetLoggerConfig::exec() and the ClientApp startup hook in
 * ClientApp::init() (called right after file settings are loaded).
 *
 * @param logger  The live LoggerBase instance (e.g. from app().logger().logger()).
 * @param json    A full logger_config JSON string (as produced by getJson /
 *                logger_config::managed::toJSON).
 * @return OK on success, or the first error encountered.
 */
HATN_CLIENTAPP_EXPORT Error applyLoggerConfigJson(
    HATN_LOGCONTEXT_NAMESPACE::LoggerBase* logger,
    const std::string& json
);

class HATN_CLIENTAPP_EXPORT MethodSetLoggerConfig : public BridgeMethod<SystemService>
{
    public:

        constexpr static const char* Name = "set_logger_config";

        MethodSetLoggerConfig(SystemService* service) : BridgeMethod<SystemService>(service, Name)
        {}

        virtual void exec(
            common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
            common::SharedPtr<Context> ctx,
            Request request,
            Callback callback
        ) override;

        virtual std::string messageType() const override;

        virtual MessageBuilderFn messageBuilder() const override;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNMETHODSETLOGGERCONFIG_H
