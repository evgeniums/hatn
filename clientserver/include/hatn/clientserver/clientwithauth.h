/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/clientwithauth.h
  */

/****************************************************************************/

#ifndef HATNCLIENTWITHAUTH_H
#define HATNCLIENTWITHAUTH_H

#include <hatn/api/client/client.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/auth/clientsessionsharedsecret.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

using DefaultRequestContext=HATN_LOGCONTEXT_NAMESPACE::TaskLogContext;

template <typename RouterT, typename RequestContextT=HATN_LOGCONTEXT_NAMESPACE::TaskLogContext, typename MessageBufT=du::WireData, typename RequestUnitT=api::RequestManaged, typename ...AuthProtocols>
class ClientWithAuthT : public DefaultRequestContext
{
    public:

        using Router=RouterT;
        using RequestContext=RequestContextT;
        using MessageBuf=MessageBufT;
        using RequestUnit=RequestUnitT;
        using Callback=clientapi::RequestCb<RequestContext>;

        using Session=ClientSession<AuthProtocols...>;
        using SessionWrapper=clientapi::SessionWrapper<Session>;
        using Client=clientapi::Client<RouterT,SessionWrapper,RequestContextT,MessageBufT,RequestUnitT>;

        using Req=clientapi::Request<SessionWrapper,MessageBufT,RequestUnitT>;
        using MessageType=typename Req::MessageType;
        using ReqCtx=clientapi::RequestContext<Req,RequestContext>;

        void setName(lib::string_view name);
        lib::string_view name() const;

        Error loadLogConfig(
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const std::string& configPath
        );

        Error exec(
            common::SharedPtr<RequestContext> ctx,
            Callback callback,
            const api::Service& service,
            const api::Method& method,
            MessageType message,
            lib::string_view topic={},
            api::Priority priority=api::Priority::Normal,
            uint32_t timeoutMs=0,
            clientapi::MethodAuth methodAuth={}
        );

        void setClient(Client* client)
        {
            m_client=client;
        }

        void setSession(Session* session)
        {
            m_sessionWrapper.setSession(session);
        }

        Session* session()
        {
            return &m_sessionWrapper.session();
        }

        const Session* session() const
        {
            return &m_sessionWrapper.session();
        }

    private:

        Client* m_client=nullptr;
        SessionWrapper m_sessionWrapper;
};

template <typename RouterT, typename RequestContextT=DefaultRequestContext, typename MessageBufT=du::WireData, typename RequestUnitT=api::RequestManaged, typename ...ExtraAuthProtocols>
class ClientWithSharedSecretAuthT : public ClientWithAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,ClientAuthProtocolSharedSecret,ExtraAuthProtocols...>
{
    public:

        const ClientAuthProtocolSharedSecret* sharedSecretAuth() const
        {
            return &this->session()->sessionImpl().template get<ClientAuthProtocolSharedSecret>();
        }

        ClientAuthProtocolSharedSecret* sharedSecretAuth()
        {
            return &this->session()->sessionImpl().template get<ClientAuthProtocolSharedSecret>();
        }
};

template <typename RouterT, typename RequestContextT=DefaultRequestContext, typename MessageBufT=du::WireData, typename RequestUnitT=api::RequestManaged, typename ...ExtraAuthProtocols>
using ClientWithSharedSecretAuthContext=common::TaskContextType<
        ClientWithSharedSecretAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,ExtraAuthProtocols...>,
        typename ClientWithSharedSecretAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,ExtraAuthProtocols...>::Client,
        typename ClientWithSharedSecretAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,ExtraAuthProtocols...>::Session,
        HATN_LOGCONTEXT_NAMESPACE::Context
    >;

template <typename RouterT, typename RequestContextT=DefaultRequestContext, typename MessageBufT=du::WireData, typename RequestUnitT=api::RequestManaged, typename ...ExtraAuthProtocols>
struct makeClientWithSharedSecretAuthContextT
{
    using type=ClientWithSharedSecretAuthContext<RouterT,RequestContextT,MessageBufT,RequestUnitT,ExtraAuthProtocols...>;

    template <typename SubcontextsArgs, typename ...BaseArgs>
    auto operator()(SubcontextsArgs&& subcontextsArgs, BaseArgs&&... baseArgs) const
    {
        auto ctx=common::makeTaskContextType<type>(std::move(subcontextsArgs),std::forward<BaseArgs>(baseArgs)...);
        init(ctx);
        return ctx;
    }

    template <typename SubcontextsArgs>
    auto operator()(SubcontextsArgs&& subcontextsArgs) const
    {
        auto ctx=common::makeTaskContextType<type>(std::move(subcontextsArgs));
        init(ctx);
        return ctx;
    }

    auto operator()() const
    {
        auto ctx=common::makeTaskContextType<type>();
        init(ctx);
        return ctx;
    }

    template <typename SubcontextsArgs, typename ...BaseArgs>
    auto operator()(const common::pmr::polymorphic_allocator<type>& allocator, SubcontextsArgs&& subcontextsArgs, BaseArgs&&... baseArgs) const
    {
        auto ctx=common::allocateTaskContextType<type>(allocator,std::move(subcontextsArgs),std::forward<BaseArgs>(baseArgs)...);
        init(ctx);
        return ctx;
    }

    template <typename SubcontextsArgs>
    auto operator()(const common::pmr::polymorphic_allocator<type>& allocator, SubcontextsArgs&& subcontextsArgs) const
    {
        auto ctx=common::allocateTaskContextType<type>(allocator,std::move(subcontextsArgs));
        init(ctx);
        return ctx;
    }

    auto operator()(const common::pmr::polymorphic_allocator<type>& allocator) const
    {
        auto ctx=common::allocateTaskContextType<type>(allocator);
        init(ctx);
        return ctx;
    }

    private:

        void init(common::SharedPtr<type>& ctx) const
        {
            auto& clientWithAuth=ctx->template get<ClientWithSharedSecretAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,ExtraAuthProtocols...>>();
            auto& client=ctx->template get<typename ClientWithSharedSecretAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,ExtraAuthProtocols...>::Client>();
            auto& session=ctx->template get<typename ClientWithSharedSecretAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,ExtraAuthProtocols...>::Session>();
            clientWithAuth.setClient(&client);
            clientWithAuth.setSession(&session);
        }
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTWITHAUTH_H
