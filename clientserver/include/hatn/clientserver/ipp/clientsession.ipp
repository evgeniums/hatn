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

#include <hatn/common/meta/chain.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/context.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/api/autherror.h>
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
void ClientSessionTraits<AuthProtocols...>::refresh(common::SharedPtr<ContextT> ctx, CallbackT callback, ClientT* client, clientapi::Response resp)
{
    HATN_CTX_SCOPE("clientsession::refresh")

    // if auth header is not ready then try to use session token
    if (!m_session->isAuthHeaderValid() && m_sessionTokenContainer && resp.isSuccess())
    {
        // check if token expired
        if (!isAuthTokenExpired(m_sessionTokenContainer.get()))
        {
            if (m_session->isSerializedHeaderNeeded())
            {
                auto ec=m_session->serializeAuthHeader(api::AuthProtocol::name(),api::AuthProtocol::version(),m_sessionTokenContainer);
                if (ec)
                {
                    HATN_CTX_ERROR(ec,"failed to serialize session token")
                }
                else
                {
                    callback(std::move(ctx),{});
                    return;
                }
            }
            else
            {
                m_session->setSessionToken(
                    m_sessionTokenContainer->field(auth_with_token::token).byteArrayShared(),
                    std::string{m_sessionTokenContainer->field(auth_with_token::tag).value()}
                );
                callback(std::move(ctx),{});
                return;
            }
        }
    }
    m_sessionTokenContainer.reset();
    m_session->resetSessionToken();
    m_session->resetAuthHeader();

    // try to refresh tokens
    if (m_refreshTokenContainer)
    {
        // check if token expired
        if (!isAuthTokenExpired(m_refreshTokenContainer.get()))
        {
            //! @todo critical: Implement token refreshing
            // return;
        }
    }
    m_refreshTokenContainer.reset();

    // send auth negotiation request
    auto negotiateAuthProtocol=[this,sessionCtx=m_session->sharedMainCtx()](auto&& invokeAuth, auto ctx, auto callback, ClientT* client)
    {
        HATN_CTX_SCOPE("clientsession::negotiate")

        if (login().empty())
        {
            auto ec=clientServerError(ClientServerError::LOGIN_NOT_SET);
            HATN_CTX_ERROR(ec,"login not set in client")
            callback(std::move(ctx),ec);
            return;
        }

        auto req=factory()->template createObject<auth_negotiate_request::managed>();
        req->setFieldValue(auth_negotiate_request::login,login());
        if (!topic().empty())
        {
            req->setFieldValue(auth_negotiate_request::topic,topic());

            //! @todo fill protocols and session_auth
        }
#if 1
        std::cout << "ClientSessionTraits::refresh[negotiateAuthProtocol]: " << req->toString(true) << std::endl;
#endif
        // define request callback
        auto reqCb=[sessionCtx=std::move(sessionCtx),invokeAuth=std::move(invokeAuth),client,callback=std::move(callback)](auto ctx, const Error& ec, api::client::Response response) mutable
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
                callback(std::move(ctx),response.error());
                return;
            }

            invokeAuth(std::move(ctx),std::move(callback),client,std::move(response));
        };

        // prepare msg
        auto msg=typename ClientT::MessageType{auth_negotiate_request::conf().name};
        auto ec=msg.setContent(*req,factory());
        if (ec)
        {
            HATN_CTX_ERROR(ec,"failed to prepare negotiation message")
            callback(std::move(ctx),api::makeApiError(std::move(ec),api::ApiAuthError::AUTH_NEGOTIATION_FAILED,api::ApiAuthErrorCategory::getCategory()));
            return;
        }

        // send request to server
        auto ctx1=ctx.template staticCast<common::TaskContext>();
        ec=client->exec(
            ctx1,
            std::move(reqCb),
            *service(),
            negotiateMethod(),
            std::move(msg),
            topic(),
            api::Priority::Highest,
            timeoutSecs()
        );
        if (ec)
        {
            HATN_CTX_ERROR(ec,"failed to invoke exec negotiation message")
            callback(std::move(ctx),api::makeApiError(std::move(ec),api::ApiAuthError::AUTH_NEGOTIATION_FAILED,api::ApiAuthErrorCategory::getCategory()));
        }
    };

    // invoke auth with negotiated auth protocol
    auto invokeAuth=[this,sessionCtx=m_session->sharedMainCtx()](auto&& handleTokens, auto ctx, auto callback, ClientT* client, api::client::Response negotiationResponse) mutable
    {
        HATN_CTX_SCOPE("clientsession::invokeauth")

        // parse response message
        auto negotiateResp=factory()->template createObject<auth_negotiate_response::managed>();
        auto ec=negotiationResponse.parse(*negotiateResp);
        if (ec)
        {
            HATN_CTX_ERROR_RECORDS(ec,"failed to parse negotiation response message",{"message_type",negotiationResponse.messageType()})
            callback(std::move(ctx),api::makeApiError(std::move(ec),api::ApiAuthError::AUTH_NEGOTIATION_FAILED,api::ApiAuthErrorCategory::getCategory()));
            return;
        }

        HATN_CTX_DEBUG_RECORDS(10,"negotiaion response",{"response",negotiateResp->toString(true)})

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
        if (authResponse.messageType()!=auth_complete::conf().name)
        {
            auto ec=clientServerError(ClientServerError::AUTH_COMPLETION_FAILED);
            HATN_CTX_ERROR_RECORDS(ec,"invalid auth complete message type",{"message_type",authResponse.messageType()})
            callback(std::move(ctx),std::move(ec));
            return;
        }

        // parse response message
        auth_complete::managed authCompleteMsg{factory()};
        authCompleteMsg.setParseToSharedArrays(true);
        du::WireBufSolidShared buf{authResponse.messageData()};
        Error ec;
        du::io::deserialize(authCompleteMsg,buf,ec);
        if (ec)
        {
            HATN_CTX_ERROR(ec,"failed to deserialize auth complete message")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_COMPLETION_FAILED));
            return;
        }
#if 1
        std::cout << "ClientSessionTraits::refresh[handleTokens] resp: " << authCompleteMsg.toString(true) << std::endl;
#endif
        // handle tokens
        m_sessionTokenContainer=authCompleteMsg.field(auth_complete::session_token).sharedValue();
        m_refreshTokenContainer=authCompleteMsg.field(auth_complete::refresh_token).sharedValue();
        if (!m_sessionTokenContainer)
        {
            HATN_CTX_ERROR(ec,"empty session token in auth complete message")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_COMPLETION_FAILED));
            return;
        }
        if (!m_refreshTokenContainer)
        {
            HATN_CTX_ERROR(ec,"empty refresh token in auth complete message")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_COMPLETION_FAILED));
            return;
        }

        // update session token in session
        m_session->setSessionToken(
            m_sessionTokenContainer->field(auth_with_token::token).byteArrayShared(),
            std::string{m_sessionTokenContainer->field(auth_with_token::tag).value()}
        );
#if 0
        std::cout << "ClientSessionTraits::refresh[handleTokens] m_sessionTokenContainer: " << m_sessionTokenContainer->toString(true) << std::endl;
#endif
        // serialize session token for holding in session
        if (!isAuthTokenExpired(m_sessionTokenContainer.get()))
        {
            auto ec=m_session->serializeAuthHeader(api::AuthProtocol::name(),api::AuthProtocol::version(),m_sessionTokenContainer);
            if (ec)
            {
                HATN_CTX_ERROR(ec,"failed to serialize session token")
                callback(std::move(ctx),clientServerError(ClientServerError::AUTH_COMPLETION_FAILED));
                return;
            }
        }
        else
        {
            HATN_CTX_ERROR(clientServerError(ClientServerError::AUTH_TOKEN_EXPIRED),"received already expired session token")
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_COMPLETION_FAILED));
            return;
        }

        // invoke tokens callback
        if (m_tokensUpdatedCb)
        {
            du::WireBufSolidShared sessionTokenBuf{factory()};
            du::io::serialize(*m_sessionTokenContainer,sessionTokenBuf,ec);
            if (ec)
            {
                HATN_CTX_ERROR(ec,"failed to serialize session token")
            }
            ec.reset();
            du::WireBufSolidShared refreshTokenBuf{factory()};
            du::io::serialize(*m_refreshTokenContainer,refreshTokenBuf,ec);
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
