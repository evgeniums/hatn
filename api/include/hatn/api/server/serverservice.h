/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/serverservice.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERSERVICE_H
#define HATNAPISERVERSERVICE_H

#include <hatn/validator/status.hpp>

#include <hatn/api/api.h>
#include <hatn/api/service.h>
#include <hatn/api/server/serverrequest.h>

HATN_API_NAMESPACE_BEGIN

namespace server
{

enum class ServiceMethodStatus : uint8_t
{
    OK=0,
    UnknownMethod=1,
    InvalidMessageType=2,
    MessageMissing=3
};

struct NoValidatorT
{
    template <typename MessageT>
    validator::status apply(const MessageT&) const noexcept
    {
        return validator::status{};
    }
};
constexpr NoValidatorT NoValidator{};

struct NoMessage
{
    constexpr static const char* unitName()
    {
        return "";
    }
};

class ServerServiceBase
{
    public:

        //! @todo Implement handleMessage()
        template <typename RequestT, typename HandlerT, typename MessageT, typename ValidatorT=NoValidatorT>
        void handleMessage(
                common::SharedPtr<RequestContext<RequestT>> request,
                RouteCb<RequestT> callback,
                HandlerT handler,
                hana::type<MessageT> msgType=hana::type_c<NoMessage>,
                const ValidatorT& validator=NoValidator
            )const ;

        //! @todo Implement methodFailed()
        template <typename RequestT>
        void methodFailed(
            common::SharedPtr<RequestContext<RequestT>> request,
            RouteCb<RequestT> callback,
            ServiceMethodStatus status
        ) const;
};

template <typename RequestT, typename Traits>
class ServerServiceT : public ServerServiceBase,
                       public common::WithTraits<Traits>
{
    public:

        using Request=RequestT;

        template <typename ...TraitsArgs>
        ServerServiceT(
            TraitsArgs&& ...traitsArgs
        ) : common::WithTraits<Traits>::WithTraits(this,std::forward<TraitsArgs>(traitsArgs)...)
        {}

        //! @todo Implement handleRequest()
        void handleRequest(
            common::SharedPtr<RequestContext<Request>> request,
            RouteCb<RequestT> callback
        ) const;

    private:

        void exec(
                common::SharedPtr<RequestContext<Request>> request,
                lib::string_view methodName,
                bool messageExists,
                lib::string_view messageType,
                RouteCb<Request> callback
            ) const
        {
            return this->traits.exec(std::move(request),methodName,messageExists,messageType,std::move(callback));
        }
};

class ServerServiceMethodBase
{
    public:

        ServerServiceMethodBase(
                std::string name
            ) : m_name(std::move(name)),
                m_messageRequired(true),
                m_service(nullptr)
        {}

        void setName(std::string name)
        {
            m_name=std::move(name);
        }

        const std::string& name() const
        {
            return m_name;
        }

        void setMessageType(std::string messageType)
        {
            m_messageType=std::move(messageType);
        }

        const std::string& messageType() const
        {
            return m_messageType;
        }

        void setService(ServerServiceBase* service)
        {
            m_service=service;
        }

        const ServerServiceBase* service() const noexcept
        {
            return m_service;
        }

        void setMessageRequired(bool enable)
        {
            m_messageRequired=enable;
        }

        bool isMessageRequired() const noexcept
        {
            return m_messageRequired;
        }

    private:

        std::string m_name;
        std::string m_messageType;
        bool m_messageRequired;

        ServerServiceBase* m_service;
};

template <typename RequestT, typename Traits, typename MessageT=NoMessage>
class ServerServiceMethodT : public common::WithTraits<Traits>
{
    public:

        using Request=RequestT;
        using Message=MessageT;

        template <typename ...TraitsArgs>
        ServerServiceMethodT(
            ServerServiceMethodBase* base,
            TraitsArgs&& ...traitsArgs
            ) : m_base(base),
                common::WithTraits<Traits>::WithTraits(std::forward<traitsArgs>(traitsArgs)...)
        {
            m_base->setMessageType(Message::unitName());
            m_base->setMessageRequired(!std::is_same_v<MessageT,NoMessage>);
        }

        void exec(
                common::SharedPtr<RequestContext<Request>> request,
                RouteCb<Request> callback,
                lib::string_view messageType
            )
        {
            if constexpr (std::is_same_v<MessageT,NoMessage>)
            {
                // handle request withoot message
                this->service()->handleMessage(
                    std::move(request),
                    std::move(callback),
                    [this](common::SharedPtr<RequestContext<Request>> request, RouteCb<Request> callback)
                    {
                        this->traits().exec(std::move(request),std::move(callback));
                    }
                );
            }
            else
            {
                // check message type
                if (messageType!=m_base->messageType())
                {
                    this->service()->methodFailed(
                        std::move(request),
                        std::move(callback),
                        ServiceMethodStatus::InvalidMessageType
                        );

                    return;
                }

                // handle message
                this->service()->handleMessage(
                    std::move(request),
                    std::move(callback),
                    [this](common::SharedPtr<RequestContext<Request>> request, RouteCb<Request> callback,common::SharedPtr<Message> msg)
                    {
                        this->traits().exec(std::move(request),std::move(callback),std::move(msg));
                    },
                    hana::type_c<Message>,
                    [this](const Message& msg)
                    {
                        this->traits().validate(msg);
                    }
                );
            }
        }

    private:

        ServerServiceMethodBase* m_base;
};

template <typename RequestT>
class ServerServiceMethod : public ServerServiceMethodBase
{
    public:

        using Request=RequestT;

        using ServerServiceMethodBase::ServerServiceMethodBase;

        virtual ~ServerServiceMethod()=default;

        virtual void exec(
            common::SharedPtr<RequestContext<Request>> request,
            RouteCb<Request> callback,
            lib::string_view messageType
        ) =0;
};

template <typename Impl, typename RequestT>
class ServerServiceMethodV : public ServerServiceMethod<RequestT>,
                             public common::WithImpl<Impl>
{
    public:

        using ImplBase=common::WithImpl<Impl>;
        using Request=RequestT;

        template <typename ...ImplArgs>
        ServerServiceMethodV(
                std::string name,
                ImplArgs&& ...implArgs
            ) : ServerServiceMethod<RequestT>(std::move(name)),
                ImplBase(this,std::forward<ImplArgs>(implArgs)...)
        {}

        virtual void exec(
                common::SharedPtr<RequestContext<Request>> request,
                RouteCb<Request> callback,
                lib::string_view messageType
            ) override
        {
            this->impl().exec(std::move(request),std::move(callback),messageType);
        }
};

class ServerServiceNoValidatorTraits
{
    public:

        template <typename MessageT>
        validator::status validate(const MessageT&) const
        {
            return validator::status::code::success;
        }
};

template <typename RequestT>
class ServerServiceMethodsTraits
{
    public:

        using Request=RequestT;

        ServerServiceMethodsTraits(ServerServiceBase* service) : m_service(service)
        {}

        //! @todo Implement exec
        void exec(common::SharedPtr<RequestContext<Request>> request,
                  lib::string_view methodName,
                  bool messageExists,
                  lib::string_view messageType,
                  RouteCb<Request> callback
            );

        //! @todo Implement register method
        void registerMethod(std::shared_ptr<ServerServiceMethod<Request>> method);

        //! @todo Implement method()
        std::shared_ptr<ServerServiceMethod<Request>> method(lib::string_view methodName);

    private:

        ServerServiceBase* m_service;
        std::map<std::string,std::shared_ptr<ServerServiceMethod<Request>>,std::less<>> m_methods;
};

template <typename RequestT>
class ServerServiceMultipleMethods : public ServerServiceT<RequestT,ServerServiceMethodsTraits<RequestT>>
{
    public:

        using Request=RequestT;

        using ServerServiceT<RequestT,ServerServiceMethodsTraits<RequestT>>::ServerServiceT;

        void registerMethod(std::shared_ptr<ServerServiceMethod<Request>> method)
        {
            this->traits().registerMethod(std::move(method));
        }

        std::shared_ptr<ServerServiceMethod<Request>> method(lib::string_view methodName)
        {
            return this->traits().method(methodName);
        }
};

template <typename RequestT>
class ServerServiceSingleMethodTraits
{
    public:

        using Request=RequestT;

        ServerServiceSingleMethodTraits(ServerServiceBase* service,
                                        std::shared_ptr<ServerServiceMethod<Request>> method
                                        ) : m_method(method)
        {
            m_method->setService(service);
        }

        //! @todo Implement exec
        void exec(common::SharedPtr<RequestContext<Request>> request,
                  lib::string_view methodName,
                  bool messageExists,
                  lib::string_view messageType,
                  RouteCb<Request> callback
                  );

    private:

        std::shared_ptr<ServerServiceMethod<Request>> m_method;
};

template <typename RequestT>
using ServerServiceSingleMethod = ServerServiceT<RequestT,ServerServiceSingleMethodTraits<RequestT>>;

template <typename RequestT>
class ServerService : public Service
{
    public:

        using Request=RequestT;

        using Service::Service;

        virtual void handleRequest(
            common::SharedPtr<RequestContext<RequestT>> request,
            RouteCb<RequestT> callback
        ) const =0;
};

template <typename RequestT, typename Impl>
class ServerServiceV : public ServerService<RequestT>,
                       public common::WithImpl<Impl>
{
    public:

        using Request=RequestT;

        template <typename ...ImplArgs>
        ServerServiceV(
                lib::string_view name,
                uint8_t version,
                ImplArgs&&... implArgs
            ) : ServerService<Request>(name,version),
                common::WithImpl<Impl>(std::forward<ImplArgs>(implArgs)...)
        {}

        template <typename ...ImplArgs>
        ServerServiceV(
            lib::string_view name,
            ImplArgs&&... implArgs
            ) : ServerService<Request>(name),
                common::WithImpl<Impl>(this,std::forward<ImplArgs>(implArgs)...)
        {}

        virtual void handleRequest(
            common::SharedPtr<RequestContext<RequestT>> request,
            RouteCb<RequestT> callback
        ) const override
        {
            this->impl().handleRequest(std::move(request),std::move(callback));
        }
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERSERVICE_H
