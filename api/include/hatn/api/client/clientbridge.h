/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/clientbridge.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTBRIDGE_H
#define HATNAPICLIENTBRIDGE_H

#include <functional>

#include <hatn/common/flatmap.h>
#include <hatn/common/taskcontext.h>

#include <hatn/logcontext/context.h>

#include <hatn/dataunit/unit.h>

#include <hatn/app/appenv.h>

#include <hatn/api/api.h>
#include <hatn/api/apiliberror.h>

HATN_API_CLIENT_BRIDGE_NAMESPACE_BEGIN

struct Request
{
    std::string envId;
    std::string topic;
    std::string messageTypeName;
    common::SharedPtr<du::Unit> message;
    std::vector<common::ByteArrayShared> dataArrays;

    Request()
    {}

    Request(
            std::string envId,
            std::string topic,
            std::string messageTypeName
        ) : envId(std::move(envId)),
            topic(std::move(topic)),
            messageTypeName(std::move(messageTypeName))
    {}
};

using Response=Request;

using Context=common::TaskContext;
using Callback=std::function<void (const Error& ec, Response response)>;

class HATN_API_EXPORT ContextBuilder
{
    public:

        ContextBuilder()=default;
        virtual ~ContextBuilder();

        ContextBuilder(const ContextBuilder&)=delete;
        ContextBuilder(ContextBuilder&&)=default;
        ContextBuilder& operator=(const ContextBuilder&)=delete;
        ContextBuilder& operator=(ContextBuilder&&)=default;

        virtual common::SharedPtr<Context> makeContext(common::SharedPtr<app::AppEnv> env)=0;
};

class DefaultContextBuilder : public ContextBuilder
{
    public:

        virtual common::SharedPtr<Context> makeContext(common::SharedPtr<app::AppEnv> env) override
        {
            return HATN_APP_NAMESPACE::allocateAppEnvContext(std::move(env));
        }
};

class HATN_API_EXPORT Method
{
    public:

        Method(std::string name) : m_name(std::move(name))
        {}

        virtual ~Method();

        Method(const Method&)=delete;
        Method(Method&&)=default;
        Method& operator=(const Method&)=delete;
        Method& operator=(Method&&)=default;

        virtual void exec(
            common::SharedPtr<app::AppEnv> env,
            common::SharedPtr<Context> ctx,
            Request request,
            Callback callback
        ) =0;

        const std::string& name() const noexcept
        {
            return m_name;
        }

    private:

        std::string m_name;
};

class HATN_API_EXPORT Service
{
    public:

        Service(std::string name) : m_name(std::move(name))
        {}

        void exec(
            common::SharedPtr<app::AppEnv> env,
            const std::string& method,
            Request request,
            Callback callback
        );

        void setContextBuilder(std::shared_ptr<ContextBuilder> builder={})
        {
            m_ctxBuilder=std::move(builder);
        }

        const auto& contextBuilder() const noexcept
        {
            return m_ctxBuilder;
        }

        void registerMethod(
            std::shared_ptr<Method> method
        )
        {
            m_methods[method->name()]=method;
        }

        const std::string& name() const noexcept
        {
            return m_name;
        }

    private:

        std::string m_name;
        std::shared_ptr<ContextBuilder>  m_ctxBuilder;
        common::FlatMap<std::string,std::shared_ptr<Method>> m_methods;
};

using MessageBuilder=std::function<Result<common::SharedPtr<du::Unit>> (const std::string& messageJson)>;

class HATN_API_EXPORT Dispatcher
{
    public:

        Dispatcher() : m_defaultCtxBuilder(std::make_shared<DefaultContextBuilder>())
        {}

        void exec(
            const std::string& service,
            const std::string& method,
            Request request,
            Callback callback
        );

        void setDefaultContextBuilder(std::shared_ptr<ContextBuilder> builder={})
        {
            m_defaultCtxBuilder=std::move(builder);
        }

        const auto& defaultContextBuilder() const noexcept
        {
            return m_defaultCtxBuilder;
        }

        void registerService(
            std::shared_ptr<Service> service
        );

        void setDefaultEnv(common::SharedPtr<app::AppEnv> defaultEnv={})
        {
            m_defaultEnv=std::move(defaultEnv);
        }

        const auto& defaultEnv() const noexcept
        {
            return m_defaultEnv;
        }

        void addEnv(common::SharedPtr<app::AppEnv> env)
        {
            m_envs[env->name()]=env;
        }

        common::SharedPtr<app::AppEnv> env(const std::string& envId) const
        {
            auto it=m_envs.find(envId);
            if (it==m_envs.end())
            {
                return m_defaultEnv;
            }
            return it->second;
        }

        Result<common::SharedPtr<du::Unit>> makeMessage(const std::string& messageType, const std::string& messageJson) const
        {
            auto it=m_messageBuilders.find(messageType);
            if (it==m_messageBuilders.end())
            {
                return apiLibError(ApiLibError::UNKNOWN_BRIDGE_MESSASGE);
            }
            return it->second(messageJson);
        }

        void registerMessageBuilder(
            std::string messageType,
            MessageBuilder builder
        )
        {
            m_messageBuilders[std::move(messageType)]=std::move(builder);
        }

    private:

        common::FlatMap<std::string,std::shared_ptr<Service>> m_services;
        common::FlatMap<std::string,common::SharedPtr<app::AppEnv>> m_envs;

        std::map<std::string,MessageBuilder> m_messageBuilders;

        std::shared_ptr<ContextBuilder>  m_defaultCtxBuilder;
        common::SharedPtr<app::AppEnv> m_defaultEnv;
};

HATN_API_CLIENT_BRIDGE_NAMESPACE_END

#endif // HATNAPICLIENTBRIDGE_H
