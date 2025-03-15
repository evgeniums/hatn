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

//---------------------------------------------------------------

using ServiceMethodStatus = protocol::ResponseStatus;

struct NoValidatorT
{
    template <typename RequestT, typename MessageT>
    validator::error_report operator () (const common::SharedPtr<RequestContext<RequestT>>&) const noexcept
    {
        return validator::error_report{};
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

/********************** ServerService **************************/

class ServerServiceBase
{
    public:

        template <typename RequestT, typename HandlerT, typename MessageT=NoMessage, typename ValidatorT=NoValidatorT>
        void handleMessage(
                common::SharedPtr<RequestContext<RequestT>> request,
                RouteCb<RequestT> callback,
                HandlerT handler,
                hana::type<MessageT> msgType=hana::type_c<NoMessage>,
                const ValidatorT& validator=NoValidator
            )const ;

        template <typename RequestT>
        void methodFailed(
            common::SharedPtr<RequestContext<RequestT>> request,
            RouteCb<RequestT> callback,
            ServiceMethodStatus status,
            const Error& ec=Error{}
        ) const;
};

//---------------------------------------------------------------

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

        void handleRequest(
            common::SharedPtr<RequestContext<Request>> request,
            RouteCb<RequestT> callback
        ) const;

    private:

        void exec(
                common::SharedPtr<RequestContext<Request>> request,
                RouteCb<Request> callback,
                lib::string_view methodName,
                bool messageExists,
                lib::string_view messageType
            ) const
        {
            return this->traits.exec(std::move(request),std::move(callback),methodName,messageExists,messageType);
        }
};

//---------------------------------------------------------------

template <typename RequestT>
class ServerService : public Service
{
public:

    using Request=RequestT;

    using Service::Service;

    virtual ~ServerService()
    {}

    virtual void handleRequest(
        common::SharedPtr<RequestContext<RequestT>> request,
        RouteCb<RequestT> callback
    ) const =0;
};

//---------------------------------------------------------------

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

/********************** ServiceMethod **************************/

class ServiceMethodBase
{
    public:

        ServiceMethodBase(
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

//---------------------------------------------------------------

/**
 * @brief Base template class for server methods.
 *
 * Actual processing is performed by Traits.
 * Traits must define two methods:
 *
 * template <typename RequestT, typename MessageT>
 * void exec(common::SharedPtr<RequestContext<Request>> request,RouteCb<Request> callback, common::SharedPtr<Message> msg)
 *
 * template <typename RequestT, typename MessageT>
 * validator::error_report validate(const common::SharedPtr<RequestContext<RequestT>>& request,const MessageT& msg)
)
 */
template <typename RequestT, typename Traits, typename MessageT=NoMessage>
class ServiceMethodT : public common::WithTraits<Traits>
{
    public:

        using Request=RequestT;
        using Message=MessageT;

        template <typename ...TraitsArgs>
        ServiceMethodT(
            ServiceMethodBase* base,
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
            bool messageExists,
            lib::string_view messageType
        ) const;

    private:

        ServiceMethodBase* m_base;
};

//---------------------------------------------------------------

template <typename RequestT>
class ServiceMethod : public ServiceMethodBase
{
    public:

        using Request=RequestT;

        using ServiceMethodBase::ServiceMethodBase;

        virtual ~ServiceMethod()=default;

        virtual void exec(
            common::SharedPtr<RequestContext<Request>> request,
            bool messageExists,
            RouteCb<Request> callback,
            lib::string_view messageType
        ) const =0;
};

//---------------------------------------------------------------

template <typename Impl, typename RequestT>
class ServiceMethodV : public ServiceMethod<RequestT>,
                             public common::WithImpl<Impl>
{
    public:

        using ImplBase=common::WithImpl<Impl>;
        using Request=RequestT;

        template <typename ...ImplArgs>
        ServiceMethodV(
                std::string name,
                ImplArgs&& ...implArgs
            ) : ServiceMethod<RequestT>(std::move(name)),
                ImplBase(this,std::forward<ImplArgs>(implArgs)...)
        {}

        virtual void exec(
                common::SharedPtr<RequestContext<Request>> request,
                RouteCb<Request> callback,
                bool messageExists,
                lib::string_view messageType
            ) const override
        {
            this->impl().exec(std::move(request),std::move(callback),messageExists,messageType);
        }
};

//---------------------------------------------------------------

class NoValidatorTraits
{
    public:

        template <typename MessageT>
        validator::status validate(const MessageT&) const
        {
            return validator::status::code::success;
        }
};

//---------------------------------------------------------------

/********************** ServiceMultipleMethods **************************/

template <typename RequestT>
class ServiceMultipleMethodsTraits
{
    public:

        using Request=RequestT;

        ServiceMultipleMethodsTraits(ServerServiceBase* service) : m_service(service)
        {}

        void exec(
            common::SharedPtr<RequestContext<Request>> request,
            RouteCb<Request> callback,
            lib::string_view methodName,
            bool messageExists,
            lib::string_view messageType
        ) const;

        void registerMethod(std::shared_ptr<ServiceMethod<Request>> method);

        std::shared_ptr<ServiceMethod<Request>> method(lib::string_view methodName) const;

    private:

        ServerServiceBase* m_service;

        //! @todo Use FlatMap?
        std::map<std::string,std::shared_ptr<ServiceMethod<Request>>,std::less<>> m_methods;
};

//---------------------------------------------------------------

template <typename RequestT>
class ServiceMultipleMethods : public ServerServiceT<RequestT,ServiceMultipleMethodsTraits<RequestT>>
{
    public:

        using Request=RequestT;

        using ServerServiceT<RequestT,ServiceMultipleMethodsTraits<RequestT>>::ServerServiceT;

        void registerMethod(std::shared_ptr<ServiceMethod<Request>> method)
        {
            this->traits().registerMethod(std::move(method));
        }

        std::shared_ptr<ServiceMethod<Request>> method(lib::string_view methodName) const
        {
            return this->traits().method(methodName);
        }
};

/********************** ServiceSingleMethod **************************/

template <typename RequestT, typename MethodT>
class ServiceSingleMethodTraits
{
    public:

        using Request=RequestT;

        ServiceSingleMethodTraits(ServerServiceBase* service,
                                        std::shared_ptr<ServiceMethod<Request>> method
                                        ) : m_method(method)
        {
            m_method->setService(service);
        }

        void exec(common::SharedPtr<RequestContext<Request>> request,
                  RouteCb<Request> callback,
                  lib::string_view methodName,
                  bool messageExists,
                  lib::string_view messageType
        ) const;

    private:

        std::shared_ptr<MethodT> m_method;
};

template <typename RequestT, typename MethodT>
using ServiceSingleMethod = ServerServiceT<RequestT,ServiceSingleMethodTraits<RequestT,MethodT>>;

//---------------------------------------------------------------

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERSERVICE_H
