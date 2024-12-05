/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/resolverinterface.h
  *
  *     Resolvers interface to use in environment
  *
  */

/****************************************************************************/

#ifndef HATNNETWORKRESOLVERINTERFACE_H
#define HATNNETWORKRESOLVERINTERFACE_H

#include <memory>

#include <hatn/common/interface.h>

#include <hatn/network/resolver.h>
#include <hatn/network/network.h>

namespace hatn {
namespace network {

//! Resolver interface to use in environment
template <network::Protocol proto> class ResolverInterface : public common::Interface<ResolverInterface<proto>>
{
    public:

        //! Get resolver
        inline network::Resolver* resolver()
        {
            return m_resolver.get();
        }

        //! Get shuffle
        inline network::ResolverShuffle* shuffle()
        {
            return m_shuffle.get();
        }

        //! Set DNS resolver
        inline void setResolver(const std::shared_ptr<network::Resolver>& resolver)
        {
            Assert(resolver->protocol()==proto,"Invalid protocol of resolver");
            m_resolver=resolver;
        }

        //! Set result shuffle
        inline void setShuffle(const std::shared_ptr<network::ResolverShuffle>& shuffle)
        {
            m_shuffle=shuffle;
        }

    private:

        std::shared_ptr<network::Resolver> m_resolver;
        std::shared_ptr<network::ResolverShuffle> m_shuffle;
};

template class HATN_NETWORK_EXPORT ResolverInterface<network::Protocol::UDP>;
template class HATN_NETWORK_EXPORT ResolverInterface<network::Protocol::TCP>;

class HATN_NETWORK_EXPORT TcpResolverInterface : public ResolverInterface<network::Protocol::TCP>
{
    public:

        DCS_CUID_DECLARE()
        ~TcpResolverInterface();
};

class HATN_NETWORK_EXPORT UdpResolverInterface : public ResolverInterface<network::Protocol::UDP>
{
    public:

        DCS_CUID_DECLARE()
        ~UdpResolverInterface();
};

//---------------------------------------------------------------
    } // namespace network
} // namespace hatn
#endif // HATNNETWORKRESOLVERINTERFACE_H
