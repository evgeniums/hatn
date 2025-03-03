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

#include <hatn/base/configobject.h>

#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/api/api.h>
#include <hatn/api/connectionpool.h>
#include <hatn/api/method.h>
#include <hatn/api/service.h>
#include <hatn/api/priority.h>
#include <hatn/api/client/session.h>
#include <hatn/api/client/request.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

constexpr const uint32_t DefaultMaxQueueDepth=256;

HDU_UNIT(config,
    HDU_FIELD(max_queue_depth,TYPE_UINT32,1,false,DefaultMaxQueueDepth)
    HDU_FIELD(max_pool_priority_connections,TYPE_UINT32,2,false,DefaultMaxPoolPriorityConnections)
)

class ClientConfig : public base::ConfigObject<config::type>
{
    public:
};

template <typename RouterTraits, typename SessionTraits, typename TaskContextT, typename MessageBufT=du::WireData, typename RequestUnitT=request::shared_managed>
class Client : public common::TaskSubcontext
{
    public:

        using Req=Request<SessionTraits,MessageBufT,RequestUnitT>;
        using MessageType=typename Req::MessageType;
        using ReqCtx=RequestContext<Req,TaskContextT>;

    private:

        struct Queue : public common::SimpleQueue<ReqCtx>
        {
            using common::SimpleQueue<ReqCtx>::SimpleQueue;
            bool busy=false;
        };

    public:

        using Context=TaskContextT;

        Client(
                std::shared_ptr<ClientConfig> cfg,
                common::SharedPtr<Router<RouterTraits>> router,
                common::Thread* thread=common::Thread::currentThread(),
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_cfg(std::move(cfg)),
                m_connectionPool(std::move(router)),
                m_allocatorFactory(factory),
                m_thread(thread),
                m_closed(false),
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

            m_connectionPool.setMaxConnectionsPerPriority(cfg->config().fieldValue(config::max_pool_priority_connections));
        }

        Client(
            std::shared_ptr<ClientConfig> cfg,
            common::SharedPtr<Router<RouterTraits>> router,
            const common::pmr::AllocatorFactory* factory
            ) : Client(std::move(cfg),std::move(router),common::Thread::currentThread(),factory)
        {}

        Error exec(
            common::SharedPtr<Context> ctx,
            common::SharedPtr<Session<SessionTraits>> session,
            const Service& service,
            const Method& method,
            MessageType message,
            RequestCb<Context> callback,
            lib::string_view topic={},
            Priority priority=Priority::Normal,
            uint32_t timeoutMs=0,
            MethodAuth methodAuth={}
        );

        common::Result<common::SharedPtr<ReqCtx>> prepare(
            common::SharedPtr<Session<SessionTraits>> session,
            const Service& service,
            const Method& method,
            MessageType message,
            MethodAuth methodAuth={}
        );

        Error exec(
            common::SharedPtr<Context> ctx,
            const Service& service,
            const Method& method,
            MessageType message,
            RequestCb<Context> callback,
            lib::string_view topic={},
            Priority priority=Priority::Normal,
            uint32_t timeoutMs=0,
            MethodAuth methodAuth={}
        )
        {
            return exec(std::move(ctx),{},service,method,std::move(message),std::move(callback),topic,priority,timeoutMs,std::move(methodAuth));
        }

        common::Result<common::SharedPtr<ReqCtx>> prepare(
            const Service& service,
            const Method& method,
            MessageType message,
            MethodAuth methodAuth={}
        )
        {
            return prepare({},service,method,std::move(message),std::move(methodAuth));
        }

        void exec(
            common::SharedPtr<Context> ctx,
            common::SharedPtr<ReqCtx> req,
            RequestCb<Context> callback
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
            m_connectionPool.setName(name);
        }

    private:

        void doExec(
            common::SharedPtr<Context> ctx,
            common::SharedPtr<ReqCtx> req,
            RequestCb<Context> callback,
            bool regenId=false
        );

        void dequeue(Priority priority);

        void resetPriority(Priority priority)
        {
            m_queues.emplace(priority,m_allocatorFactory->objectMemoryResource());
            m_sessionWaitingReqCount.emplace(priority,0);
        }

        void sendRequest(common::SharedPtr<ReqCtx> req);

        template <typename Connection>
        void recvResponse(common::SharedPtr<ReqCtx> req, Connection connection);

        void refreshSession(common::SharedPtr<ReqCtx> req, Response resp);

        void pushToSessionWaitingQueue(common::SharedPtr<ReqCtx> req);

        std::shared_ptr<ClientConfig> m_cfg;

        ConnectionPool<RouterTraits> m_connectionPool;

        const common::pmr::AllocatorFactory* m_allocatorFactory;

        common::FlatMap<Priority,Queue> m_queues;

        common::Thread* m_thread;
        bool m_closed;

        using SessionWaitingQueueMap=common::pmr::map<SessionId,Queue,std::less<>>;

        SessionWaitingQueueMap m_sessionWaitingQueues;
        common::FlatMap<Priority,size_t> m_sessionWaitingReqCount;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENT_H
