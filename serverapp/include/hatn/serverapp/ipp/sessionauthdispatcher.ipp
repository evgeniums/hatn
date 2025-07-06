/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/ipp/sessionauthdispatcher.ipp
  */

/****************************************************************************/

#ifndef HATNSERVERSESSIONAUTHDISPATCHER_IPP
#define HATNSERVERSESSIONAUTHDISPATCHER_IPP

#include <hatn/logcontext/context.h>

#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/visitors.h>

#include <hatn/api/apiliberror.h>

#include <hatn/clientserver/auth/defaultauth.h>

#include <hatn/serverapp/auth/authservice.h>
#include <hatn/serverapp/auth/sessionauthdispatcher.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

/**************************** SessionAuthDispatcher ***************************/

//--------------------------------------------------------------------------

template <typename EnvT, typename RequestT>
SessionAuthProtocol<EnvT,RequestT>::SessionAuthProtocol() : api::AuthProtocol(DefaultAuthSessionProtocol::instance())
{
}

//--------------------------------------------------------------------------

template <typename EnvT, typename RequestT>
void SessionAuthProtocol<EnvT,RequestT>::invoke(
        common::SharedPtr<serverapi::RequestContext<Request>> reqCtx,
        serverapi::DispatchCb<Request> callback,
        common::ByteArrayShared authFieldContent
    )
{
    HATN_CTX_SCOPE("sessionauthprotocol")

    auto& req=serverapi::request<Request>(reqCtx).request();

    // parse token holder
    auto tokenHolder=req.factory()->template createObject<auth_with_token::managed>();
    Error ec;
    du::WireBufSolidShared holderBuf{std::move(authFieldContent)};
    du::io::deserialize(*tokenHolder,holderBuf,ec);
    if (ec)
    {
        HATN_CTX_SCOPE_ERROR("failed to parse auth field content")
        req.response.setStatus(api::protocol::ResponseStatus::AuthError,api::apiLibError(api::ApiLibError::AUTH_HEADER_FORMAT));
        callback(std::move(reqCtx));
        return;
    }

    // define check session callback
    auto cb=[](auto ctx, const Error& ec, common::SharedPtr<auth_token::managed> response)
    {
        //! @todo critical: implement response
    };

    // check session
    auto& reqEnv=serverapi::requestEnv<Request>(reqCtx);
    const auto& sessionController=reqEnv->sessionController();
    sessionController.checkSession(
        std::move(reqCtx),
        std::move(cb),
        std::move(tokenHolder)
    );
}

/**************************** SessionAuthDispatcher ***************************/

//--------------------------------------------------------------------------

template <typename EnvT, typename RequestT>
SessionAuthDispatcher<EnvT,RequestT>::SessionAuthDispatcher()
{
    this->addSkipAuthService(DefaultAuthService::instance());
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERSESSIONAUTHDISPATCHER_IPP
