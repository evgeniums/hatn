/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/bativebridge.h
  *
  */

/****************************************************************************/

#ifndef HATNAPPNATIVEBRIDGE_H
#define HATNAPPNATIVEBRIDGE_H

#include <functional>

#include <hatn/common/flatmap.h>
#include <hatn/common/taskcontext.h>

#include <hatn/dataunit/unit.h>

#include <hatn/app/app.h>
#include <hatn/app/appenv.h>

HATN_APP_NAMESPACE_BEGIN

namespace bridge {

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

class HATN_APP_EXPORT ContextBuilder
{
    public:

        virtual ~ContextBuilder();

        ContextBuilder(const ContextBuilder&)=delete;
        ContextBuilder(ContextBuilder&&)=default;
        ContextBuilder& operator=(const ContextBuilder&)=delete;
        ContextBuilder& operator=(ContextBuilder&&)=default;

        virtual common::SharedPtr<Context> makeContext(common::SharedPtr<AppEnv> env)=0;
};

class HATN_APP_EXPORT Method
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
            common::SharedPtr<AppEnv> env,
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

class HATN_APP_EXPORT Service
{
    public:

        Service(std::string name) : m_name(std::move(name))
        {}

        void exec(
            common::SharedPtr<AppEnv> env,
            const std::string& method,
            Request request,
            Callback callback
        );

        void resetContextBuilder(std::shared_ptr<ContextBuilder> builder={})
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

class HATN_APP_EXPORT Dispatcher
{
    public:

        void exec(
            const std::string& service,
            const std::string& method,
            Request request,
            Callback callback
        );

        void resetDefaultContextBuilder(std::shared_ptr<ContextBuilder> builder={})
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

        void resetDefaultEnv(common::SharedPtr<AppEnv> defaultEnv={})
        {
            m_defaultEnv=std::move(defaultEnv);
        }

        const auto& defaultEnv() const noexcept
        {
            return m_defaultEnv;
        }

        void addEnv(common::SharedPtr<AppEnv> env)
        {
            m_envs[env->name()]=env;
        }

        common::SharedPtr<AppEnv> env(const std::string& envId) const
        {
            auto it=m_envs.find(envId);
            if (it==m_envs.end())
            {
                return m_defaultEnv;
            }
            return it->second;
        }

    private:

        common::FlatMap<std::string,std::shared_ptr<Service>> m_services;
        common::FlatMap<std::string,common::SharedPtr<AppEnv>> m_envs;

        std::shared_ptr<ContextBuilder>  m_defaultCtxBuilder;
        common::SharedPtr<AppEnv> m_defaultEnv;
};

} // namespace bridge

HATN_APP_NAMESPACE_END

#endif // HATNAPPNATIVEBRIDGE_H
