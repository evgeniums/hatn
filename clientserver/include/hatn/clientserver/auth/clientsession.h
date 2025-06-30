/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/auth/clientsession.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSESSION_H
#define HATNCLIENTSESSION_H

#include <hatn/common/env.h>

#include <hatn/api/client/session.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/auth/clientauthprotocol.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

class HATN_CLIENT_SERVER_EXPORT ClientSessionImpl
{
    public:

        using Callback=clientapi::SessionRefreshCb;
        using TokensUpdatedFn=std::function<void(common::ByteArrayShared,common::ByteArrayShared)>;

        void setLogin(
            lib::string_view login,
            lib::string_view topic={}
        );

        Error loadSessionToken(lib::string_view content);

        Error loadRefreshToken(lib::string_view content);

        void authWithTokens(Callback callback);

        void setTokensUpdatedCb(TokensUpdatedFn tokensUpdatedCb)
        {
            m_tokensUpdatedCb=std::move(tokensUpdatedCb);
        }

    protected:

        common::SharedPtr<auth_token::managed> m_sessionToken;
        common::SharedPtr<auth_token::managed> m_refreshToken;

        TokensUpdatedFn m_tokensUpdatedCb;
};

template <typename ...AuthProtocols>
class ClientSessionTraits : public common::TaskContext,
                            public ClientSessionImpl,
                            public common::Env<AuthProtocols...>
{
    public:

        template <typename ...Args>
        ClientSessionTraits(Args&& ...args);

        template <typename ContextT, typename CallbackT, typename ClientT>
        void refresh(common::SharedPtr<ContextT> ctx, CallbackT callback, ClientT* client, clientapi::Response ={});

    private:

        using AuthHandler=std::function<void (common::TaskContextShared,Callback,common::SharedPtr<auth_negotiate_response::managed>)>;

        void invokeAuthProtocol(
            Callback callback,
            common::SharedPtr<auth_negotiate_response::managed> authNegotiateResponse
        );

        common::FlatMap<ClientAuthProtocol,AuthHandler> m_protocolHandlers;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSESSION_H
