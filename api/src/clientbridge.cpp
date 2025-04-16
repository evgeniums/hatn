/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/clientbridge.сpp
  *
  */

#include <hatn/api/apiliberror.h>
#include <hatn/api/client/clientbridge.h>

HATN_API_CLIENT_BRIDGE_NAMESPACE_BEGIN

//---------------------------------------------------------------

ContextBuilder::~ContextBuilder()
{}

//---------------------------------------------------------------

Method::~Method()
{}

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
        thread->execAsync(
            [callback{std::move(callback)}]()
            {
                callback(apiLibError(ApiLibError::UNKNOWN_BRIDGE_METHOD),Response{});
            }
        );
        return;
    }

    auto mthd=it->second;
    auto ctx=m_ctxBuilder->makeContext(env);
    auto handler=[ctx{std::move(ctx)},mthd{std::move(mthd)},env,request{std::move(request)},callback{std::move(callback)}]()
    {
        ctx->onAsyncHandlerEnter();
        mthd->exec(env,std::move(ctx),std::move(request),std::move(callback));
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
    auto it=m_services.find(service);
    if (it==m_services.end())
    {
        auto thread=m_defaultEnv->get<app::Threads>().threads()->defaultThread();
        thread->execAsync(
            [callback{std::move(callback)}]()
            {
                callback(apiLibError(ApiLibError::UNKNOWN_BRIDGE_SERVICE),Response{});
            }
        );
        return;
    }

    auto envr=env(request.envId);
    it->second->exec(std::move(envr),method,std::move(request),std::move(callback));
}

//---------------------------------------------------------------

HATN_API_CLIENT_BRIDGE_NAMESPACE_END
