/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/ipp/authservice.ipp
  */

/****************************************************************************/

#ifndef HATNLOCALSESSIONCONTROLLER_IPP
#define HATNLOCALSESSIONCONTROLLER_IPP

#include <hatn/serverapp/auth/authprotocols.h>
#include <hatn/serverapp/auth/sharedsecretprotocol.h>
#include <hatn/serverapp/auth/authservice.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

/**************************** AuthNegotiateMethod ***************************/

//--------------------------------------------------------------------------

template <typename RequestT>
void AuthNegotiateMethodImpl<RequestT>::exec(
        common::SharedPtr<serverapi::RequestContext<Request>> request,
        serverapi::RouteCb<Request> callback,
        common::SharedPtr<Message> msg
    ) const
{
    auto& req=serverapi::request<Request>(request).request();

    // negotiate auth protocol
    const auto& protocols=req.template envContext<WithAuthProtocols>();
    auto proto=protocols->negotiate(msg.get());
    if (proto)
    {
        //! @todo critical: log error
        req.response.setStatus(api::protocol::ResponseStatus::AuthError,clientServerError(ClientServerError::AUTH_NEGOTIATION_FAILED));
        callback(std::move(request));
        return;
    }

    // invoke first stage of negotiated protocol if applicable

    //! @todo Negotiate session auth protocol
    //! @todo Use registry of auth protocols, currently only HSS protocol supported
    const auto& hssProtocol=req.template envContext<WithSharedSecretAuthProtocol>();
    if (proto->is(*hssProtocol))
    {
        auto cb=[callback=std::move(callback)](auto request, const Error& ec, common::SharedPtr<auth_negotiate_response::managed> message)
        {
            auto& req=serverapi::request<Request>(request).request();
            //! @todo critical: chain and log error
            //! @todo check if it is internal error or invalid login data
            if (ec)
            {
                req.response.setStatus(api::protocol::ResponseStatus::AuthError,ec);
            }
            else
            {
                //! @todo set category?
                req.response.unit.field(api::protocol::response::message).set(std::move(message));
                req.response.setStatus();
            }
            callback(std::move(request));
        };

        hssProtocol->prepare(
            std::move(request),
            std::move(cb),
            std::move(msg),
            req.factory()
        );
        return;
    }

    //! @todo critical: log error
    req.response.setStatus(api::protocol::ResponseStatus::AuthError,clientServerError(ClientServerError::AUTH_NEGOTIATION_FAILED));
    callback(std::move(request));
    return;
}

//--------------------------------------------------------------------------

template <typename RequestT>
HATN_VALIDATOR_NAMESPACE::error_report AuthNegotiateMethodImpl<RequestT>::validate(
        const common::SharedPtr<serverapi::RequestContext<Request>> /*request*/,
        const Message& /*msg*/
    ) const
{
    //! @todo critical: Implement validation of AuthNegotiate
    return HATN_VALIDATOR_NAMESPACE::error_report{};
}

/***************************** AuthService ***********************************/

//--------------------------------------------------------------------------

template <typename RequestT>
AuthService<RequestT>::AuthService() : Base(AuthServiceName,AuthServiceVersion)
{
    auto negotiate=std::make_shared<AuthNegotiateMethod<Request>>();
    this->impl().registerMethod(std::move(negotiate));
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALSESSIONCONTROLLER_IPP
