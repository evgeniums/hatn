/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/plaintcprouter.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIPLAINTCPROUTER_H
#define HATNAPIPLAINTCPROUTER_H

#include <hatn/logcontext/logcontext.h>

#include <hatn/api/api.h>
#include <hatn/api/router.h>
#include <hatn/api/client/plaintcpconnection.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

class TcpClientConfig
{
    public:

        //! @todo Adjust socket settings

        TcpClientConfig(common::Thread* thread=common::Thread::currentThread()) : m_thread(thread)
        {}

        TcpClientConfig(
            std::shared_ptr<IpHostResolver> resolver,
            common::Thread* thread=common::Thread::currentThread(),
            std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle={}
            ) : m_thread(thread),
            m_resolver(std::move(resolver)),
            m_shuffle(std::move(shuffle))
        {}

        TcpClientConfig(
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

class PlainTcpRouterTraits
{
    public:

        using ConnectionContext=PlainTcpConnectionContext;
        using Connection=PlainTcpConnection;

        template <typename ...Args>
        PlainTcpRouterTraits(
                Args&... args
            ) : m_config(std::forward<Args>(args)...),
                m_allocatorFactory(common::pmr::AllocatorFactory::getDefault())
        {}

        template <typename ...Args>
        PlainTcpRouterTraits(
                const common::pmr::AllocatorFactory* allocatorFactory,
                Args&... args
            ) : m_config(std::forward<Args>(args)...),
                m_allocatorFactory(allocatorFactory)
        {}

        template <typename ContextT>
        void makeConnection(
                common::SharedPtr<ContextT> ctx,
                RouterCallbackFn<ConnectionContext> callback,
                lib::string_view name={}
            )
        {
            std::ignore=ctx;
            callback(Error{},
                     allocatePlainTcpConnectionContext(m_allocatorFactory->objectAllocator<ConnectionContext>(),
                                                       m_config.hosts(),m_config.resolver(),m_config.thread(),m_config.shuffle(),name)
                     );
        }

    private:

        TcpClientConfig m_config;
        const common::pmr::AllocatorFactory* m_allocatorFactory;
};

using PlainTcpRouter=Router<PlainTcpRouterTraits>;

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPIPLAINTCPROUTER_H
