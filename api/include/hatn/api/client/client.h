/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/client.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENT_H
#define HATNAPICLIENT_H

#include <hatn/common/simplequeue.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/taskcontext.h>
#include <hatn/common/threadwithqueue.h>

#include <hatn/base/configobject.h>

#include <hatn/logcontext/context.h>

#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/api/api.h>
#include <hatn/api/connectionpool.h>
#include <hatn/api/method.h>
#include <hatn/api/service.h>
#include <hatn/api/priority.h>
#include <hatn/api/client/session.h>
#include <hatn/api/client/clientrequest.h>
#include <hatn/api/client/defaulttraits.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

constexpr const uint32_t DefaultMaxQueueDepth=256;

HDU_UNIT(config,
    HDU_FIELD(max_queue_depth,TYPE_UINT32,1,false,DefaultMaxQueueDepth)
)

template <typename RouterT,
         template <typename Router, typename Traits> class Transport,
         typename SessionWrapperT,
         typename Traits=DefaultClientTraits>
class Client : public common::TaskSubcontext,
               public base::ConfigObject<config::type>
{
    public:

        using MessageBuf=typename Traits::MessageBuf;
        using RequestUnit=typename Traits::RequestUnit;

        using Req=Request<SessionWrapperT,MessageBuf,RequestUnit>;
        using MessageType=typename Req::MessageType;
        using Context=typename Traits::Context;
        using ReqCtx=RequestContext<Req,Context>;

        using Router=RouterT;

    private:

        using Queue=common::SimpleQueue<common::SharedPtr<ReqCtx>,common::WithLock<>>;
        using SessionWaitingQueue=common::SimpleQueue<common::SharedPtr<ReqCtx>>;

    public:

        Client(
                common::SharedPtr<RouterT> router,
                common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current(),
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_transport(std::move(router),thread,factory),
                m_allocatorFactory(factory),
                m_thread(thread),
                m_closed(false),
                m_networkDisconnected(false),
                m_sessionWaitingQueues(factory->objectAllocator<typename SessionWaitingQueueMap::value_type>())
        {
            handlePriorities(
                [this](Priority priority)
                {
                    resetPriority(priority);
                },
                [this](size_t count)
                {
                    m_queues.reserve(count);
                }
            );
        }

        Client(
                common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current(),
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : Client({},thread,factory)
        {
        }

        Client(
            common::SharedPtr<RouterT> router,
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
            auto ec=base::ConfigObject<config::type>::loadLogConfig(configTree,configPath,records,settings);
            HATN_CHECK_EC(ec)
            ec=m_transport.loadLogConfig(configTree,configPath,records,settings);
            HATN_CHECK_EC(ec)
            return OK;
        }

        Error exec(
            common::SharedPtr<Context> ctx,
            RequestCb<Context> callback,
            SessionWrapperT session,
            const Service& service,
            const Method& method,
            MessageType message,
            lib::string_view topic={},
            Priority priority=Priority::Normal,
            uint32_t timeoutMs=0,
            MethodAuth methodAuth={}
        );

        auto prepare(
            const common::SharedPtr<Context>& ctx,
            SessionWrapperT session,
            const Service& service,
            const Method& method,
            MessageType message,
            lib::string_view topic={},
            MethodAuth methodAuth={}
        );

        Error exec(
            common::SharedPtr<Context> ctx,
            RequestCb<Context> callback,
            const Service& service,
            const Method& method,
            MessageType message,
            lib::string_view topic={},
            Priority priority=Priority::Normal,
            uint32_t timeoutMs=0,
            MethodAuth methodAuth={}
        )
        {
            return exec(std::move(ctx),std::move(callback),SessionWrapperT{},service,method,std::move(message),topic,priority,timeoutMs,std::move(methodAuth));
        }

        auto prepare(
            const common::SharedPtr<Context>& ctx,
            const Service& service,
            const Method& method,
            MessageType message,
            lib::string_view topic={},
            MethodAuth methodAuth={}
        )
        {
            return prepare(ctx,{},service,method,std::move(message),topic,std::move(methodAuth));
        }

        void exec(
            common::SharedPtr<Context> ctx,
            RequestCb<Context> callback,
            common::SharedPtr<ReqCtx> req
        );

        Error cancel(
            common::SharedPtr<ReqCtx>& req
        );

        template <typename ContextT1, typename CallbackT>
        void close(
            common::SharedPtr<ContextT1> ctx,
            CallbackT callback,
            bool callbackRequests
        );

        void setName(lib::string_view name)
        {
            if (name.empty())
            {
                name=this->mainCtx().name();
            }
            m_transport.setName(name);
        }

        auto router() const
        {
            return m_transport.router();
        }

        template <typename T>
        void setRouter(T router) const
        {
            return m_transport.setRouter(std::move(router));
        }

        void setNetworkDisconnected(bool enable)
        {
            m_networkDisconnected.store(enable);
        }

    private:

        void doExec(
            common::SharedPtr<Context> ctx,
            RequestCbInternal<Context> callback,
            common::SharedPtr<ReqCtx> req,            
            bool regenId=false
        );

        void postDequeue(Priority priority);
        void dequeue(Priority priority);

        void resetPriority(Priority priority)
        {
            m_queues.emplace(priority,m_allocatorFactory->objectMemoryResource());
            m_sessionWaitingReqCount.emplace(priority,0);
        }

        void sendRequest(common::SharedPtr<ReqCtx> req);

        template <typename Connection>
        void recvResponse(common::SharedPtr<ReqCtx> req, Connection connection);

        void refreshSession(common::SharedPtr<ReqCtx> req, Response resp={});

        void pushToSessionWaitingQueue(common::SharedPtr<ReqCtx> req);

        Transport<Router,Traits> m_transport;

        const common::pmr::AllocatorFactory* m_allocatorFactory;

        common::FlatMap<Priority,Queue> m_queues;

        common::ThreadQWithTaskContext* m_thread;
        std::atomic<bool> m_closed;
        std::atomic<bool> m_networkDisconnected;

        using SessionWaitingQueueMap=common::pmr::map<SessionId,SessionWaitingQueue,std::less<>>;

        SessionWaitingQueueMap m_sessionWaitingQueues;
        common::FlatMap<Priority,size_t> m_sessionWaitingReqCount;
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

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENT_H
