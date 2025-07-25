/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodnetworkstatus.h
  *
  */

/****************************************************************************/

#ifndef HATNMETHODNETWORKSTATUS_H
#define HATNMETHODNETWORKSTATUS_H

#include <hatn/clientapp/bridgemethod.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class SystemService;

class HATN_CLIENTAPP_EXPORT MethodNetworkStatus : public BridgeMethod<SystemService>
{
    public:

        constexpr static const char* Name="network_status";

        MethodNetworkStatus(SystemService* service) : BridgeMethod<SystemService>(service,Name)
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

#endif // HATNMETHODNETWORKSTATUS_H
