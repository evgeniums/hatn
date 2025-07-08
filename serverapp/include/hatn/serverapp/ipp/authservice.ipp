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

#ifndef HATNSERVERAUTHSERVICE_IPP
#define HATNSERVERAUTHSERVICE_IPP

#include <hatn/api/autherror.h>

#include <hatn/clientserver/auth/defaultauth.h>

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
    HATN_CTX_SCOPE("authnegotiatemethod")

    auto& req=serverapi::request<Request>(request).request();

    // negotiate auth protocol
    const auto& protocols=req.template envContext<WithAuthProtocols>();
    auto proto=protocols->negotiate(msg.get());
    if (proto)
    {
        req.setResponseError(proto.takeError(),apiAuthError(api::ApiAuthError::AUTH_NEGOTIATION_FAILED),api::protocol::ResponseStatus::AuthError);
        callback(std::move(request));
        return;
    }

    // invoke first stage of negotiated protocol if applicable

    //! @todo Negotiate session auth protocol
    //! @todo Use registry of auth protocols, currently only HSS protocol supported
    const auto& hssProtocol=req.template envContext<WithSharedSecretAuthProtocol>();
    if (proto->is(*hssProtocol))
    {
        auto cb=[callback=std::move(callback)](auto request, Error ec, common::SharedPtr<auth_negotiate_response::managed> message)
        {
            auto& req=serverapi::request<Request>(request).request();
            if (ec)
            {
                req.setResponseError(std::move(ec),api::protocol::ResponseStatus::InternalServerError);
            }
            else
            {
                req.response.setSuccessMessage(std::move(message));
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

    req.setResponseError(apiAuthError(api::ApiAuthError::AUTH_NEGOTIATION_FAILED),api::protocol::ResponseStatus::AuthError);
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
AuthService<RequestT>::AuthService() : Base(DefaultAuthService::instance())
{
    this->impl().registerMethod(std::make_shared<AuthNegotiateMethod<Request>>());
    this->impl().registerMethod(std::make_shared<AuthHssCheckMethod<Request>>());
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERAUTHSERVICE_IPP
