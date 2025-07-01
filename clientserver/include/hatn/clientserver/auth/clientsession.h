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

#include <hatn/base/configobject.h>

#include <hatn/api/client/session.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/auth/clientauthprotocol.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(client_session_config,
    HDU_FIELD(timeout_secs,TYPE_UINT32,1,false,30)
)

class HATN_CLIENT_SERVER_EXPORT ClientSessionImpl : public common::pmr::WithFactory,
                                                    public api::AuthProtocol,
                                                    public api::WithService,
                                                    public base::ConfigObject<client_session_config::type>
{
    public:

        using Callback=clientapi::SessionRefreshCb;
        using TokensUpdatedFn=std::function<void(common::ByteArrayShared,common::ByteArrayShared)>;

        ClientSessionImpl(
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        void setLogin(
            lib::string_view login,
            lib::string_view topic={}
        )
        {
            m_login=login;
            m_topic=topic;
        }

        Error loadSessionToken(lib::string_view content);

        Error loadRefreshToken(lib::string_view content);

        void setTokensUpdatedCb(TokensUpdatedFn tokensUpdatedCb)
        {
            m_tokensUpdatedCb=std::move(tokensUpdatedCb);
        }

        const std::string& login() const noexcept
        {
            return m_login;
        }

        const std::string& topic() const noexcept
        {
            return m_topic;
        }

        uint32_t timeoutSecs() const
        {
            return config().fieldValue(client_session_config::timeout_secs);
        }

    protected:

        common::SharedPtr<auth_token::managed> m_sessionToken;
        common::SharedPtr<auth_token::managed> m_refreshToken;

        std::string m_login;
        std::string m_topic;

        TokensUpdatedFn m_tokensUpdatedCb;

    private:

        Error loadToken(common::SharedPtr<auth_token::managed>& token, lib::string_view content) const;
};

template <typename ...AuthProtocols>
class ClientSessionTraits : public ClientSessionImpl,
                            public common::Env<AuthProtocols...>
{
    public:

        using SessionType=clientapi::Session<ClientSessionTraits<AuthProtocols...>>;

        template <typename ...Args>
        ClientSessionTraits(SessionType* session, Args&& ...args);

        template <typename ContextT, typename CallbackT, typename ClientT>
        void refresh(common::SharedPtr<ContextT> ctx, CallbackT callback, ClientT* client, clientapi::Response ={});

    private:

        SessionType* m_session;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSESSION_H
