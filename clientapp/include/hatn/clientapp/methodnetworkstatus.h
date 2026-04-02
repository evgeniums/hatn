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

HDU_UNIT(network_status,
    HDU_FIELD(connected,TYPE_BOOL,1)
    HDU_FIELD(event,TYPE_STRING,2)
    HDU_FIELD(medium,TYPE_STRING,3)
    HDU_FIELD(online_type,TYPE_STRING,4)
)

class HATN_CLIENTAPP_EXPORT MethodNetworkStatus : public BridgeMethod<SystemService>
{
    public:

        constexpr static const char* EventCategory="network_state";
        constexpr static const char* EventDisconnect="disconnect";
        constexpr static const char* EventConnect="connect";
        constexpr static const char* EventSwitch="switch";

        constexpr static const char* MediumEthernet="ethernet";
        constexpr static const char* MediumWifi="wifi";
        constexpr static const char* MediumBluetooth="bluetooth";
        constexpr static const char* MediumCellular="cellular";
        constexpr static const char* MediumUnknown="unknown";

        constexpr static const char* OnlineLocal="local";
        constexpr static const char* OnlineGlobal="global";
        constexpr static const char* OnlineUnknown="unknown";

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
