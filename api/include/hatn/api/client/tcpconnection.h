/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/tcpconnection.h
  *
  */

/****************************************************************************/

#ifndef HATNAPITCPCONNECTON_H
#define HATNAPITCPCONNECTON_H

#include <hatn/logcontext/logcontext.h>

#include <hatn/api/api.h>
#include <hatn/api/connection.h>
#include <hatn/api/client/router.h>
#include <hatn/api/client/tcpclient.h>

HATN_API_NAMESPACE_BEGIN

#if 0

template <typename ParentT, typename BaseT=common::TaskContext>
class TaskContextWithParent : public BaseT
{
    public:

        using BaseT::BaseT;

        void resetParent(common::SharedPtr<ParentT> parent={}) noexcept
        {
            m_parent=std::move(parent);
        }

        common::SharedPtr<ParentT> parent() const noexcept
        {
            return m_parent;
        }

    private:

        common::SharedPtr<ParentT> m_parent;
};

#endif

namespace client {

using TcpConnection=Connection<TcpClient>;

class TcpClientBuilderContext
{
    public:

        TcpClientBuilderContext(common::Thread* thread=common::Thread::currentThread()) : m_thread(thread)
        {}

        TcpClientBuilderContext(
            std::shared_ptr<IpHostResolver> resolver,
            common::Thread* thread=common::Thread::currentThread(),
            std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle={}
        ) : m_thread(thread),
            m_resolver(std::move(resolver)),
            m_shuffle(std::move(shuffle))
        {}

        TcpClientBuilderContext(
            std::vector<IpHostName> hosts,
            std::shared_ptr<IpHostResolver> resolver,
            common::Thread* thread=common::Thread::currentThread(),
            std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle={}
        ) : m_thread(thread),
            m_hosts(std::move(hosts)),
            m_resolver(std::move(resolver)),
            m_shuffle(std::move(shuffle))
        {}

        void setResolver(std::shared_ptr<IpHostResolver> resolver) noexcept
        {
            m_resolver=std::move(resolver);
        }

        void setShuffle(std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle) noexcept
        {
            m_shuffle=std::move(shuffle);
        }

        void setHosts(std::vector<IpHostName> hosts) noexcept
        {
            m_hosts=std::move(hosts);
        }

        const std::vector<IpHostName>& hosts() const noexcept
        {
            return m_hosts;
        }

        std::shared_ptr<IpHostResolver> resolver() const noexcept
        {
            return m_resolver;
        }

        common::Thread* thread() const noexcept
        {
            return m_thread;
        }

        std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle() const noexcept
        {
            return m_shuffle;
        }

    private:

        common::Thread* m_thread;
        std::vector<IpHostName> m_hosts;
        std::shared_ptr<IpHostResolver> m_resolver;
        std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> m_shuffle;
};

#if 0
template <typename BaseT>
using TcpConnectionContext=common::TaskContextType<TcpConnection,BaseT>;

template <typename ConnectionCtxT>
class TcpConnectionTraits
{
    public:

        using Connection=TcpConnection;

        template <typename ContextT>
        void makeConnection(
                common::SharedPtr<ContextT> ctx,
                RouterCallbackFn<ConnectionCtxT> callback
            )
        {
            TcpConnectionRouterContext* routerCtx=ctx->template get<TcpConnectionRouterContext>();
        }

    private:

};
#endif

} // namespace client

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::client::TcpClient,HATN_API_EXPORT)
HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::client::TcpConnection,HATN_API_EXPORT)

#endif // HATNAPITCPCONNECTON_H
