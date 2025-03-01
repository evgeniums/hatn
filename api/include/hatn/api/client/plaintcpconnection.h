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

#endif

namespace client {

using PlainTcpConnection=Connection<TcpClient>;
using PlainTcpConnectionContext=common::TaskContextType<TcpClient,PlainTcpConnection,HATN_LOGCONTEXT_NAMESPACE::Context>;

inline auto makePlainTcpConnectionContext(
        std::vector<IpHostName> hosts,
        std::shared_ptr<IpHostResolver> resolver,
        HATN_COMMON_NAMESPACE::Thread* thread=common::Thread::currentThread(),
        std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle={},
        lib::string_view name={}
    )
{
    auto connectionCtx=HATN_COMMON_NAMESPACE::makeTaskContextType<PlainTcpConnectionContext>(
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(std::move(hosts),std::move(resolver),thread,std::move(shuffle)),
            HATN_COMMON_NAMESPACE::subcontext(),
            HATN_COMMON_NAMESPACE::subcontext()
            ),
        name
        );
    auto& tcpClient=connectionCtx->get<TcpClient>();
    auto& connection=connectionCtx->get<PlainTcpConnection>();
    connection.setStreams(&tcpClient);
    return connectionCtx;
}

inline auto makePlainTcpConnectionContext(
    std::vector<IpHostName> hosts,
    std::shared_ptr<IpHostResolver> resolver,
    HATN_COMMON_NAMESPACE::Thread* thread,
    lib::string_view name={}
    )
{
    return makePlainTcpConnectionContext(std::move(hosts),std::move(resolver),thread,{},name);
}

//! @todo Implement allocate plain tcp context

} // namespace client

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::client::TcpClient,HATN_API_EXPORT)
HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::client::PlainTcpConnection,HATN_API_EXPORT)

#endif // HATNAPITCPCONNECTON_H
