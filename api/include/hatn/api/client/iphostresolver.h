/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/iphostresolver.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIIPHOSTRESOLVER_H
#define HATNAPIIPHOSTRESOLVER_H

#include <hatn/network/asio/caresolver.h>

#include <hatn/api/api.h>
#include <hatn/api/client/resolver.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

struct IpHostName
{
    std::string name;
    uint16_t port=0;
    bool service=false;
    HATN_NETWORK_NAMESPACE::IpVersion ipVersion=HATN_NETWORK_NAMESPACE::IpVersion::ALL;
};

class IpHostResolverTraits
{
    public:

        using ServerName=IpHostName;
        using ResolvedEndpoints=std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint>;

        IpHostResolverTraits(
                common::Thread* thread,
                const std::vector<HATN_NETWORK_NAMESPACE::NameServer>& nameServers={},
                const std::string& resolvConfPath={}
            ) : m_resolver(thread,nameServers,resolvConfPath)
        {}

        template <typename ContextT>
        void resolve(
                common::SharedPtr<ContextT> ctx,
                const ServerName& serverName,
                ResolverCallbackFn<ResolvedEndpoints> callback
            )
        {
            if (serverName.service)
            {
                m_resolver.resolveService(serverName.name,std::move(callback),std::move(ctx),serverName.ipVersion);
            }
            else
            {
                m_resolver.resolveName(serverName.name,std::move(callback),std::move(ctx),serverName.port,serverName.ipVersion);
            }
        }

    private:

        HATN_NETWORK_NAMESPACE::asio::CaResolver m_resolver;
};

using IpHostResolver=Resolver<IpHostResolverTraits>;

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPIIPHOSTRESOLVER_H
