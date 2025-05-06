/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file api/client/bridgeappcontextappcontext.h
  */

/****************************************************************************/

#ifndef HATNAPPCONTEXT_H
#define HATNAPPCONTEXT_H

#include <hatn/common/taskcontext.h>

#include <hatn/logcontext/context.h>

#include <hatn/app/appenv.h>

#include <hatn/api/api.h>
#include <hatn/api/client/clientbridge.h>

HATN_APP_NAMESPACE_BEGIN
class App;
HATN_APP_NAMESPACE_END

HATN_API_CLIENT_BRIDGE_NAMESPACE_BEGIN

class WithApp
{
    public:

        WithApp(HATN_APP_NAMESPACE::App* app=nullptr) : m_app(app)
        {}

        void setApp(HATN_APP_NAMESPACE::App* app=nullptr)
        {
            m_app=app;
        }

        HATN_APP_NAMESPACE::App* app() noexcept
        {
            return m_app;
        }

    private:

        HATN_APP_NAMESPACE::App* m_app;
};

using BridgeAppContext=HATN_COMMON_NAMESPACE::TaskContextType<WithApp,HATN_LOGCONTEXT_NAMESPACE::TaskLogContext>;

class BridgeAppContextBuilder : public ContextBuilder,
                              public WithApp
{
    public:

        using WithApp::WithApp;

        virtual HATN_COMMON_NAMESPACE::SharedPtr<Context> makeContext(HATN_COMMON_NAMESPACE::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env) override
        {
            auto ctx=HATN_COMMON_NAMESPACE::makeTaskContextType<BridgeAppContext>(
                HATN_COMMON_NAMESPACE::subcontexts(
                    HATN_COMMON_NAMESPACE::subcontext(app())
                )
            );

            auto& logCtx=ctx->get<HATN_LOGCONTEXT_NAMESPACE::Context>();
            auto& logger=env->get<HATN_APP_NAMESPACE::Logger>();
            logCtx.setLogger(logger.logger());

            return ctx;
        }
};

HATN_API_CLIENT_BRIDGE_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE_EXPORT(HATN_API_CLIENT_BRIDGE_NAMESPACE::WithApp,HATN_API_EXPORT)

#endif // HATNAPPCONTEXT_H
