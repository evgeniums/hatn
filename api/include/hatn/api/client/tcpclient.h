/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/tcpclient.h
  *
  */

/****************************************************************************/

#ifndef HATNAPITCPCLIENT_H
#define HATNAPITCPCLIENT_H

#include <hatn/common/thread.h>

#include <hatn/network/resolvershuffle.h>
#include <hatn/network/asio/tcpstream.h>

#include <hatn/api/api.h>
#include <hatn/api/client/iphostresolver.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

class TcpClient : public HATN_NETWORK_NAMESPACE::asio::TcpStream
{
    public:

        using Base=HATN_NETWORK_NAMESPACE::asio::TcpStream;

        TcpClient(
                std::shared_ptr<IpHostResolver> resolver,
                common::Thread* thread=common::Thread::currentThread(),
                std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle={}
            ) : Base(thread),
                m_resolver(std::move(resolver)),
                m_shuffle(std::move(shuffle))
        {}

        TcpClient(
            std::vector<IpHostName> hosts,
            std::shared_ptr<IpHostResolver> resolver,
            common::Thread* thread=common::Thread::currentThread(),
            std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> shuffle={}
            ) : Base(thread),
                m_resolver(std::move(resolver)),
                m_shuffle(std::move(shuffle)),
                m_hosts(std::move(hosts))
        {}

        void prepare(
                std::function<void (const Error &)> callback
            )
        {
            if (isClosed())
            {
                reset();
            }
            resolveNext(0,std::move(callback));
        }

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

    private:

        void connectNext(size_t hostsIdx, size_t epIdx, std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> eps, std::function<void (const Error &)> callback)
        {
            if (isClosed())
            {
                callback(commonError(CommonError::ABORTED));
                return;
            }

            setRemoteEndpoint(eps[epIdx]);

            auto cb=[this,hostsIdx,epIdx{epIdx+1},eps{std::move(eps)},callback{std::move(callback)}](const Error &ec)
            {
                if (ec)
                {
                    if (epIdx==eps.size())
                    {
                        if (hostsIdx==m_hosts.size())
                        {
                            callback(ec);
                        }
                        else
                        {
                            resolveNext(hostsIdx,std::move(callback));
                        }
                    }
                    else
                    {
                        connectNext(hostsIdx,epIdx,std::move(eps),std::move(callback));
                    }
                    return;
                }
                callback(ec);
            };            
            Base::prepare(cb);
        }

        void resolveNext(size_t hostsIdx, std::function<void (const Error &)> callback)
        {
            if (isClosed())
            {
                callback(commonError(CommonError::ABORTED));
                return;
            }

            if (m_hosts.empty() || hostsIdx==m_hosts.size())
            {
                callback(HATN_NETWORK_NAMESPACE::networkError(HATN_NETWORK_NAMESPACE::NetworkError::DNS_FAILED));
                return;
            }

            if (hostsIdx>=m_hosts.size())
            {
                hostsIdx=0;
            }
            auto ctx=sharedMainCtx();
            auto cb=[this,callback{std::move(callback)},ctx,hostsIdx{hostsIdx+1}](const Error& ec, std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> eps)
            {
                std::ignore=ctx;
                if (ec)
                {
                    if (hostsIdx<m_hosts.size())
                    {
                        resolveNext(hostsIdx,std::move(callback));
                    }
                    else
                    {
                        callback(HATN_NETWORK_NAMESPACE::networkError(HATN_NETWORK_NAMESPACE::NetworkError::DNS_FAILED));
                    }
                    return;
                }

                if (eps.empty())
                {
                    if (hostsIdx<m_hosts.size())
                    {
                        resolveNext(hostsIdx,std::move(callback));
                    }
                    else
                    {
                        callback(HATN_NETWORK_NAMESPACE::networkError(HATN_NETWORK_NAMESPACE::NetworkError::DNS_FAILED));
                    }
                    return;
                }

                if (m_shuffle)
                {
                    m_shuffle->shuffle(eps);
                }
                connectNext(hostsIdx,0,std::move(eps),std::move(callback));
            };
            m_resolver->resolve(ctx,m_hosts[hostsIdx],cb);
        }

        std::shared_ptr<IpHostResolver> m_resolver;
        std::shared_ptr<HATN_NETWORK_NAMESPACE::ResolverShuffle> m_shuffle;
        std::vector<IpHostName> m_hosts;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPITCPCLIENT_H
