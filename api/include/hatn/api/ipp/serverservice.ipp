/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/serverservice.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERSERVICE_IPP
#define HATNAPISERVERSERVICE_IPP

#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/visitors.h>

#include <hatn/api/server/serverservice.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

/********************** ServerServiceBase **************************/

template <typename RequestT, typename HandlerT, typename MessageT, typename ValidatorT>
void ServerServiceBase::handleMessage(
        common::SharedPtr<RequestContext<RequestT>> request,
        RouteCb<RequestT> callback,
        HandlerT handler,
        hana::type<MessageT>,
        const ValidatorT& validator
    ) const
{
    if constexpr (std::is_same_v<MessageT,NoMessage>)
    {
        handler(std::move(request),std::move(callback));
    }
    else
    {
        auto& req=request->template get<RequestT>();

        // build message object
        auto msg=req.template get<AllocatorFactory>().factory()->template createObject<MessageT>();

        //  parse message
        const auto& messageField=request->unit.field(protocol::request::message);
        du::WireBufSolidShared buf{messageField.skippedNotParsedContent()};
        if (!du::io::deserialize(*msg,buf))
        {
            req.response.setStatus(protocol::ResponseStatus::FormatError);
            cb(std::move(request));
            return;
        }

        // validate message
        auto validationStatus=validator(*msg);
        if (!validationStatus)
        {
            //! @todo construct locale aware validation reports

            req.response.setStatus(protocol::ResponseStatus::ValidationError);
            cb(std::move(request));
            return;
        }

        // handle message in service
        handler(std::move(request),std::move(callback), std::move(msg));
    }
}

//---------------------------------------------------------------

template <typename RequestT>
void ServerServiceBase::methodFailed(
        common::SharedPtr<RequestContext<RequestT>> request,
        RouteCb<RequestT> callback,
        ServiceMethodStatus status,
        const Error& ec
    ) const
{
    auto& req=request->template get<RequestT>();
    req.response.setStatus(status,ec);
    callback(std::move(request));
}

/********************** ServerServiceT **************************/

template <typename RequestT, typename Traits>
void ServerServiceT<RequestT,Traits>::handleRequest(
        common::SharedPtr<RequestContext<Request>> request,
        RouteCb<RequestT> callback
    ) const
{
    auto& req=request->template get<RequestT>();

    auto cb=[callback(std::move(callback))](common::SharedPtr<api::server::RequestContext<RequestT>> request)
    {
        //! @todo Log status?
        callback(std::move(request));
    };

    exec(
        std::move(request),
        std::move(cb),
        req.unit.field(protocol::request::method).value(),
        req.unit.field(protocol::request::message).isSet(),
        req.unit.field(protocol::request::message_type).value()
    );
}

/********************** ServiceMethodT **************************/

template <typename RequestT, typename Traits, typename MessageT>
void ServiceMethodT<RequestT,Traits,MessageT>::exec(
        common::SharedPtr<RequestContext<Request>> request,
        RouteCb<Request> callback,
        bool messageExists,
        lib::string_view messageType
    ) const
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
        // check if message is set in request
        if (!messageExists)
        {
            this->service()->methodFailed(
                std::move(request),
                std::move(callback),
                ServiceMethodStatus::MessageMissing
                );
            return;
        }

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
                return this->traits().validate(msg);
            }
        );
    }
}

/********************** ServiceMultipleMethodsTraits **************************/


template <typename RequestT>
void ServiceMultipleMethodsTraits<RequestT>::registerMethod(
        std::shared_ptr<ServiceMethod<Request>> method
    )
{
    m_methods[method->name()]=method;
}

//---------------------------------------------------------------

template <typename RequestT>
std::shared_ptr<ServiceMethod<RequestT>> ServiceMultipleMethodsTraits<RequestT>::method(
        lib::string_view methodName
    ) const
{
    auto it=m_methods.find(methodName);
    if (it!=m_methods.end())
    {
        return it->second;
    }
    return std::shared_ptr<ServiceMethod<RequestT>>{};
}

//---------------------------------------------------------------

template <typename RequestT>
void ServiceMultipleMethodsTraits<RequestT>::exec(
        common::SharedPtr<RequestContext<Request>> request,
        RouteCb<Request> callback,
        lib::string_view methodName,
        bool messageExists,
        lib::string_view messageType
    ) const
{
    auto& req=request->template get<RequestT>();

    // find method
    auto mthd=method(methodName);
    if (!mthd)
    {
        m_service->methodFailed(
            std::move(request),
            std::move(callback),
            ServiceMethodStatus::UnknownMethod
            );
        return;
    }

    // exec method
    mthd->exec(
        std::move(request),
        std::move(callback),
        messageExists,
        messageType
    );
}

/********************** ServiceSingleMethodTraits **************************/

template <typename RequestT, typename MethodT>
void ServiceSingleMethodTraits<RequestT,MethodT>::exec(
        common::SharedPtr<RequestContext<Request>> request,
        RouteCb<Request> callback,
        lib::string_view methodName,
        bool messageExists,
        lib::string_view messageType
    ) const
{
    auto& req=request->template get<RequestT>();

    // check if method names match
    if (methodName!=m_method->name())
    {
        m_method->service()->methodFailed(
            std::move(request),
            std::move(callback),
            ServiceMethodStatus::UnknownMethod
        );
        return;
    }

    // exec method
    m_method->exec(
        std::move(request),
        std::move(callback),
        messageExists,
        messageType
    );
}

//---------------------------------------------------------------

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERSERVICE_IPP
