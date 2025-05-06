/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file whitemclient/testservicedb.h
  */

/****************************************************************************/

#ifndef WHITEMCLIENTTESSERVICEDB_H
#define WHITEMCLIENTTESSERVICEDB_H

#include <hatn/common/taskcontext.h>

#include <hatn/logcontext/context.h>

#include <hatn/app/appdefs.h>

#include <hatn/api/client/clientbridge.h>

HATN_APP_NAMESPACE_BEGIN
class App;
HATN_APP_NAMESPACE_END

HATN_API_CLIENT_BRIDGE_NAMESPACE_BEGIN

class WithBaseApp
{
    public:

        WithBaseApp(HATN_APP_NAMESPACE::App* app=nullptr) : m_app(app)
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

using BaseAppContext=HATN_COMMON_NAMESPACE::TaskContextType<WithBaseApp,HATN_LOGCONTEXT_NAMESPACE::TaskLogContext>;

class BaseAppContextBuilder : public HATN_API_CLIENT_BRIDGE_NAMESPACE::ContextBuilder,
                              public WithBaseApp
{
    public:

        using WithBaseApp::WithBaseApp;

        virtual HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_CLIENT_BRIDGE_NAMESPACE::Context> makeContext(HATN_COMMON_NAMESPACE::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env) override
        {
            auto ctx=HATN_COMMON_NAMESPACE::makeTaskContextType<BaseAppContext>(
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

class TestMethodOpenDb : public HATN_API_CLIENT_BRIDGE_NAMESPACE::Method
{
    public:

        TestMethodOpenDb() : Method("open_db")
        {}

        virtual void exec(
            HATN_COMMON_NAMESPACE::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
            HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_CLIENT_BRIDGE_NAMESPACE::Context> ctx,
            HATN_API_CLIENT_BRIDGE_NAMESPACE::Request request,
            HATN_API_CLIENT_BRIDGE_NAMESPACE::Callback callback
        ) override;
};

class TestMethodDestroyDb : public HATN_API_CLIENT_BRIDGE_NAMESPACE::Method
{
    public:

        TestMethodDestroyDb() : Method("destroy_db")
        {}

        virtual void exec(
            HATN_COMMON_NAMESPACE::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
            HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_CLIENT_BRIDGE_NAMESPACE::Context> ctx,
            HATN_API_CLIENT_BRIDGE_NAMESPACE::Request request,
            HATN_API_CLIENT_BRIDGE_NAMESPACE::Callback callback
        ) override;
};

class TestServiceDb : public HATN_API_CLIENT_BRIDGE_NAMESPACE::Service
{
    public:

        TestServiceDb(HATN_APP_NAMESPACE::App* app);
};

HATN_API_CLIENT_BRIDGE_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE_EXPORT(HATN_API_CLIENT_BRIDGE_NAMESPACE::WithBaseApp,HATN_API_EXPORT)

#endif // WHITEMCLIENTTESSERVICEDB_H
