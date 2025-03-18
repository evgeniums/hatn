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

template <typename SessionT, typename ClientContextT, typename ClientT, typename ServiceMethodsAuthT=ServiceMethodsAuthSingle<NoMethodAuth>>
class ServiceClient : public Service,
                      public ServiceMethodsAuthT,
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
                ServiceMethodsAuthT(this),
                ClientWithSession(std::move(session),std::move(clientCtx)),
                common::pmr::WithFactory(factory)
        {}

        ServiceClient(
                lib::string_view serviceName,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault(),
                uint8_t serviceVersion=1
            ) : Service(serviceName,serviceVersion),
                ServiceMethodsAuthT(this),
                common::pmr::WithFactory(factory)
        {}

        using ClientWithSession::exec;

        void exec(
            common::SharedPtr<Context> ctx,
            RequestCb<Context> callback,
            const Method& method,
            MessageType message,            
            lib::string_view topic={},
            Priority priority=Priority::Normal,
            uint32_t timeoutMs=0
        )
        {
            auto methodAuthCb=[selfCtx{this->sharedMainCtx()},this,&method,message,topic,callback{std::move(callback)},priority,timeoutMs]
                    (auto ctx, const Error& ec, MethodAuth methodAuth)
            {
                if (ec)
                {
                    callback(std::move(ctx),ec,Response{});
                    return;
                }

                auto ec1=ClientWithSession::exec(ctx,*this,method,std::move(message),std::move(callback),topic,priority,timeoutMs,methodAuth);
                if (ec1)
                {
                    callback(ctx,ec1,Response{});
                }
            };
            this->makeAuthHeader(std::move(ctx),method,message,topic,this->factory());
        }

        template <typename CallbackT>
        void prepare(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const Method& method,
            MessageType message,
            lib::string_view topic={}
        )
        {
            auto methodAuthCb=[selfCtx{this->sharedMainCtx()},this,&method,message,topic,callback{std::move(callback)}](auto ctx, const Error& ec, MethodAuth methodAuth)
            {
                if (ec)
                {
                    callback(std::move(ctx),ec,common::SharedPtr<ReqCtx>{});
                    return;
                }

                auto req=ClientWithSession::prepare(ctx,*this,method,std::move(message),topic,std::move(methodAuth));
                if (req)
                {
                    callback(std::move(ctx),req.takeError(),common::SharedPtr<ReqCtx>{});
                    return;
                }
                callback(std::move(ctx),Error{},req.takeValue());
            };
            this->makeAuthHeader(std::move(ctx),method,message,topic,this->factory());
        }
};

template <typename SessionT, typename ClientContextT, typename ClientT, typename ServiceMethodsAuthT=ServiceMethodsAuthSingle<NoMethodAuth>>
using ServiceClientContext=common::TaskContextType<ServiceClient<SessionT,ClientContextT,ClientT,ServiceMethodsAuthT>,HATN_LOGCONTEXT_NAMESPACE::Context>;

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

template <typename Traits>
class MappedServiceClients : public common::WithTraits<Traits>,
                             public common::TaskSubcontext
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT, typename CallbackT>
        void findServiceClient(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            lib::string_view topic={}
        ) const
        {
            this->traits().findServiceClient(std::move(ctx),std::move(callback),topic);
        }
};

template <typename ServiceClientContextT, typename ServiceClientT>
class SingleServiceClientTraits
{
    public:

        using ServiceClientContext=ServiceClientContextT;
        using ServiceClient=ServiceClientT;

        SingleServiceClientTraits(common::SharedPtr<ServiceClientContext> serviceClientCtx) : m_serviceClientCtx(std::move(serviceClientCtx))
        {}

        void setServiceClient(common::SharedPtr<ServiceClientContext> serviceClientCtx)
        {
            m_serviceClientCtx=std::move(serviceClientCtx);
        }

        common::SharedPtr<ServiceClientContext> singleServiceClientCtx() const
        {
            return m_serviceClientCtx;
        }

        ServiceClient* singleServiceClient() const
        {
            if (!m_serviceClientCtx)
            {
                return nullptr;
            }
            return &m_serviceClientCtx->template get<ServiceClient>();
        }

        template <typename ContextT, typename CallbackT>
        void findServiceClient(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            lib::string_view topic={}
            ) const
        {
            std::ignore=topic;
            callback(std::move(ctx),m_serviceClientCtx,m_serviceClientCtx->template get<ServiceClient>());
        }

    private:

        common::SharedPtr<ServiceClientContext> m_serviceClientCtx;
};

template <typename ServiceClientContextT, typename ServiceClientT>
class SingleServiceClient : public MappedServiceClients<SingleServiceClientTraits<ServiceClientContextT,ServiceClientT>>
{
    public:

        using Base=MappedServiceClients<SingleServiceClientTraits<ServiceClientContextT,ServiceClientT>>;

        using ServiceClientContext=typename Base::ServiceClientContext;
        using ServiceClient=typename Base::ServiceClient;

        using Base::Base;

        void setServiceClient(common::SharedPtr<ServiceClientContext> serviceClientCtx)
        {
            this->traits().setServiceClient(std::move(serviceClientCtx));
        }

        common::SharedPtr<ServiceClientContext> singleServiceClientCtx() const
        {
            return this->traits().singleServiceClientCtx();
        }

        ServiceClient* singleServiceClient() const
        {
            return this->traits().singleServiceClient();
        }
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPISERVICECLIENT_H
