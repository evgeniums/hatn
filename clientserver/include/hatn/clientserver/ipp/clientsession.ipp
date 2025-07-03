/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/ipp/clientsession.ipp
  */

/****************************************************************************/

#ifndef HATNCLIENTSESSIONT_IPP
#define HATNCLIENTSESSIONT_IPP

#include "hatn/api/message.h"
#include <hatn/common/meta/chain.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/context.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/api/service.h>
#include <hatn/api/method.h>
#include <hatn/api/priority.h>
#include <hatn/api/client/clientresponse.h>

#include <hatn/clientserver/clientservererror.h>
#include <hatn/clientserver/auth/clientsession.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ...AuthProtocols>
template <typename ...Args>
ClientSessionTraits<AuthProtocols...>::ClientSessionTraits(SessionType* session, const common::pmr::AllocatorFactory* factory, Args&& ...args)
    : ClientSessionBase(factory),
      common::Env<AuthProtocols...>(std::forward<Args>(args)...),
      m_session(session)
{
    hana::for_each(this->contexts(),
                   [this](auto& protocol)
                   {
                       protocol.setSession(this);
                   }
    );
}

//--------------------------------------------------------------------------

template <typename ...AuthProtocols>
template <typename ContextT, typename CallbackT, typename ClientT>
void ClientSessionTraits<AuthProtocols...>::refresh(common::SharedPtr<ContextT> ctx, CallbackT callback, ClientT* client, clientapi::Response)
{
    HATN_CTX_SCOPE("clientsession::refresh")

    // if auth header is not ready then try to use session token
    if (!m_session->isAuthHeaderValid())
    {
        if (m_sessionToken)
        {
            // check if token expired
            if (!isAuthTokenExpired(m_sessionToken.get()))
            {
                std::ignore=m_session->serializeAuthHeader(name(),version(),m_sessionToken);
                callback(std::move(ctx),{});
                return;
            }
        }
    }
    m_sessionToken.reset();

    // try to refresh tokens
    if (m_refreshToken)
    {
        // check if token expired
        if (!isAuthTokenExpired(m_refreshToken.get()))
        {
            //! @todo critical: Implement token refreshing
            // return;
        }
    }
    m_refreshToken.reset();

    // send auth negotiation request
    auto negotiateAuthProtocol=[this,sessionCtx=m_session->sharedMainCtx()](auto&& invokeAuth, auto ctx, auto callback, ClientT* client)
    {
        HATN_CTX_SCOPE("clientsession::negotiate")

        auto req=factory()->template createObject<auth_negotiate_request::managed>();
        req->setFieldValue(auth_negotiate_request::login,login());
        if (!topic().empty())
        {
            req->setFieldValue(auth_negotiate_request::topic,topic());

            //! @todo fill protocols and session_auth
        }

        // define request callback
        auto reqCb=[this,sessionCtx=std::move(sessionCtx),invokeAuth=std::move(invokeAuth),client,callback=std::move(callback)](auto ctx, const Error& ec, api::client::Response response) mutable
        {
            HATN_CTX_SCOPE("clientsession::negotiatecb")

            if (ec)
            {
                HATN_CTX_DEBUG_RECORDS(1,"failed to send auth negotiate to server",{"errm",ec.message()},{"errc",ec.codeString()})
                callback(std::move(ctx),ec);
                return;
            }

            if (!response.isSuccess())
            {
                callback(std::move(ctx),response.parseError(factory()));
                return;
            }

            invokeAuth(std::move(ctx),std::move(callback),client,std::move(response));
        };

        // prepare msg
        auto msg=api::Message<>{auth_negotiate_request::conf().name};
        auto ec=msg.setContent(*req,factory());
        if (ec)
        {
            HATN_CTX_ERROR(ec,"failed to prepare negotiation message")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_NEGOTIATION_FAILED));
            return;
        }

        // send request to server
        //! @todo optimization: Use static method singleton
        ec=client->exec(
            ctx,
            std::move(reqCb),
            *service(),
            api::Method{AuthNegotiateMethodName},
            std::move(msg),
            topic(),
            api::Priority::Highest,
            timeoutSecs()
        );        
        if (ec)
        {
            HATN_CTX_ERROR(ec,"failed to invoke exec negotiation message")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_NEGOTIATION_FAILED));
        }
    };

    // invoke auth with negotiated auth protocol
    auto invokeAuth=[this,sessionCtx=m_session->sharedMainCtx()](auto&& handleTokens, auto ctx, auto callback, ClientT* client, api::client::Response negotiationResponse) mutable
    {
        HATN_CTX_SCOPE("clientsession::invokeauth")

        // check response message
        if (negotiationResponse.unit->fieldValue(api::protocol::response::message_type)!=auth_negotiate_response::conf().name)
        {
            auto ec=clientServerError(ClientServerError::AUTH_NEGOTIATION_FAILED);
            HATN_CTX_ERROR_RECORDS(ec,"invalid negotiation response message type",{"message_type",negotiationResponse.unit->fieldValue(api::protocol::response::message_type)})
            callback(std::move(ctx),std::move(ec));
            return;
        }

        // parse response message
        auto negotiateResp=factory()->template createObject<auth_negotiate_response::managed>();
        du::WireBufSolidShared buf{negotiationResponse.message};
        Error ec;
        du::io::deserialize(*negotiateResp,buf,ec);
        if (ec)
        {
            HATN_CTX_ERROR(ec,"failed to deserialize negotiation response message")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_NEGOTIATION_FAILED));
            return;
        }

        // define invoke cb
        auto invokeCb=[handleTokens=std::move(handleTokens),callback=std::move(callback),sessionCtx=std::move(sessionCtx)](auto ctx,const Error& ec, api::client::Response authResponse) mutable
        {
            HATN_CTX_SCOPE("clientsession::invokeauthcb")

            if (ec)
            {
                callback(std::move(ctx),std::move(ec));
                return;
            }

            handleTokens(std::move(ctx),std::move(callback),std::move(authResponse));
        };

        // invoke negotiated auth protocol
        const auto& protoField=negotiateResp->field(auth_negotiate_response::protocol);
        auto selector=[&protoField](const auto& authProto)
        {
            return protoField.field(api::auth_protocol::protocol).value()==authProto.name() && protoField.field(api::auth_protocol::version).value()==authProto.version();
        };
        auto visitor=[ctx,invokeCb=std::move(invokeCb),client,negotiateResp=std::move(negotiateResp)](auto& authProto)
        {
            authProto.invoke(std::move(ctx),std::move(invokeCb),client,std::move(negotiateResp));
        };
        this->visitIf(visitor,selector);
    };

    // handle auth tokens from response
    auto handleTokens=[this](auto ctx, auto callback, api::client::Response authResponse)
    {
        HATN_CTX_SCOPE("clientsession::handleTokens")

        // check response message
        if (authResponse.unit->fieldValue(api::protocol::response::message_type)!=auth_complete::conf().name)
        {
            auto ec=clientServerError(ClientServerError::AUTH_COMPLETION_FAILED);
            HATN_CTX_ERROR_RECORDS(ec,"invalid auth complete message type",{"message_type",authResponse.unit->fieldValue(api::protocol::response::message_type)})
            callback(std::move(ctx),std::move(ec));
            return;
        }

        // parse response message
        auth_complete::shared_managed authCompleteMsg{factory()};
        du::WireBufSolidShared buf{authResponse.message};
        Error ec;
        du::io::deserialize(authCompleteMsg,buf,ec);
        if (ec)
        {
            HATN_CTX_ERROR(ec,"failed to deserialize auth complete message")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_COMPLETION_FAILED));
            return;
        }

        // handle tokens
        m_sessionToken=authCompleteMsg.field(auth_complete::session_token).get();
        m_refreshToken=authCompleteMsg.field(auth_complete::refresh_token).get();
        if (!m_sessionToken)
        {
            HATN_CTX_ERROR(ec,"empty session token in auth complete message")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_COMPLETION_FAILED));
            return;
        }
        if (!m_refreshToken)
        {
            HATN_CTX_ERROR(ec,"empty refresh token in auth complete message")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_COMPLETION_FAILED));
            return;
        }

        // serialize tokens and invoke tokens callback
        if (m_tokensUpdatedCb)
        {
            du::WireBufSolidShared sessionTokenBuf{factory()};
            du::io::serialize(*m_sessionToken,sessionTokenBuf,ec);
            if (ec)
            {
                HATN_CTX_ERROR(ec,"failed to serialize session token")
            }
            ec.reset();
            du::WireBufSolidShared refreshTokenBuf{factory()};
            du::io::serialize(*m_refreshToken,refreshTokenBuf,ec);
            if (ec)
            {
                HATN_CTX_ERROR(ec,"failed to serialize refresh token")
            }

            // invoke tokens callback
            m_tokensUpdatedCb(sessionTokenBuf.sharedMainContainer(),refreshTokenBuf.sharedMainContainer());
        }

        // done
        callback(std::move(ctx),Error{});
    };

    // invoke chain
    auto chain=hatn::chain(
        std::move(negotiateAuthProtocol),
        std::move(invokeAuth),
        std::move(handleTokens)
    );
    chain(std::move(ctx),std::move(callback),client);
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSESSIONT_IPP
