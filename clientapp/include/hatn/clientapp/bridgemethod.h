/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file clientapp/bridgemethod.h
  */

/****************************************************************************/

#ifndef HATNCLIENBRIDGEMETHOD_H
#define HATNCLIENBRIDGEMETHOD_H

#include <hatn/app/app.h>
#include <hatn/app/appenv.h>

#include <hatn/clientapp/clientbridge.h>
#include <hatn/clientapp/bridgeappcontext.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class HATN_CLIENTAPP_EXPORT ContextApp
{
    public:

        static ClientApp* clientApp(
            const common::SharedPtr<HATN_APP_NAMESPACE::AppEnv>& env,
            const common::SharedPtr<Context>& ctx
        );

        static HATN_APP_NAMESPACE::App* app(
            const common::SharedPtr<HATN_APP_NAMESPACE::AppEnv>& env,
            const common::SharedPtr<Context>& ctx
        );
};

template <typename ServiceT>
class BridgeMethod : public ServiceMethod<ServiceT>,
                     public ContextApp
{
    public:

        using ServiceMethod<ServiceT>::ServiceMethod;

        auto controller() const
        {
            return this->service()->controller();
        }
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENBRIDGEMETHOD_H
