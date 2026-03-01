/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/tcpclientconfig.h
  *
  */

/****************************************************************************/

#ifndef HATNTCPCLIENTCONFIG_H
#define HATNTCPCLIENTCONFIG_H

#include <hatn/logcontext/logcontext.h>
#include <hatn/network/resolvershuffle.h>

#include <hatn/api/api.h>
#include <hatn/api/client/iphostresolver.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

class TcpClientConfig
{
    public:

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

        void setHost(IpHostName host) noexcept
        {
            setHosts({std::move(host)});
        }

        void setHost(std::string name, uint16_t port) noexcept
        {
            auto host=IpHostName{std::move(name),port};
            setHost(std::move(host));
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

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNTCPCLIENTCONFIG_H
