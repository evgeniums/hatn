/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientbridge.h
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTBRIDGE_H
#define HATNCLIENTBRIDGE_H

#include <functional>

#include <hatn/common/flatmap.h>
#include <hatn/common/taskcontext.h>

#include <hatn/logcontext/context.h>

#include <hatn/dataunit/unit.h>

#include <hatn/app/appenv.h>

#include <hatn/clientapp/clientapperror.h>
#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/confirmationdescriptor.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

struct Request
{
    std::string envId;
    std::string topic;
    std::string messageTypeName;
    du::UnitWrapper message;
    std::vector<common::ByteArrayShared> buffers;

    ConfirmationDescriptor confirmation;

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

    template <typename T>
    void setMessage(common::SharedPtr<T> msg, lib::string_view messageType={})
    {
        if (messageType.empty())
        {
            messageType=msg->unitName();
        }

        messageTypeName=messageType;
        message=std::move(msg);
    }
};

using Response=Request;

using Callback=std::function<void (const Error& ec, Response response)>;

class HATN_CLIENTAPP_EXPORT ContextBuilder
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
            auto ctx=HATN_LOGCONTEXT_NAMESPACE::makeLogCtx();
            auto& logCtx=ctx->get<HATN_LOGCONTEXT_NAMESPACE::Context>();
            auto& logger=env->get<app::Logger>();
            logCtx.setLogger(logger.logger());
            return ctx;
        }
};

using MessageBuilderFn=std::function<Result<du::UnitWrapper> (const std::string& messageJson)>;

class HATN_CLIENTAPP_EXPORT Method
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

        virtual std::string messageType() const
        {
            return std::string{};
        }

        virtual MessageBuilderFn messageBuilder() const
        {
            return MessageBuilderFn{};
        }

        template <typename T>
        auto messageTypeT() const
        {
            return T::name;
        }

        template <typename T>
        auto messageBuilderT() const
        {
            return [](const std::string& messageJson) -> Result<du::UnitWrapper>
            {
                auto msg=common::makeShared<T>();

                Error ec;
                msg->loadFromJSON(messageJson,ec);
                HATN_CHECK_EC(ec)

                return msg;
            };
        }

    private:

        std::string m_name;
};

template <typename ServiceT>
class ServiceMethod : public Method
{
    public:

        ServiceMethod(ServiceT *service, std::string name)
            : Method(std::move(name)),
              m_service(service)
        {}

        ServiceT* service() const noexcept
        {
            return m_service;
        }

    private:

        ServiceT* m_service;
};


template <typename MessageT>
struct MessageBuilder
{
    common::Result<HATN_DATAUNIT_NAMESPACE::UnitWrapper> operator() (const std::string& messageJson) const
    {
        auto obj=common::makeShared<MessageT>();
        auto ok=obj->loadFromJSON(messageJson);
        if (!ok)
        {
            return clientAppError(ClientAppError::FAILED_PARSE_BRIDGE_JSON);
        }
        return obj;
    }
};

class HATN_CLIENTAPP_EXPORT Service
{
    public:

        Service(std::string name) : m_name(std::move(name))
        {}

        Service(ClientApp* app, std::string name);

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
            auto messageType=method->messageType();
            auto messageBuilder=method->messageBuilder();
            if (messageBuilder && !messageType.empty())
            {
                registerMessageBuilder(std::move(messageType),std::move(messageBuilder));
            }
        }

        const std::string& name() const noexcept
        {
            return m_name;
        }

        void registerMessageBuilder(
                std::string messageType,
                MessageBuilderFn builder
            )
        {
            if (m_messageBuilders.find(messageType)==m_messageBuilders.end())
            {
                m_messageBuilders[std::move(messageType)]=std::move(builder);
            }
        }

        template <typename MessageT>
        void registerMessageBuilder(
                std::string messageType,
                MessageBuilder<MessageT> builder
            )
        {
            registerMessageBuilder(std::move(messageType),builder);
        }

        Result<du::UnitWrapper> makeMessage(const std::string& messageType, const std::string& messageJson) const
        {
            auto it=m_messageBuilders.find(messageType);
            if (it==m_messageBuilders.end())
            {
                return clientAppError(ClientAppError::UNKNOWN_BRIDGE_MESSAGE);
            }
            return it->second(messageJson);
        }

    private:

        std::string m_name;
        std::shared_ptr<ContextBuilder>  m_ctxBuilder;
        common::FlatMap<std::string,std::shared_ptr<Method>> m_methods;

        std::map<std::string,MessageBuilderFn> m_messageBuilders;
};

template <typename ServiceName, typename ControllerT, typename ClientAppT>
class ServiceT : public Service
{
    public:

        using Controller=ControllerT;
        using ClientApp=ClientAppT;

        ServiceT(std::shared_ptr<Controller> ctrl)
            : Service(ServiceName::Name),
              m_ctrl(std::move(ctrl))
        {}

        ServiceT(ClientApp* app) : ServiceT(std::make_shared<Controller>(app))
        {}

        Controller* controller() const
        {
            return m_ctrl.get();
        }

        std::shared_ptr<Controller> controllerShared() const
        {
            return m_ctrl;
        }

    private:

        std::shared_ptr<Controller> m_ctrl;
};

class HATN_CLIENTAPP_EXPORT ConfirmationController
{
    public:

        using Callback=std::function<void (const Error& ec, Response response, Request request)>;

        ConfirmationController(std::string typeName) : m_typeName(std::move(typeName))
        {}

        virtual ~ConfirmationController();

        ConfirmationController(const ConfirmationController&)=default;
        ConfirmationController(ConfirmationController&&)=default;
        ConfirmationController& operator=(const ConfirmationController&)=default;
        ConfirmationController& operator=(ConfirmationController&&)=default;

        virtual void checkConfirmation(
            const std::string& service,
            const std::string& method,
            Request request,
            Callback callback
        ) =0;

        const std::string& type() const
        {
            return m_typeName;
        }

    private:

        std::string m_typeName;
};

class HATN_CLIENTAPP_EXPORT Dispatcher
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
            common::MutexScopedLock l{m_mutex};
            m_envs[env->name()]=env;
        }

        void removeEnv(const std::string& name)
        {
            common::MutexScopedLock l{m_mutex};
            m_envs.erase(name);
        }

        common::SharedPtr<app::AppEnv> env(const std::string& envId) const
        {
            common::MutexScopedLock l{m_mutex};

            auto it=m_envs.find(envId);
            if (it==m_envs.end())
            {
                return m_defaultEnv;
            }
            return it->second;
        }

        common::SharedPtr<app::AppEnv> exactEnv(const std::string& envId) const
        {
            common::MutexScopedLock l{m_mutex};

            auto it=m_envs.find(envId);
            if (it!=m_envs.end())
            {
                return it->second;
            }
            return common::SharedPtr<app::AppEnv>{};
        }

        Result<du::UnitWrapper> makeMessage(const std::string& service, const std::string& messageType, const std::string& messageJson) const
        {
            auto it=m_services.find(service);
            if (it==m_services.end())
            {
                return clientAppError(ClientAppError::UNKNOWN_BRIDGE_SERVICE);
            }
            return it->second->makeMessage(messageType,messageJson);
        }

        void registerConfirmation(
            const std::string& service,
            const std::string& method,
            std::shared_ptr<ConfirmationController> confirmation
        );

        ConfirmationController* confirmation(
            const std::string& service,
            const std::string& method
        ) const;

    private:

        mutable common::MutexLock m_mutex;

        common::FlatMap<std::string,std::shared_ptr<Service>,std::less<void>> m_services;
        common::FlatMap<std::string,common::SharedPtr<app::AppEnv>,std::less<void>> m_envs;
        common::FlatMap<std::string,std::shared_ptr<ConfirmationController>,std::less<void>> m_confirmations;

        std::shared_ptr<ContextBuilder>  m_defaultCtxBuilder;
        common::SharedPtr<app::AppEnv> m_defaultEnv;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTBRIDGE_H
