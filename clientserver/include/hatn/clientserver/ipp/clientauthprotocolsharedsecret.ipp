/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/ipp/clientauthprotocolsharedsecret.ipp
  */

/****************************************************************************/

#ifndef HATNAUTHPROTOCOLSHAREDSECRET_IPP
#define HATNAUTHPROTOCOLSHAREDSECRET_IPP

#include "hatn/api/message.h"
#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/api/autherror.h>
#include <hatn/api/service.h>
#include <hatn/api/method.h>
#include <hatn/api/priority.h>
#include <hatn/api/client/clientresponse.h>

#include <hatn/clientserver/clientservererror.h>
#include <hatn/clientserver/auth/authprotocol.h>
#include <hatn/clientserver/auth/clientsession.h>
#include <hatn/clientserver/auth/clientauthprotocolsharedsecret.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextT, typename CallbackT, typename ClientT>
void ClientAuthProtocolSharedSecret::invoke(
        common::SharedPtr<ContextT> ctx,
        CallbackT callback,
        ClientT* client,
        common::SharedPtr<auth_negotiate_response::managed> authNegotiateResponse
    ) const
{
    HATN_CTX_SCOPE("authsharedsecret")

    // check message type
    if (authNegotiateResponse->fieldValue(auth_protocol_response::message_type) != auth_hss_challenge::conf().name)
    {
        auto ec=api::makeApiError(api::ApiAuthError::AUTH_NEGOTIATION_FAILED,api::ApiAuthErrorCategory::getCategory());
        HATN_CTX_ERROR_RECORDS(ec,"invalid message type in auth_negotiate_response",{"message_type",authNegotiateResponse->fieldValue(auth_protocol_response::message_type)})
        callback(std::move(ctx),ec,{});
        return;
    }

    // parse message
    Error ec;
    auth_hss_challenge::type challengeMsg{session()->factory()};
    du::WireBufSolidShared buf{authNegotiateResponse->field(auth_protocol_response::message).skippedNotParsedContent()};
    du::io::deserialize(challengeMsg,buf,ec);
    if (ec)
    {
        HATN_CTX_ERROR(ec,"failed to parse auth_hss_challenge in auth_negotiate_response")
        callback(std::move(ctx),api::makeApiError(api::ApiAuthError::AUTH_NEGOTIATION_FAILED,api::ApiAuthErrorCategory::getCategory()),{});
        return;
    }

    // prepare HSS auth request with MAC using shared secret
    auto req=session()->factory()->template createObject<auth_hss_check::managed>();
    req->setFieldValue(auth_hss_check::token,authNegotiateResponse->fieldValue(auth_protocol_response::token));
    ec=calculateMAC(challengeMsg.fieldValue(auth_hss_challenge::challenge),*req->field(auth_hss_check::mac).buf(true),challengeMsg.fieldValue(auth_hss_challenge::cipher_suite));
    if (ec)
    {
        HATN_CTX_ERROR(ec,"failed to calculate MAC")
        callback(std::move(ctx),clientServerError(ClientServerError::AUTH_PROCESSING_FAILED),{});
        return;
    }

    // define request callback
    auto reqCb=[callback=std::move(callback)](auto ctx, const Error& ec, api::client::Response response) mutable
    {
        HATN_CTX_SCOPE("authsharedsecretexeccb")

        if (ec)
        {
            HATN_CTX_DEBUG_RECORDS(1,"failed to send HSS to server",{"errm",ec.message()},{"errc",ec.codeString()})
            callback(std::move(ctx),ec,api::client::Response{});
            return;
        }

        callback(std::move(ctx),Error{},std::move(response));
    };

    // prepare msg
    auto msg=api::Message<>{auth_hss_check::conf().name};
    ec=msg.setContent(*req,session()->factory());
    if (ec)
    {
        HATN_CTX_ERROR(ec,"failed to prepare HSS check message")
        callback(std::move(ctx),clientServerError(ClientServerError::AUTH_PROCESSING_FAILED),api::client::Response{});
        return;
    }

    // send request to server
    ec=client->exec(
        ctx,
        std::move(reqCb),
        *session()->service(),
        api::Method{AuthHssCheckMethodName},
        std::move(msg),
        session()->topic(),
        api::Priority::Highest,
        session()->timeoutSecs()
    );
    if (ec)
    {
        HATN_CTX_ERROR(ec,"failed to exec HSS check message")
        callback(std::move(ctx),clientServerError(ClientServerError::AUTH_PROCESSING_FAILED),api::client::Response{});
        return;
    }
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNAUTHPROTOCOLSHAREDSECRET_IPP
