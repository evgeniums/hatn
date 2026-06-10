/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodgetloggerconfig.h
  *
  * SystemService method: read the persisted custom logger config from file settings.
  * Returns the logger_config JSON (or empty if not set — caller falls back to defaults).
  */

/****************************************************************************/

#ifndef HATNMETHODGETLOGGERCONFIG_H
#define HATNMETHODGETLOGGERCONFIG_H

#include <hatn/clientapp/bridgemethod.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class SystemService;

class HATN_CLIENTAPP_EXPORT MethodGetLoggerConfig : public BridgeMethod<SystemService>
{
    public:

        constexpr static const char* Name = "get_logger_config";

        MethodGetLoggerConfig(SystemService* service) : BridgeMethod<SystemService>(service, Name)
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

#endif // HATNMETHODGETLOGGERCONFIG_H
