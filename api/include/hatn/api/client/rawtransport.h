/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/rawtransport.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTRAWTRANSPORT_H
#define HATNAPICLIENTRAWTRANSPORT_H

#include <hatn/common/threadwithqueue.h>

#include <hatn/base/configobject.h>
#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/logcontext/context.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>
#include <hatn/api/priority.h>
#include <hatn/api/connectionpool.h>

HATN_API_NAMESPACE_BEGIN

class Tenancy;

namespace client {

HDU_UNIT(raw_transport_config,
    HDU_FIELD(max_pool_priority_connections,TYPE_UINT32,2,false,DefaultMaxPoolPriorityConnections)
)

template <typename RouterT, typename Traits>
class RawTransport : public base::ConfigObject<raw_transport_config::type>
{
    public:

        RawTransport(
                common::SharedPtr<RouterT> router,
                common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current(),
                const common::pmr::AllocatorFactory* /*factory*/=common::pmr::AllocatorFactory::getDefault()
            ) : m_connectionPool(std::move(router),thread)
        {
        }

        RawTransport(
                common::SharedPtr<RouterT> router,
                const common::pmr::AllocatorFactory* factory
            ) : RawTransport(std::move(router),common::ThreadQWithTaskContext::current(),factory)
        {}


        RawTransport(
                common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current(),
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : RawTransport({},common::ThreadQWithTaskContext::current(),thread,factory)
        {
        }

        Error loadLogConfig(
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const std::string& configPath,
            HATN_BASE_NAMESPACE::config_object::LogRecords& records,
            const HATN_BASE_NAMESPACE::config_object::LogSettings& settings
        );

        template <typename RequestT>
        Error serializeRequest(
            common::SharedPtr<RequestT> req,
            lib::string_view topic,
            const Tenancy& tenancy
        );

        template <typename RequestT>
        Error serializeRequest(
            common::SharedPtr<RequestT> req
        );

        template <typename RequestT, typename CallbackT>
        void sendRequest(
            common::SharedPtr<RequestT> req,
            CallbackT callback
        );

        template <typename ContextT, typename CallbackT>
        void close(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback
        );

        template <typename RequestT>
        void cancelRequest(
            common::SharedPtr<RequestT>
        )
        {}

        bool canSend(Priority p) const;

        void setName(lib::string_view name)
        {
            m_connectionPool.setName(name);
        }

        void setRouter(common::SharedPtr<RouterT> router)
        {
            m_connectionPool.setRouter(std::move(router));
        }

        auto router() const
        {
            return m_connectionPool.router();
        }

    private:

        ConnectionPool<RouterT> m_connectionPool;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTRAWTRANSPORT_H
