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
#include <hatn/common/asiotimer.h>
#include <hatn/common/taskcontext.h>

#include <hatn/dataunit/unitwrapper.h>

#include <hatn/api/api.h>
#include <hatn/api/connectionpool.h>
#include <hatn/api/method.h>
#include <hatn/api/service.h>
#include <hatn/api/priority.h>
#include <hatn/api/client/router.h>
#include <hatn/api/client/session.h>
#include <hatn/api/client/request.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT=request::shared_managed>
class Client : public common::TaskSubcontext
{
    public:

        using Connection=typename RouterTraits::Connection;
        using Context=ContextT;
        using Req=Request<SessionTraits,ContextT,RequestUnitT>;

    private:

        struct QueueItem
        {
            Req req;
            common::WeakPtr<Context> ctx;
            RequestCb<Context> callback;
            common::AsioDeadlineTimer timer;
        };

        using Queue=common::SimpleQueue<QueueItem>;

    public:

        Client(
                common::SharedPtr<Router<RouterTraits>> router,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
               ) : m_router(std::move(router)),
                   m_allocatorFactory(factory)
        {
            resetQueue(Priority::Lowest);
            resetQueue(Priority::Low);
            resetQueue(Priority::Normal);
            resetQueue(Priority::High);
            resetQueue(Priority::Highest);
        }

        template <typename UnitT>
        Error exec(
            common::SharedPtr<Context> ctx,
            common::SharedPtr<Session<SessionTraits>> session,
            const Service& service,
            const Method& method,
            const UnitT& content,
            RequestCb<Context> callback,
            lib::string_view topic={},
            Priority priority=Priority::Normal,
            uint32_t timeoutMs=0
        );

        template <typename UnitT>
        common::Result<Req> prepare(
            common::SharedPtr<Session<SessionTraits>> session,
            const Service& service,
            const Method& method,
            const UnitT& content
        );

        template <typename UnitT>
        Error exec(
            common::SharedPtr<Context> ctx,
            const Service& service,
            const Method& method,
            const UnitT& content,
            RequestCb<Context> callback,
            lib::string_view topic={},
            Priority priority=Priority::Normal,
            uint32_t timeoutMs=0
        )
        {
            return exec(std::move(ctx),{},service,method,content,std::move(callback),topic,priority,timeoutMs);
        }

        template <typename UnitT>
        common::Result<Req> prepare(
            const Service& service,
            const Method& method,
            const UnitT& content
        )
        {
            return prepare({},service,method,content);
        }

        void exec(
            common::SharedPtr<Context> ctx,
            Req req,
            RequestCb<Context> callback
        );

        Error cancel(
            Req req
        );

        template <typename ContextT1, typename CallbackT>
        void close(
            common::SharedPtr<ContextT1> ctx,
            CallbackT callback
        );

    private:

        void doExec(
            common::SharedPtr<Context> ctx,
            Req req,
            RequestCb<Context> callback,
            bool regenId=false
        );

        void maybeReadQueue(Queue& queue);

        void resetQueue(Priority priority)
        {
            m_queues.emplace(priority,m_allocatorFactory->objectMemoryResource());
        }

        ConnectionPool<Connection> m_connections;
        common::SharedPtr<Router<RouterTraits>> m_router;

        const common::pmr::AllocatorFactory* m_allocatorFactory;

        common::FlatMap<Priority,Queue> m_queues;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENT_H
