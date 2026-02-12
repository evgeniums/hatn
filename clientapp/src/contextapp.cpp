/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientapp/contextapp.cpp
  *
  */

#include <hatn/clientapp//bridgemethod.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE::ClientApp* ContextApp::clientApp(
    const common::SharedPtr<HATN_APP_NAMESPACE::AppEnv>&,
    const common::SharedPtr<Context>& ctx
)
{
    //! @todo optimization: use CUID and static cast
    auto baseAppCtx=ctx.dynamicCast<HATN_CLIENTAPP_NAMESPACE::BridgeAppContext>();
    Assert(baseAppCtx,"invalid context");

    auto& withApp=baseAppCtx->get<HATN_CLIENTAPP_NAMESPACE::WithClientApp>();
    return withApp.clientApp();
}

//--------------------------------------------------------------------------

HATN_APP_NAMESPACE::App* ContextApp::app(
    const common::SharedPtr<HATN_APP_NAMESPACE::AppEnv>& env,
    const common::SharedPtr<Context>& ctx
)
{
    return &clientApp(env,ctx)->app();
}

//--------------------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
