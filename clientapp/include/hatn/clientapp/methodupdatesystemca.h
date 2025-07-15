/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodupdatesystemca.h
  *
  */

/****************************************************************************/

#ifndef HATNMETHODUPDATESYSTEMCA_H
#define HATNMETHODUPDATESYSTEMCA_H

#include <hatn/clientapp/bridgemethod.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class SystemService;

class HATN_CLIENTAPP_EXPORT MethodUpdateSystemCa : public BridgeMethod<SystemService>
{
    public:

        constexpr static const char* Name="update_system_ca";

        MethodUpdateSystemCa(SystemService* service) : BridgeMethod<SystemService>(service,Name)
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

#endif // HATNMETHODUPDATESYSTEMCA_H
