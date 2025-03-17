/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/serviceclient.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVICECLIENT_H
#define HATNAPISERVICECLIENT_H

#include <hatn/api/service.h>
#include <hatn/api/client/sessionclient.h>
#include <hatn/api/client/methodauth.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename SessionT, typename ClientContextT, typename ClientT, typename MethodAuthHandlerT=MethodAuthHandler<NoMethodAuthTraits>>
class ServiceClient : public Service,
                      public ServiceMethodsAuth<MethodAuthHandlerT>,
                      public SessionClient<SessionT,ClientContextT,ClientT>,
                      public common::pmr::WithFactory
{
    public:

        using ClientWithSession=SessionClient<SessionT,ClientContextT,ClientT>;

        using Session=typename ClientWithSession::Session;
        using Client=typename ClientWithSession::Client;
        using ClientContext=typename ClientWithSession::ClientContext;
        using Context=typename ClientWithSession::Context;
        using MessageType=typename ClientWithSession::MessageType;
        using ReqCtx=typename ClientWithSession::ReqCtx;

        ServiceClient(
                lib::string_view serviceName,
                common::SharedPtr<Session> session,
                common::SharedPtr<ClientContext> clientCtx,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault(),
                uint8_t serviceVersion=1
            ) : Service(serviceName,serviceVersion),
                ServiceMethodsAuth<MethodAuthHandlerT>(this),
                ClientWithSession(std::move(session),std::move(clientCtx)),
                common::pmr::WithFactory(factory)
        {}

        ServiceClient(
                lib::string_view serviceName,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault(),
                uint8_t serviceVersion=1
            ) : Service(serviceName,serviceVersion),
                ServiceMethodsAuth<MethodAuthHandlerT>(this),
                common::pmr::WithFactory(factory)
        {}

        using ClientWithSession::exec;

        Error exec(
            common::SharedPtr<Context> ctx,
            const Method& method,
            MessageType message,
            RequestCb<Context> callback,
            lib::string_view topic={},
            Priority priority=Priority::Normal,
            uint32_t timeoutMs=0
        )
        {
            auto methodAuth=this->makeAuthHeader(method,message,topic,this->factory());
            if (methodAuth)
            {
                return methodAuth.takeError();
            }
            return ClientWithSession::exec(std::move(ctx),*this,method,std::move(message),std::move(callback),topic,priority,timeoutMs,methodAuth.takeValue());
        }

        common::Result<common::SharedPtr<ReqCtx>> prepare(
            const common::SharedPtr<Context>& ctx,
            const Method& method,
            MessageType message,
            lib::string_view topic={}
        )
        {
            auto methodAuth=this->makeAuthHeader(method,message,topic,this->factory());
            if (methodAuth)
            {
                return methodAuth.takeError();
            }
            return ClientWithSession::prepare(std::move(ctx),*this,method,std::move(message),methodAuth.takeValue());
        }
};

template <typename SessionT, typename ClientContextT, typename ClientT, typename MethodAuthHandlerT=MethodAuthHandler<NoMethodAuthTraits>>
using ServiceClientContext=common::TaskContextType<ServiceClient<SessionT,ClientContextT,ClientT,MethodAuthHandlerT>,HATN_LOGCONTEXT_NAMESPACE::Context>;

template <typename T>
struct allocateServiceClientContextT
{
    template <typename ...Args>
    auto operator () (
            const HATN_COMMON_NAMESPACE::pmr::polymorphic_allocator<T>& allocator,
            Args&&... args
        ) const
    {
        return HATN_COMMON_NAMESPACE::allocateTaskContextType<T>(
            allocator,
            HATN_COMMON_NAMESPACE::subcontexts(
                    HATN_COMMON_NAMESPACE::subcontext(std::forward<Args>(args)...),
                    HATN_COMMON_NAMESPACE::subcontext()
                )
            );
    }
};
template <typename T>
constexpr allocateServiceClientContextT<T> allocateServiceClientContext{};

template <typename T>
struct makeServiceClientContextT
{
    template <typename ...Args>
    auto operator () (
            Args&&... args
        ) const
    {
        return HATN_COMMON_NAMESPACE::makeTaskContextType<T>(
            HATN_COMMON_NAMESPACE::subcontexts(
                HATN_COMMON_NAMESPACE::subcontext(std::forward<Args>(args)...),
                HATN_COMMON_NAMESPACE::subcontext()
                )
            );
    }
};
template <typename T>
constexpr makeServiceClientContextT<T> makeServiceClientContext{};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPISERVICECLIENT_H
