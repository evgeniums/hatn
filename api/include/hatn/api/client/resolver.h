/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/resolver.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIRESOLVER_H
#define HATNAPIRESOLVER_H

#include <hatn/common/error.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/objecttraits.h>

#include <hatn/network/asio/caresolver.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename ResolvedEndpoints>
using ResolverCallbackFn=std::function<void (const Error&, ResolvedEndpoints)>;

template <typename Traits>
class Resolver : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        using ResolvedEndpoints=typename Traits::ResolvedEndpoints;
        using ServerName=typename Traits::ServerName;

        template <typename ContextT>
        void resolve(
                common::SharedPtr<ContextT> ctx,
                const ServerName& serverName,
                ResolverCallbackFn<ResolvedEndpoints> callback
            )
        {
            this->traits().resolve(std::move(ctx),serverName,std::move(callback));
        }
};

struct IpServerName
{
    std::string name;
    uint16_t port=0;
    bool service=false;
    HATN_NETWORK_NAMESPACE::IpVersion ipVersion=HATN_NETWORK_NAMESPACE::IpVersion::ALL;
};

class IpServerResolverTraits
{
    public:

        using ServerName=IpServerName;
        using ResolvedEndpoints=std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint>;

        IpServerResolverTraits(
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

using IpServerResolver=Resolver<IpServerResolverTraits>;

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPIRESOLVER_H
