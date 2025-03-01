/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/connectionpool.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISCONNECTIONPOOL_H
#define HATNAPISCONNECTIONPOOL_H

#include <hatn/common/sharedptr.h>
#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/flatmap.h>

#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/api/api.h>
#include <hatn/api/router.h>
#include <hatn/api/priority.h>
#include <hatn/api/apiconstants.h>

HATN_API_NAMESPACE_BEGIN

template <typename RouterTraits>
class ConnectionPool
{
    using ConnectionContext=typename RouterTraits::ConnectionContext;
    using Connections=common::pmr::map<common::TaskContextId,ConnectionContext,std::less<>>;

    public:

        using Connection=typename RouterTraits::Connection;

        ConnectionPool(
                common::SharedPtr<Router<RouterTraits>> router,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_router(router),
                m_allocatorFactory(factory),
                m_maxConnectionsPerPriority(DefaultMaxPoolPriorityConnections),
                m_closed(false)
        {
            handlePriorities(
                [this](Priority priority)
                {
                    resetPriority(priority);
                },
                [this](size_t count)
                {
                    m_connections.reserve(count);
                }
            );
        }

        void setMaxConnectionsPerPriority(size_t maxConnectionsPerPriority) noexcept
        {
            m_maxConnectionsPerPriority=maxConnectionsPerPriority;
        }

        bool canSend(Priority priority) const
        {
            auto it=m_connections.find(priority);
            Assert(it!=m_connections.end(),"Invalid priority");
            return it->second.size()<m_maxConnectionsPerPriority;
        }

        template <typename ContextT>
        void close(
                common::SharedPtr<ContextT> ctx,
                std::function<void (const Error&)> callback
            )
        {
            m_closed=true;
            closeNextPriority(std::move(ctx),std::move(callback));
        }

        void send(
                common::SharedPtr<common::TaskContext> taskCtx,
                Priority priority,
                common::SpanBuffers buffers,
                std::function<void (const Error& ec, Connection* connection)> cb
            )
        {
            //! @todo check if pool is closed

            auto it=m_connections.find(priority);

            //! @todo find not busy connection
            //! check if connection is closed, destroy closed and look for next
            //! create new connection if all busy
            //! set task context as parent to found/created connection context
            //! send to found/created connection

            //! destroy connection in case of error
            //! try to send via other connection if transferred bytes==0
            //! invoke callback
        }

        void recv(
                common::SharedPtr<common::TaskContext> taskCtx,
                Connection* connection,
                du::WireBufSolid& buf,
                std::function<void (const Error& ec)> cb
            )
        {
            //! @todo check if pool is closed

            //! @todo receive data from connection to buf
            //! destroy connection in case of error
            //! mark connection as not busy after receiving
            //! invoke callback
        }

    private:

        template <typename ContextT>
        void closeNextPriority(common::SharedPtr<ContextT> ctx, std::function<void (const Error&)> cb)
        {
            auto it=m_connections.begin();
            if (it==it->m_connections.end())
            {
                cb(Error{});
                return;
            }

            closeNextConnection(
                std::move(ctx),
                it->second,
                [ctx{std::move(ctx)},&it,cb{std::move(cb)},this]()
                {
                    m_connections.erase(it);
                    closeNextPriority(std::move(ctx),std::move(cb));
                }
            );
        }

        template <typename ContextT>
        void closeNextConnection(common::SharedPtr<ContextT> ctx, Connections& connections, std::function<void (const Error&)> cb)
        {
            auto it=connections.begin();
            if (it==connections.end())
            {
                cb(Error{});
                return;
            }
            auto connectionCtx=it->second;
            auto& connection=connectionCtx->template get<Connection>();
            connection.close(
                [ctx{std::move(ctx)},&it,&connections,cb{std::move(cb)},this](const Error&)
                {
                    connections.erase(it);
                    closeNextConnection(std::move(ctx),connections,std::move(cb));
                }
            );
        }

        void resetPriority(Priority priority)
        {
            m_connections.emplace(priority,m_allocatorFactory->objectMemoryResource());
        }

        common::SharedPtr<Router<RouterTraits>> m_router;
        const common::pmr::AllocatorFactory* m_allocatorFactory;

        common::FlatMap<Priority,Connections> m_connections;

        size_t m_maxConnectionsPerPriority;
        bool m_closed;
};

HATN_API_NAMESPACE_END

#endif // HATNAPISCONNECTIONPOOL_H
