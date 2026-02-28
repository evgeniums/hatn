/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file grpcclient/grpctransport.h
  *
  */

/****************************************************************************/

#ifndef HATNGRPCTRANSPORT_H
#define HATNGRPCTRANSPORT_H

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/base/configobject.h>

#include <hatn/logcontext/context.h>

#include <hatn/api/api.h>
#include <hatn/api/client/defaulttraits.h>
#include <hatn/api/client/clientrequest.h>

#include <hatn/grpcclient/grpcclient.h>
#include <hatn/grpcclient/grpcrouter.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

HDU_UNIT(config,
    HDU_FIELD(var1,TYPE_UINT32,1)
)

template <typename SessionWrapperT, typename Traits=clientapi::DefaultClientTraits>
class Client : public common::TaskSubcontext,
               public base::ConfigObject<config::type>
{
    public:

        using Req=clientapi::Request<SessionWrapperT,typename Traits::MessageBuf,typename Traits::RequestUnit>;
        using MessageType=typename Req::MessageType;
        using Context=typename Traits::Context;
        using ReqCtx=clientapi::RequestContext<Req,Context>;

    public:

        Client(
                common::SharedPtr<Router> router,
                common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current(),
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_router(std::move(router)),
                m_allocatorFactory(factory),
                m_thread(thread),                
                m_closed(false),
                m_networkDisconnected(false)
        {
        }

        Client(
            common::SharedPtr<Router> router,
            const common::pmr::AllocatorFactory* factory
            ) : Client(std::move(router),common::ThreadQWithTaskContext::current(),factory)
        {}

        Error loadLogConfig(
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const std::string& configPath,
            HATN_BASE_NAMESPACE::config_object::LogRecords& records,
            const HATN_BASE_NAMESPACE::config_object::LogSettings& settings
        )
        {
        }

        Error exec(
            common::SharedPtr<Context> ctx,
            clientapi::RequestCb<Context> callback,
            SessionWrapperT session,
            const api::Service& service,
            const api::Method& method,
            MessageType message,
            lib::string_view topic={},
            api::Priority priority=api::Priority::Normal,
            uint32_t timeoutMs=0,
            clientapi::MethodAuth methodAuth={}
        );

        Error exec(
            common::SharedPtr<Context> ctx,
            clientapi::RequestCb<Context> callback,
            const api::Service& service,
            const api::Method& method,
            MessageType message,
            lib::string_view topic={},
            api::Priority priority=api::Priority::Normal,
            uint32_t timeoutMs=0,
            clientapi::MethodAuth methodAuth={}
        )
        {
            return exec(std::move(ctx),std::move(callback),SessionWrapperT{},service,method,std::move(message),topic,priority,timeoutMs,std::move(methodAuth));
        }

        void exec(
            common::SharedPtr<Context> ctx,
            clientapi::RequestCb<Context> callback,
            common::SharedPtr<ReqCtx> req
        );

        template <typename ContextT1, typename CallbackT>
        void close(
            common::SharedPtr<ContextT1> ctx,
            CallbackT callback,
            bool callbackRequests
        );

        void setName(std::string name)
        {
            m_name=std::move(name);
        }

        auto router() const
        {
            return m_router;
        }

        void setNetworkDisconnected(bool enable)
        {
            m_networkDisconnected.store(enable);
        }

    private:

        common::SharedPtr<Router> m_router;
        const common::pmr::AllocatorFactory* m_allocatorFactory;
        common::ThreadQWithTaskContext* m_thread;

        std::atomic<bool> m_closed;
        std::atomic<bool> m_networkDisconnected;

        std::string m_name;
};

template <typename ClientT>
using ClientContext=common::TaskContextType<ClientT,HATN_LOGCONTEXT_NAMESPACE::Context>;

template <typename T>
struct allocateClientContextT
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
constexpr allocateClientContextT<T> allocateClientContext{};

template <typename T>
struct makeClientContextT
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
constexpr makeClientContextT<T> makeClientContext{};

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCTRANSPORT_H
