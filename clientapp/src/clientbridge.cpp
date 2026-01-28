/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientbridge.сpp
  *
  */

#include <hatn/clientapp/clientapperror.h>
#include <hatn/clientapp/bridgeappcontext.h>
#include <hatn/clientapp/clientbridge.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------

ContextBuilder::~ContextBuilder()
{}

//---------------------------------------------------------------

Method::~Method()
{}

//---------------------------------------------------------------

ConfirmationController::~ConfirmationController()
{}

//---------------------------------------------------------------

Service::Service(ClientApp* app, std::string name) : Service(std::move(name))
{
    setContextBuilder(std::make_shared<BridgeAppContextBuilder>(app));
}

//---------------------------------------------------------------

void Service::exec(
        common::SharedPtr<app::AppEnv> env,
        const std::string& method,
        Request request,
        Callback callback
    )
{
    auto thread=env->get<app::Threads>().threads()->defaultThread();
    auto it=m_methods.find(method);
    if (it==m_methods.end())
    {
        std::cerr << "failed to exec service method" << std::endl;

        thread->execAsync(
            [callback=std::move(callback),this,method]()
            {
                HATN_CTX_ENTER_SCOPE("service::exec")
                HATN_CTX_PUSH_FIXED_VAR("bridge_srv",name())
                HATN_CTX_PUSH_FIXED_VAR("bridge_mthd",method)

                auto ec=clientAppError(ClientAppError::UNKNOWN_BRIDGE_METHOD);
                HATN_CTX_ERROR(ec,"failed to exec service method")

                HATN_CTX_LEAVE_SCOPE()
                callback(ec,Response{});
            }
        );
        return;
    }

    auto mthd=it->second;

    auto msgType=mthd->messageType();
    if (!msgType.empty() && msgType!=request.messageTypeName)
    {
        thread->execAsync(
            [callback=std::move(callback),this,method]()
            {
                HATN_CTX_ENTER_SCOPE("service::exec")
                HATN_CTX_PUSH_FIXED_VAR("bridge_srv",name())
                HATN_CTX_PUSH_FIXED_VAR("bridge_mthd",method)

                auto ec=clientAppError(ClientAppError::BRIDGE_MESSASGE_TYPE_MISMATCH);
                HATN_CTX_ERROR(ec,"failed to exec service method")

                HATN_CTX_LEAVE_SCOPE()
                callback(ec,Response{});
            }
        );
        return;
    }

    auto ctx=m_ctxBuilder->makeContext(env);
    auto handler=[this,ctx{std::move(ctx)},mthd{std::move(mthd)},env,request{std::move(request)},callback{std::move(callback)}]()
    {
        ctx->onAsyncHandlerEnter();

        {
            HATN_CTX_SCOPE_WITH_BARRIER("service::exec")

            HATN_CTX_SCOPE_PUSH("bridge_srv",name())
            HATN_CTX_SCOPE_PUSH("bridge_mthd",mthd->name())
            HATN_CTX_SCOPE_PUSH("bridge_env",request.envId)
            HATN_CTX_SCOPE_PUSH("bridge_topic",request.topic)
            HATN_CTX_SCOPE_PUSH("bridge_msg_type",request.messageTypeName)

            auto cb=[callback=std::move(callback)](const Error& ec, Response response)
            {
                if (ec)
                {
                    HATN_CTX_DEBUG_RECORDS(1,"BRIDGE-EXEC",{"status","failed"},{"errc",ec.codeString()},{"error",ec.message()})
                }
                else
                {
                    HATN_CTX_DEBUG_RECORDS(1,"BRIDGE-EXEC",{"status","success"})
                }

                HATN_CTX_STACK_BARRIER_OFF("service::exec")

                callback(ec,std::move(response));
            };

            mthd->exec(env,std::move(ctx),std::move(request),std::move(cb));
        }

        ctx->onAsyncHandlerExit();
    };
    thread->execAsync(handler);
}

//---------------------------------------------------------------

void Dispatcher::registerService(std::shared_ptr<Service> service)
{
    Assert(m_services.find(service->name())==m_services.end(),"Duplicate service");
    if (!service->contextBuilder())
    {
        service->setContextBuilder(m_defaultCtxBuilder);
    }
    m_services[service->name()]=service;
}

//---------------------------------------------------------------

void Dispatcher::exec(
        const std::string& service,
        const std::string& method,
        Request request,
        Callback callback
    )
{
    Assert(m_defaultEnv,"Default env must be set in dispatcher of the app bridge");
    auto execService=[this](
                           const std::string& service,
                           const std::string& method,
                           Request request,
                           Callback callback
                    )
    {
        auto it=m_services.find(service);
        if (it==m_services.end())
        {
            auto thread=m_defaultEnv->get<app::Threads>().threads()->defaultThread();
            thread->execAsync(
                [callback{std::move(callback)},service,method]()
                {
                    HATN_CTX_SCOPE("dispatcher:exec:cb")
                    HATN_CTX_SCOPE_PUSH("bridge_srv",service)
                    HATN_CTX_SCOPE_PUSH("bridge_mthd",method)

                    auto ec=clientAppError(ClientAppError::UNKNOWN_BRIDGE_SERVICE);
                    HATN_CTX_ERROR(ec,"failed to exec dispatcher service")

                    callback(ec,Response{});
                }
            );
            return;
        }

        auto envr=env(request.envId);
        it->second->exec(std::move(envr),method,std::move(request),std::move(callback));
    };

    auto confirm=confirmation(service,method);
    if (confirm!=nullptr && !request.confirmation.skipConfirmation)
    {
        auto cb=[service,method,callback,execService=std::move(execService)](const Error& ec, Response response, Request request)
        {
            if (ec)
            {
                callback(ec,std::move(response));
                return;
            }

            execService(service,method,std::move(request),std::move(callback));
        };
        confirm->checkConfirmation(service,method,std::move(request),std::move(cb));
    }
    else
    {
        execService(service,method,std::move(request),std::move(callback));
    }
}

//---------------------------------------------------------------

void Dispatcher::registerConfirmation(
        const std::string& service,
        const std::string& method,
        std::shared_ptr<ConfirmationController> confirmation
    )
{
    std::string key=method+service;
    m_confirmations.emplace(std::move(key),std::move(confirmation));
}

//---------------------------------------------------------------

ConfirmationController* Dispatcher::confirmation(
        const std::string& service,
        const std::string& method
    ) const
{
    std::string key=method+service;
    auto it=m_confirmations.find(key);
    if (it!=m_confirmations.end())
    {
        return it->second.get();
    }
    return nullptr;
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END

HATN_TASK_CONTEXT_DEFINE(HATN_CLIENTAPP_NAMESPACE::WithClientApp,WithClientApp)
